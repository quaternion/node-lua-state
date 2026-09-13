#pragma once

#include <napi.h>

class LuaStateError : public Napi::ObjectWrap<LuaStateError> {
public:
  static void NapiInit(Napi::Env, Napi::Object);
  static Napi::Error New(Napi::Env, const char* code, const char* message);

  LuaStateError(const Napi::CallbackInfo&);

private:
  static inline Napi::FunctionReference constructor_;
};