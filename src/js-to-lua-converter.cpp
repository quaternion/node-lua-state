#include <cassert>
#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>

#include "js-object-lua-ref-map.hpp"
#include "js-to-lua-converter.h"
#include "lua-js-runtime.h"
#include "lua-state-core.h"
#include "napi/napi-string-buffer.h"

JsToLuaConverter::JsToLuaConverter(const Napi::Env& env, LuaStateCore& core) : env_(env), core_(core) { lua_refs_.reserve(32); }

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
    case napi_string: {
      NapiStringBuffer<1024> buf;
      if (buf.TryFastString(value.Env(), value)) {
        core_.PushString(buf.GetFastString());
      } else {
        core_.PushString(buf.GetSlowString(value.Env(), value));
      }
      break;
    }
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
  if (visited_) {
    LuaRegistryRef ref;
    if (visited_->TryGet(object, ref)) {
      core_.PushRef(ref);
      return;
    }
  }

  struct StackFrame {
    Napi::Object obj;
    Napi::Array props;
    bool is_array;
    int length;
  };

  std::vector<StackFrame> stack;
  stack.reserve(32);

  auto push_new_table = [&](const Napi::Object& obj) -> LuaRegistryRef {
    int array_length = 0;
    int obj_length = 0;
    bool is_array = obj.IsArray();
    Napi::Array props;

    if (is_array) {
      array_length = obj.As<Napi::Array>().Length();
    } else {
      props = obj.GetPropertyNames();
      obj_length = props.Length();
    }

    core_.NewTable(array_length, obj_length);

    if (!visited_) {
      visited_ = std::make_unique<JsObjectLuaRefMap>(env_);
    }

    auto ref = core_.PopRef();
    visited_->Set(obj, ref);
    lua_refs_.emplace_back(ref);

    stack.emplace_back(StackFrame{obj, std::move(props), is_array, is_array ? array_length : obj_length});

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

      if (!visited_->TryGet(child, child_ref)) {
        child_ref = push_new_table(child);
      }

      core_.PushRef(child_ref);
    }
  };

  auto root_ref = push_new_table(object);
  napi_env env = object.Env();
  NapiStringBuffer<256> prop_name_buf;

  while (!stack.empty()) {
    auto current_frame = stack.back();
    stack.pop_back();

    LuaRegistryRef current_ref;
    bool ok = visited_->TryGet(current_frame.obj, current_ref);
    assert(ok);

    core_.PushRef(current_ref);

    if (current_frame.is_array) {
      Napi::Array array = current_frame.obj.As<Napi::Array>();

      for (uint32_t i = 0; i < current_frame.length; ++i) {
        Napi::Value array_item = array.Get(i);
        push_value(array_item);
        core_.SetIndex(-2, i + 1);
      }
    } else {
      for (uint32_t i = 0; i < current_frame.length; ++i) {
        Napi::Value prop_name = current_frame.props.Get(i);
        Napi::Value prop_value = current_frame.obj.Get(prop_name);

        push_value(prop_value);

        if (prop_name_buf.TryFastString(env, prop_name)) {
          core_.SetField(-2, prop_name_buf.GetFastString());
        } else {
          core_.SetField(-2, prop_name_buf.GetSlowString(env, prop_name));
        }
      }
    }

    core_.Pop(1);
  }

  core_.PushRef(root_ref);
}
