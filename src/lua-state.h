#pragma once

#include <napi.h>

#include "lua-state-context.h"

class LuaState : public Napi::ObjectWrap<LuaState> {
public:
  LuaState(const Napi::CallbackInfo&);

  static void Init(Napi::Env, Napi::Object);

private:
  LuaStateContext ctx_;

  // --- Eval methods
  Napi::Value EvalLuaFile(const Napi::CallbackInfo&);
  Napi::Value EvalLuaString(const Napi::CallbackInfo&);

  // --- Global methods
  Napi::Value GetLuaGlobalValue(const Napi::CallbackInfo&);
  Napi::Value GetLuaValueLength(const Napi::CallbackInfo&);
  Napi::Value GetLuaVersion(const Napi::CallbackInfo&);
  Napi::Value SetLuaGlobalValue(const Napi::CallbackInfo&);
};
