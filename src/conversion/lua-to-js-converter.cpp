#include "conversion/lua-to-js-converter.h"
#include "runtime/lua-js-runtime.h"

LuaToJsConverter::LuaToJsConverter(LuaJsRuntime& runtime) : runtime_(runtime) {
  objects_.reserve(64);
  results.reserve(16);
}

LuaToJsConverter::~LuaToJsConverter() {}

LuaToJsConverter::Scope LuaToJsConverter::CreateScope(const Napi::Env& env) {
  env_ = &env;
  return Scope(*this);
}

// Visitor Implementation

void LuaToJsConverter::OnValue(LuaNil _value) { results.emplace_back(env_->Null()); }
void LuaToJsConverter::OnValue(LuaBool value) { results.emplace_back(Napi::Boolean::New(*env_, value.value)); }
void LuaToJsConverter::OnValue(LuaNumber value) { results.emplace_back(Napi::Number::New(*env_, value.value)); }
void LuaToJsConverter::OnValue(LuaString value) { results.emplace_back(Napi::String::New(*env_, value.ptr, value.len)); }
void LuaToJsConverter::OnValue(LuaFunction value) { results.emplace_back(runtime_.CreateJsProxyFunction(*env_, value)); }
bool LuaToJsConverter::OnValue(LuaTable value) {
  auto [it, inserted] = objects_.try_emplace(value.identity, Napi::Object::New(*env_));
  results.emplace_back(it->second);
  return inserted;
}

void LuaToJsConverter::SetTable(LuaTable table) {
  auto it = objects_.find(table.identity);
  current_object_ = it->second;
}
void LuaToJsConverter::OnProperty(LuaTableKey key, LuaNil _value) { SetProperty(key, env_->Null()); }
void LuaToJsConverter::OnProperty(LuaTableKey key, LuaBool value) { SetProperty(key, Napi::Boolean::New(*env_, value.value)); }
void LuaToJsConverter::OnProperty(LuaTableKey key, LuaNumber value) { SetProperty(key, Napi::Number::New(*env_, value.value)); }
void LuaToJsConverter::OnProperty(LuaTableKey key, LuaString value) { SetProperty(key, Napi::String::New(*env_, value.ptr, value.len)); }
void LuaToJsConverter::OnProperty(LuaTableKey key, LuaFunction value) { SetProperty(key, runtime_.CreateJsProxyFunction(*env_, value)); }
bool LuaToJsConverter::OnProperty(LuaTableKey key, LuaTable value) {
  auto [it, inserted] = objects_.try_emplace(value.identity, Napi::Object::New(*env_));
  SetProperty(key, it->second);
  return inserted;
}

// Result

Napi::Value LuaToJsConverter::BuildResult() {
  size_t size = results.size();

  if (size == 0) {
    return env_->Undefined();
  }

  if (size == 1) {
    return results[0];
  }

  auto array = Napi::Array::New(*env_, size);

  for (size_t i = 0; i < size; i++) {
    array.Set(i, results[i]);
  }

  return array;
}

// Private

void LuaToJsConverter::SetProperty(LuaTableKey key, Napi::Value value) {
  std::visit(
    [&](auto&& k) {
      using T = std::decay_t<decltype(k)>;

      if constexpr (std::is_same_v<T, LuaString>) {
        current_object_.Set(Napi::String::New(*env_, k.ptr, k.len), value);
      } else {
        current_object_.Set(k.value, value);
      }
    },
    key
  );
}

void LuaToJsConverter::Reset() {
  objects_.clear();
  results.clear();
}
