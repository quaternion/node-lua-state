#pragma once

#include <napi.h>

class LuaError : public Napi::ObjectWrap<LuaError> {
public:
  static void NapiInit(Napi::Env, Napi::Object);
  static Napi::Error New(Napi::Env, const Napi::Object&);

  LuaError(const Napi::CallbackInfo&);

private:
  static inline Napi::FunctionReference constructor_;
};