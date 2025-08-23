#pragma once

#include <napi.h>

#include "lua-state-context.h"

class LuaStateWrapper : public Napi::ObjectWrap<LuaStateWrapper> {
public:
  LuaStateWrapper(const Napi::CallbackInfo& info);
  ~LuaStateWrapper();

  static Napi::Object init(Napi::Env env, Napi::Object exports);

private:
  LuaStateContext ctx_;

  // --- Eval methods
  Napi::Value evalLuaFile(const Napi::CallbackInfo& info);
  Napi::Value evalLuaString(const Napi::CallbackInfo& info);

  // --- Global methods
  Napi::Value getLuaGlobalValue(const Napi::CallbackInfo& info);
  Napi::Value getLuaValueLength(const Napi::CallbackInfo& info);
  Napi::Value setLuaGlobalValue(const Napi::CallbackInfo& info);
};
