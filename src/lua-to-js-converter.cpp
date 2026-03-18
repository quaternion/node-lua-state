#include <cassert>

#include "lua-to-js-converter.h"

#include <iostream>

LuaToJsConverter::LuaToJsConverter(const Napi::Env& env, LuaJsRuntime& runtime) : env_(env), runtime_(runtime) {
  objects_.reserve(8);
  results_.reserve(8);
}

LuaToJsConverter::~LuaToJsConverter() {}

// Visitor Implementation

void LuaToJsConverter::OnValue(LuaNil _value) { results_.push_back(env_.Null()); }
void LuaToJsConverter::OnValue(LuaBool value) { results_.push_back(Napi::Boolean::New(env_, value.value)); }
void LuaToJsConverter::OnValue(LuaNumber value) { results_.push_back(Napi::Number::New(env_, value.value)); }
void LuaToJsConverter::OnValue(LuaString value) { results_.push_back(Napi::String::New(env_, value.ptr, value.len)); }
void LuaToJsConverter::OnValue(LuaFunction value) { results_.push_back(runtime_.CreateJsProxyFunction(env_, value)); }
bool LuaToJsConverter::OnValue(LuaTable value) {
  auto [it, inserted] = objects_.try_emplace(value.identity, Napi::Object::New(env_));
  results_.push_back(it->second);
  return inserted;
}

void LuaToJsConverter::SetTable(LuaTable table) { current_object_ = &objects_.at(table.identity); }
bool LuaToJsConverter::IsVisited(LuaTable table) { return objects_.contains(table.identity); };

void LuaToJsConverter::OnProperty(LuaTableKey key, LuaNil _value) { SetProperty(key, env_.Null()); }
void LuaToJsConverter::OnProperty(LuaTableKey key, LuaBool value) { SetProperty(key, Napi::Boolean::New(env_, value.value)); }
void LuaToJsConverter::OnProperty(LuaTableKey key, LuaNumber value) { SetProperty(key, Napi::Number::New(env_, value.value)); }
void LuaToJsConverter::OnProperty(LuaTableKey key, LuaString value) { SetProperty(key, Napi::String::New(env_, value.ptr, value.len)); }
void LuaToJsConverter::OnProperty(LuaTableKey key, LuaFunction value) { SetProperty(key, runtime_.CreateJsProxyFunction(env_, value)); }
bool LuaToJsConverter::OnProperty(LuaTableKey key, LuaTable value) {
  auto [it, inserted] = objects_.try_emplace(value.identity, Napi::Object::New(env_));
  SetProperty(key, it->second);
  return inserted;
}

// Result

Napi::Value LuaToJsConverter::GetResult() {
  size_t size = results_.size();

  if (size == 0) {
    return env_.Undefined();
  }

  if (size == 1) {
    return results_[0];
  }

  auto array = Napi::Array::New(env_, size);

  for (size_t i = 0; i < size; i++) {
    array.Set(i, std::move(results_[i]));
  }

  return array;
}

const std::vector<Napi::Value>& LuaToJsConverter::Results() { return results_; }

// Private

void LuaToJsConverter::SetProperty(LuaTableKey key, Napi::Value value) {
  std::visit(
    [&](auto&& k) {
      using T = std::decay_t<decltype(k)>;

      if constexpr (std::is_same_v<T, LuaString>) {
        current_object_->Set(Napi::String::New(env_, k.ptr, k.len), value);
      } else {
        current_object_->Set(k.value, value);
      }
    },
    key
  );
}
