#pragma once

#include <napi.h>

class LuaError : public Napi::ObjectWrap<LuaError> {
public:
  static void Init(Napi::Env env, Napi::Object exports);
  static Napi::Error New(const Napi::Env&, const std::string&, const std::string& = "");

private:
  static inline Napi::FunctionReference lua_error_js_constructor;
};