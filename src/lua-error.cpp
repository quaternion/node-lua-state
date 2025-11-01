#include "lua-error.h"

/**
 * napi initializer
 */
void LuaError::Init(Napi::Env env, Napi::Object exports) {
  Napi::Function lua_error_js_class = Napi::Function::New(
    env,
    [](const Napi::CallbackInfo& info) {
      std::string message = info.Length() > 0 ? info[0].As<Napi::String>().Utf8Value() : "";
      std::string stack = info.Length() > 1 ? info[1].As<Napi::String>().Utf8Value() : "";

      Napi::Object self = info.This().As<Napi::Object>();

      self.Set("name", "LuaError");
      self.Set("message", message);

      std::string formattedStack;

      if (!stack.empty()) {
        formattedStack = "LuaError" + (!message.empty() ? ": " + message : "");
        formattedStack += "\n" + stack;
      }
      self.Set("stack", formattedStack);

      return self;
    },
    "LuaError"
  );

  auto global = env.Global();

  // set ptototypes
  Napi::Function set_prototype_of = global.Get("Object").ToObject().Get("setPrototypeOf").As<Napi::Function>();
  Napi::Function error_js_class = global.Get("Error").As<Napi::Function>();

  set_prototype_of.Call({lua_error_js_class, error_js_class});
  set_prototype_of.Call({lua_error_js_class.Get("prototype"), error_js_class.Get("prototype")});

  lua_error_js_class.Get("prototype").As<Napi::Object>().Set("constructor", lua_error_js_class);

  // save js constructor as static variable
  lua_error_js_constructor_ = Napi::Persistent(lua_error_js_class);
  env.AddCleanupHook([] { lua_error_js_constructor_.Reset(); });

  exports.Set("LuaError", lua_error_js_class);
}

/**
 * factory
 */
Napi::Error LuaError::New(const Napi::Env& env, const std::string& message, const std::string& stack) {
  Napi::Object lua_error_js_instance = lua_error_js_constructor_.New({Napi::String::New(env, message), Napi::String::New(env, stack)});

  return Napi::Error(env, lua_error_js_instance);
}