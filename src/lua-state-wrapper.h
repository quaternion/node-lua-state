#pragma once

#include <napi.h>

extern "C" {
#include <lua.h>
}

class LuaStateWrapper : public Napi::ObjectWrap<LuaStateWrapper> {
public:
  static Napi::Object init(Napi::Env env, Napi::Object exports);

  LuaStateWrapper(const Napi::CallbackInfo& info);
  ~LuaStateWrapper();

private:
  lua_State* lua_state_;

  // --- Eval methods
  Napi::Value evalLuaFile(const Napi::CallbackInfo& info);
  Napi::Value evalLuaString(const Napi::CallbackInfo& info);

  // --- Global methods
  Napi::Value getLuaGlobalValue(const Napi::CallbackInfo& info);
  Napi::Value getLuaValueLength(const Napi::CallbackInfo& info);
  Napi::Value setLuaGlobalValue(const Napi::CallbackInfo& info);
};
