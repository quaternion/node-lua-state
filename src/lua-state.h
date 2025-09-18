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
  Napi::Value EvalLuaFile(const Napi::CallbackInfo& info);
  Napi::Value EvalLuaString(const Napi::CallbackInfo& info);

  // --- Global methods
  Napi::Value GetLuaGlobalValue(const Napi::CallbackInfo& info);
  Napi::Value GetLuaValueLength(const Napi::CallbackInfo& info);
  Napi::Value SetLuaGlobalValue(const Napi::CallbackInfo& info);
};
