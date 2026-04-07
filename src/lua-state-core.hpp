#pragma once

#include <unordered_set>
#include <vector>

// #include "lua-state-core.h"
#include "lua-values.h"
#include "lua-visitor-concept.h"

extern "C" {
#include <lauxlib.h>
#include <lua.h>
#include <lualib.h>
}

template <LuaVisitor Visitor> inline void LuaStateCore::Traverse(int index, Visitor& visitor) {
  switch (lua_type(L_, index)) {
    case LUA_TNUMBER:
      visitor.OnValue(LuaNumber{lua_tonumber(L_, index)});
      break;
    case LUA_TSTRING: {
      size_t len;
      const char* ptr = lua_tolstring(L_, index, &len);
      visitor.OnValue(LuaString{ptr, len});
      break;
    }
    case LUA_TBOOLEAN:
      visitor.OnValue(LuaBool{lua_toboolean(L_, index) != 0});
      break;
    case LUA_TFUNCTION: {
      visitor.OnValue(LuaFunction{lua_topointer(L_, index), index});
      break;
    }
    case LUA_TTABLE:
      TraverseTable(index, visitor);
      break;
    default:
      visitor.OnValue(LuaNil{});
      break;
  }
}

template <LuaVisitor Visitor> void LuaStateCore::TraverseTable(int index, Visitor& visitor) {
  struct QueueFrame {
    LuaRegistryRef ref;
    LuaTable table;
  };

  auto root_table = LuaTable{lua_topointer(L_, index)};
  auto root_ref = CopyRef(index);

  visitor.OnValue(root_table);

  std::vector<QueueFrame> queue;
  queue.reserve(8);
  queue.emplace_back(QueueFrame{root_ref, root_table});

  while (!queue.empty()) {
    QueueFrame current_frame = queue.back();
    queue.pop_back();

    // push table to lua stack by ref
    PushRef(current_frame.ref);

    int table_index = GetTop();

    visitor.SetTable(current_frame.table);

    // loop by table properties
    PushNil();

    while (lua_next(L_, table_index)) {
      auto prop_key_type = lua_type(L_, -2);

      // continue if key is not number or string
      if (prop_key_type != LUA_TNUMBER && prop_key_type != LUA_TSTRING) {
        Pop(1);
        continue;
      }

      // fetch table key
      LuaTableKey key = [&]() -> LuaTableKey {
        if (prop_key_type == LUA_TNUMBER) {
          return LuaNumber{lua_tonumber(L_, -2)};
        }

        size_t len;
        const char* ptr = lua_tolstring(L_, -2, &len);
        return LuaString{ptr, len};
      }();

      switch (lua_type(L_, -1)) {
        case LUA_TNUMBER:
          visitor.OnProperty(key, LuaNumber{lua_tonumber(L_, -1)});
          break;
        case LUA_TSTRING: {
          size_t len;
          const char* ptr = lua_tolstring(L_, -1, &len);
          visitor.OnProperty(key, LuaString{ptr, len});
          break;
        }
        case LUA_TBOOLEAN:
          visitor.OnProperty(key, LuaBool{lua_toboolean(L_, -1) != 0});
          break;
        case LUA_TFUNCTION: {
          visitor.OnProperty(key, LuaFunction{lua_topointer(L_, -1), -1});
          break;
        }
        case LUA_TTABLE: {
          auto child_table = LuaTable{lua_topointer(L_, -1)};

          // if the child table has not been visited, add a new frame to the queue
          if (visitor.OnProperty(key, child_table)) {
            auto child_ref = CopyRef(-1);
            queue.emplace_back(QueueFrame{child_ref, child_table});
          }
          break;
        }
        default:
          visitor.OnProperty(key, LuaNil{});
          break;
      }

      Pop(1);
    }

    Pop(1);

    ReleaseRef(current_frame.ref);
  }
};
