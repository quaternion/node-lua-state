#include <cassert>
#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>

#include "js-object-lua-ref-weak-map.hpp"
#include "js-to-lua-converter.h"
#include "lua-js-runtime.h"
#include "lua-state-core.h"

JsToLuaConverter::JsToLuaConverter(const Napi::Env& env, LuaStateCore& core) : core_(core), visited_(env) { lua_refs_.reserve(32); }

JsToLuaConverter::~JsToLuaConverter() {
  for (auto& ref : lua_refs_) {
    core_.ReleaseRef(ref);
  }
}

void JsToLuaConverter::PushValue(const Napi::Value& value) {
  auto value_type = value.Type();

  if (value_type != napi_object) {
    PushPrimitive(value_type, value);
    return;
  }

  if (value.IsDate()) {
    core_.PushNumber(value.As<Napi::Date>().ValueOf());
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
  {
    LuaRegistryRef ref;
    if (visited_.TryGet(object, ref)) {
      core_.PushRef(ref);
      return;
    }
  }

  std::vector<Napi::Object> stack;
  stack.reserve(32);

  auto push_new_table = [&](const Napi::Object& obj) -> LuaRegistryRef {
    core_.NewTable();
    auto ref = core_.PopRef();

    visited_.Set(obj, ref);
    lua_refs_.push_back(ref);

    return ref;
  };

  auto push_value = [&](const Napi::Value& value) {
    napi_valuetype value_type = value.Type();

    if (value_type != napi_object) {
      PushPrimitive(value_type, value);
    } else if (value.IsDate()) {
      core_.PushNumber(value.As<Napi::Date>().ValueOf());
    } else {
      Napi::Object child = value.As<Napi::Object>();

      LuaRegistryRef child_ref;

      if (!visited_.TryGet(child, child_ref)) {
        child_ref = push_new_table(child);
        stack.push_back(child);
      }

      core_.PushRef(child_ref);
    }
  };

  auto root_ref = push_new_table(object);

  stack.push_back(object);

  while (!stack.empty()) {
    auto current_obj = stack.back();
    stack.pop_back();

    LuaRegistryRef current_ref;
    bool ok = visited_.TryGet(current_obj, current_ref);
    assert(ok);

    core_.PushRef(current_ref);

    if (current_obj.IsArray()) {
      Napi::Array array = current_obj.As<Napi::Array>();
      auto length = array.Length();

      for (uint32_t i = 0; i < length; ++i) {
        Napi::Value array_item = array.Get(i);
        push_value(array_item);
        core_.SetIndex(-2, i + 1);
      }
    } else {
      // TODO: change to "napi_get_all_property_names"
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
}
