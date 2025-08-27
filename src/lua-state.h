#pragma once

#include <napi.h>

#include "lua-state-context.h"

class LuaState : public Napi::ObjectWrap<LuaState> {
public:
  LuaState(const Napi::CallbackInfo& info);

  static void Init(Napi::Env env, Napi::Object exports);

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
