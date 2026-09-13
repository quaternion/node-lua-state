#include "napi/lua-state-error.h"

/**
 * Napi Initializer
 */
void LuaStateError::NapiInit(Napi::Env env, Napi::Object exports) {
  auto lua_state_error_class = DefineClass(env, "LuaStateError", {});

  auto global = env.Global();
  auto error_class = global.Get("Error").As<Napi::Function>();

  auto set_proto = global.Get("Object").As<Napi::Object>().Get("setPrototypeOf").As<Napi::Function>();

  set_proto.Call({lua_state_error_class, error_class});
  set_proto.Call({lua_state_error_class.Get("prototype"), error_class.Get("prototype")});

  constructor_ = Napi::Persistent(lua_state_error_class);
  constructor_.SuppressDestruct();

  exports.Set("LuaStateError", lua_state_error_class);
}

/**
 * Constructor
 */
LuaStateError::LuaStateError(const Napi::CallbackInfo& info) : Napi::ObjectWrap<LuaStateError>(info) {
  Napi::Env env = info.Env();

  Napi::Object self = info.This().As<Napi::Object>();
  self.Set("name", "LuaStateError");

  Napi::String message_value = info.Length() > 0 ? info[0].As<Napi::String>() : Napi::String::New(env, "");
  self.Set("message", message_value);

  if (info.Length() > 1 && info[1].IsObject()) {
    Napi::Object options = info[1].As<Napi::Object>();

    auto cause_value = options.Get("cause");
    if (!cause_value.IsUndefined()) {
      self.Set("cause", cause_value);
    }
  }
}

/**
 * Factory
 */
Napi::Error LuaStateError::New(Napi::Env env, const char* code, const char* message) {
  Napi::Object instance = constructor_.New({Napi::String::New(env, message)});
  instance.Set("code", code);

  return Napi::Error(env, instance);
}