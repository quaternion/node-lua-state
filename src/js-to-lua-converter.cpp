#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>

#include "js-to-lua-converter.h"
#include "lua-js-runtime.h"
#include "lua-state-core.h"

JsToLuaConverter::JsToLuaConverter(LuaStateCore& core) : core_(core) {}

void JsToLuaConverter::NapiInit(Napi::Env env) {
  visited_symbol_ref_ = Napi::Persistent(Napi::Symbol::New(env));
  env.AddCleanupHook([] { visited_symbol_ref_.Reset(); });
}

void JsToLuaConverter::PushValue(const Napi::Value& value) {
  auto value_type = value.Type();

  if (value_type != napi_object) {
    PushPrimitive(value_type, value);
    return;
  }

  if (value.IsDate()) {
    double millisecons = value.As<Napi::Date>().ValueOf();
    core_.PushNumber(millisecons);
    return;
  }

  PushObject(value.As<Napi::Object>());
};

void JsToLuaConverter::PushPrimitive(const napi_valuetype value_type, const Napi::Value& value) {
  switch (value_type) {
    case napi_string:
      core_.PushString(value.As<Napi::String>().Utf8Value());
      break;
    case napi_number:
      core_.PushNumber(value.As<Napi::Number>().DoubleValue());
      break;
    case napi_bigint:
      core_.PushString(value.As<Napi::BigInt>().ToString().Utf8Value());
      break;
    case napi_boolean:
      core_.PushBool(value.As<Napi::Boolean>());
      break;
    case napi_function: {
      auto fn = value.As<Napi::Function>();
      auto* holder = static_cast<JsFunctionHolder*>(core_.NewUserData(sizeof(JsFunctionHolder)));
      new (holder) JsFunctionHolder{Napi::Persistent(fn)};

      core_.PushMetaTable(LuaJsRuntime::MetaTableName);
      core_.SetMetaTable(-2);

      break;
    }
    default:
      core_.PushNil();
      break;
  }
}

void JsToLuaConverter::PushObject(const Napi::Object& object) {
  auto env = object.Env();
  Napi::Symbol visited_symbol = visited_symbol_ref_.Value();

  std::vector<Napi::Object> stack;
  std::vector<Napi::Object> visited;
  stack.reserve(32);
  visited.reserve(32);

  auto push_new_table = [&](const Napi::Object& obj) -> LuaRegistryRef {
    core_.NewTable();
    auto ref = core_.PopRef();
    obj.Set(visited_symbol, Napi::Number::New(env, ref.value));
    visited.push_back(obj);
    return ref;
  };

  auto push_value = [&](const Napi::Value& value) {
    napi_valuetype value_type = value.Type();

    if (value_type != napi_object) {
      PushPrimitive(value_type, value);
    } else {
      // FIXME: handle Date
      Napi::Object child = value.As<Napi::Object>();

      Napi::Value child_visited_value = child.Get(visited_symbol);

      if (child_visited_value.IsNumber()) {
        int child_ref_value = child_visited_value.As<Napi::Number>().Int64Value();
        core_.PushRef(LuaRegistryRef{child_ref_value});
      } else {
        auto ref = push_new_table(child);
        core_.PushRef(ref);
        stack.push_back(child);
      }
    }
  };

  auto root_ref = push_new_table(object);
  // core_.Pop(1);

  stack.push_back(object);

  while (!stack.empty()) {
    auto current_obj = stack.back();
    stack.pop_back();

    const int current_ref_value = current_obj.Get(visited_symbol).As<Napi::Number>().Int64Value();
    core_.PushRef(LuaRegistryRef{current_ref_value});

    if (current_obj.IsArray()) {
      Napi::Array array = current_obj.As<Napi::Array>();
      auto length = array.Length();

      for (uint32_t i = 0; i < length; ++i) {
        Napi::Value array_item = array.Get(i);
        push_value(array_item);
        core_.SetIndex(-2, i + 1);
      }
    } else {
      Napi::Array prop_names = current_obj.GetPropertyNames();
      auto length = prop_names.Length();

      for (uint32_t i = 0; i < length; ++i) {
        Napi::Value prop_name = prop_names.Get(i);
        Napi::Value prop_value = current_obj.Get(prop_name);
        push_value(prop_value);
        core_.SetField(-2, prop_name.As<Napi::String>().Utf8Value());
      }
    }

    core_.Pop(1);
  }

  core_.PushRef(root_ref);

  for (auto& obj : visited) {
    int visited_ref_value = obj.Get(visited_symbol).As<Napi::Number>().Int64Value();
    core_.ReleaseRef(LuaRegistryRef{visited_ref_value});
    obj.Delete(visited_symbol);
  }
}
