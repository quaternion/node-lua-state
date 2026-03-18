#pragma once

#include <optional>
#include <string>

#include "lua-values.h"
#include "lua-visitor-concept.h"

extern "C" {
#include <lua.h>
}

class LuaStateCore {
public:
  explicit LuaStateCore();
  ~LuaStateCore();

  void OpenLibs(const std::optional<std::vector<std::string>>&);
  void Close();
  bool IsClosed();
  std::string GetLuaVersion();

  LuaRegistryRef PopRef();
  LuaRegistryRef CopyRef(int index);
  void ReleaseRef(const LuaRegistryRef&);
  void PushRef(const LuaRegistryRef&);

  int GetTop();
  void Pop(int n);

  void PushNil();
  void PushBool(bool);
  void PushNumber(double);
  void PushString(std::string_view);
  void PushValue(int index);
  void PushLightUserData(void* data);
  void PushMetaTable(std::string_view name);
  void PushCClosure(lua_CFunction fn, int args_count);

  void NewTable();
  bool NewMetaTable(std::string_view name);
  void* NewUserData(size_t size);

  void SetField(int table_index, std::string_view key);
  void SetIndex(int table_index, int i);
  void SetMetaTable(int index);
  void SetGlobal(std::string_view name);

  void Error(std::string_view msg);

  template <LuaVisitor Visitor> void Traverse(int index, Visitor& visitor);

  void LoadString(const std::string_view& source) noexcept(false);
  void LoadFile(const std::string_view& path) noexcept(false);
  int PCall(int args_count) noexcept(false);
  std::optional<int> GetLength(int index);

  enum class PushValueByPathStatus { NotFound, BrokenPath, Found };
  PushValueByPathStatus PushValueByPath(const std::string& path);

  void PrintLuaStack(const std::string_view& title);
  void SetTop(int idx) { lua_settop(L_, idx); }

  struct LuaException {};

  struct StackGuard {
  public:
    explicit StackGuard(const LuaStateCore& core) : L_(core.L_), index_(lua_gettop(core.L_)) {}
    ~StackGuard() noexcept { lua_settop(L_, index_); }

  private:
    lua_State* L_;
    int index_;
  };

private:
  lua_State* L_;
  bool closed_;
};

#include "lua-state-core.hpp"