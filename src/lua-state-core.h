#pragma once

#include <mutex>
#include <optional>
#include <string>
#include <unordered_map>
#include <variant>

#include "lua-values.h"
#include "lua-visitor-concept.h"

extern "C" {
#include <lua.h>
}

class LuaStateCore {
public:
  explicit LuaStateCore();
  ~LuaStateCore();

  static LuaStateCore* From(lua_State*);

  void OpenLibs(const std::optional<std::vector<std::string>>&);
  void Close();
  bool IsClosed();

  void AttachUserData(const void*, const void*);
  static void* GetUserData(const void*, lua_State* L);

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

  template <LuaVisitor Visitor> void Traverse(int index, Visitor& visitor);

  std::optional<int> PCall(int args_count);
  std::optional<int> GetLength(int index);

  enum class PushValueByPathStatus { NotFound, BrokenPath, Found };
  PushValueByPathStatus PushValueByPath(const std::string& path);

  bool LoadString(const std::string_view& source);
  bool LoadFile(const std::string_view& path);

  void PrintLuaStack(const std::string_view& title);
  void SetTop(int idx) { lua_settop(L_, idx); }

  std::string GetLuaVersion();

  // ============================================

  // void SetLuaValue(const std::string&, const Napi::Value&);

  Napi::Function FindOrCreateJsFunction(const Napi::Env&, int);

  struct StackGuard {
  public:
    explicit StackGuard(const LuaStateCore& core) : L_(core.L_), index_(lua_gettop(core.L_)) {}
    ~StackGuard() { lua_settop(L_, index_); }

  private:
    lua_State* L_;
    int index_;
  };

private:
  lua_State* L_;
  bool closed_;

  std::unordered_map<const void*, Napi::Function> js_functions_cache_;
  std::mutex js_functions_cache_mtx_;

  static inline std::unordered_map<lua_State*, LuaStateCore*> contexts_;
};

#include "lua-state-core.hpp"