#include "lua-to-js-converter.h"

LuaToJsConverter::LuaToJsConverter(const Napi::Env& env, LuaJsRuntime& runtime, LuaToJsContext& ctx) : env_(env), runtime_(runtime), ctx_(ctx) {}

LuaToJsConverter::~LuaToJsConverter() {}

// Visitor Implementation

void LuaToJsConverter::OnValue(LuaNil _value) { ctx_.results.emplace_back(env_.Null()); }
void LuaToJsConverter::OnValue(LuaBool value) { ctx_.results.emplace_back(Napi::Boolean::New(env_, value.value)); }
void LuaToJsConverter::OnValue(LuaNumber value) { ctx_.results.emplace_back(Napi::Number::New(env_, value.value)); }
void LuaToJsConverter::OnValue(LuaString value) { ctx_.results.emplace_back(Napi::String::New(env_, value.ptr, value.len)); }
void LuaToJsConverter::OnValue(LuaFunction value) { ctx_.results.emplace_back(runtime_.CreateJsProxyFunction(env_, value)); }
bool LuaToJsConverter::OnValue(LuaTable value) {
  auto [it, inserted] = ctx_.objects.try_emplace(value.identity, Napi::Object::New(env_));
  ctx_.results.emplace_back(it->second);
  return inserted;
}

void LuaToJsConverter::SetTable(LuaTable table) {
  auto it = ctx_.objects.find(table.identity);
  ctx_.current_object = it->second;
}
void LuaToJsConverter::OnProperty(LuaTableKey key, LuaNil _value) { SetProperty(key, env_.Null()); }
void LuaToJsConverter::OnProperty(LuaTableKey key, LuaBool value) { SetProperty(key, Napi::Boolean::New(env_, value.value)); }
void LuaToJsConverter::OnProperty(LuaTableKey key, LuaNumber value) { SetProperty(key, Napi::Number::New(env_, value.value)); }
void LuaToJsConverter::OnProperty(LuaTableKey key, LuaString value) { SetProperty(key, Napi::String::New(env_, value.ptr, value.len)); }
void LuaToJsConverter::OnProperty(LuaTableKey key, LuaFunction value) { SetProperty(key, runtime_.CreateJsProxyFunction(env_, value)); }
bool LuaToJsConverter::OnProperty(LuaTableKey key, LuaTable value) {
  auto [it, inserted] = ctx_.objects.try_emplace(value.identity, Napi::Object::New(env_));
  SetProperty(key, it->second);
  return inserted;
}

// Result

Napi::Value LuaToJsConverter::BuildResult() {
  size_t size = ctx_.results.size();

  if (size == 0) {
    return env_.Undefined();
  }

  if (size == 1) {
    return ctx_.results[0];
  }

  auto array = Napi::Array::New(env_, size);

  for (size_t i = 0; i < size; i++) {
    array.Set(i, ctx_.results[i]);
  }

  return array;
}

// Private

void LuaToJsConverter::SetProperty(LuaTableKey key, Napi::Value value) {
  std::visit(
    [&](auto&& k) {
      using T = std::decay_t<decltype(k)>;

      if constexpr (std::is_same_v<T, LuaString>) {
        ctx_.current_object.Set(Napi::String::New(env_, k.ptr, k.len), value);
      } else {
        ctx_.current_object.Set(k.value, value);
      }
    },
    key
  );
}
