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

template <LuaVisitor Visitor> void LuaStateCore::Traverse(int index, Visitor& visitor) {
  auto root_type = lua_type(L_, index);

  // quick return if value is not table
  if (root_type != LUA_TTABLE) {
    DispatchPrimitive(L_, root_type, index, [&](auto&& value) { visitor.OnValue(value); });
    return;
  }

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

      auto prop_value_type = lua_type(L_, -1);

      if (prop_value_type != LUA_TTABLE) {
        DispatchPrimitive(L_, prop_value_type, -1, [&](auto&& value) { visitor.OnProperty(key, value); });
      } else {
        auto child_table = LuaTable{lua_topointer(L_, -1)};

        // if the child table has not been visited, add a new frame to the queue
        if (visitor.OnProperty(key, child_table)) {
          auto child_ref = CopyRef(-1);
          queue.emplace_back(QueueFrame{child_ref, child_table});
        }
      }

      Pop(1);
    }

    Pop(1);

    ReleaseRef(current_frame.ref);
  }
};

template <typename Callback> void DispatchPrimitive(lua_State* L, const int lua_type, const int index, Callback&& cb) {
  switch (lua_type) {
    case LUA_TNUMBER:
      cb(LuaNumber{lua_tonumber(L, index)});
      break;
    case LUA_TSTRING: {
      size_t len;
      const char* ptr = lua_tolstring(L, index, &len);
      cb(LuaString{ptr, len});
      break;
    }
    case LUA_TBOOLEAN:
      cb(LuaBool{lua_toboolean(L, index) != 0});
      break;
    case LUA_TFUNCTION: {
      cb(LuaFunction{lua_topointer(L, index), index});
      break;
    }
    case LUA_TNIL:
      cb(LuaNil{});
      break;
  }
}