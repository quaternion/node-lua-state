#pragma once

#include <napi.h>

#include "napi/napi-string-buffer.h"
#include "runtime/lua-js-runtime.h"

class LuaState : public Napi::ObjectWrap<LuaState> {
public:
  LuaState(const Napi::CallbackInfo&);

  static void NapiInit(Napi::Env, Napi::Object);

private:
  std::shared_ptr<LuaJsRuntime> runtime_;
  NapiStringBuffer<256> string_buf_;

  Napi::Value Close(const Napi::CallbackInfo&);

  // --- Eval methods
  Napi::Value EvalLuaFile(const Napi::CallbackInfo&);
  Napi::Value EvalLuaString(const Napi::CallbackInfo&);

  // --- Global methods
  Napi::Value GetLuaGlobalValue(const Napi::CallbackInfo&);
  Napi::Value GetLuaValueLength(const Napi::CallbackInfo&);
  Napi::Value GetLuaVersion(const Napi::CallbackInfo&);
  Napi::Value SetLuaGlobalValue(const Napi::CallbackInfo&);

  // --- Config
  static LuaConfig ParseLuaConfig(const Napi::CallbackInfo&);
};
