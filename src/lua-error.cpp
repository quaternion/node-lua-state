#include "lua-error.h"

/**
 * Napi Initializer
 */
void LuaError::Init(Napi::Env env, Napi::Object exports) {
  auto lua_error_class = DefineClass(env, "LuaError", {});

  auto global = env.Global();
  auto error_class = global.Get("Error").As<Napi::Function>();

  auto set_proto = global.Get("Object").As<Napi::Object>().Get("setPrototypeOf").As<Napi::Function>();

  set_proto.Call({lua_error_class, error_class});
  set_proto.Call({lua_error_class.Get("prototype"), error_class.Get("prototype")});

  constructor_ = Napi::Persistent(lua_error_class);
  constructor_.SuppressDestruct();

  exports.Set("LuaError", lua_error_class);
}

/**
 * Constructor
 */
LuaError::LuaError(const Napi::CallbackInfo& info) : Napi::ObjectWrap<LuaError>(info) {
  Napi::Env env = info.Env();

  Napi::Object self = info.This().As<Napi::Object>();
  self.Set("name", "LuaError");

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
Napi::Error LuaError::New(Napi::Env env, const Napi::Object& lua_error) {
  auto message_value = lua_error.Get("message");
  auto cause_value = lua_error.Get("cause");
  auto stack_value = lua_error.Get("stack");

  auto message = message_value.IsString()      ? message_value.As<Napi::String>()
                 : message_value.IsUndefined() ? Napi::String::New(env, "")
                                               : message_value.ToString();

  Napi::Object options = Napi::Object::New(env);
  if (!cause_value.IsUndefined()) {
    options.Set("cause", cause_value);
  }

  Napi::Object instance = constructor_.New({message, options});

  if (stack_value.IsString()) {
    std::string msg = message.Utf8Value();
    std::string header = "LuaError:";
    if (!msg.empty()) {
      header += " " + msg;
    }

    std::string stack = header + "\n    " + stack_value.As<Napi::String>().Utf8Value();

    instance.Set("stack", stack);
  }

  return Napi::Error(env, instance);
}