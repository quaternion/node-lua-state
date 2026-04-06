#pragma once

#include <napi.h>
#include <string_view>

enum class NapiStringHint { Key, Value };

template <size_t N> struct NapiStringBuffer {
  char buf[N];
  size_t len;

  inline bool TryFastStringKey(napi_env env, napi_value value) {
    napi_status status = napi_get_value_string_utf8(env, value, buf, N, &len);
    NAPI_THROW_IF_FAILED(env, status, "");

    if (len >= N) {
      return false;
    }

    return true;
  }

  inline bool TryFastStringValue(napi_env env, napi_value value) {
    napi_status status = napi_get_value_string_utf8(env, value, nullptr, 0, &len);
    NAPI_THROW_IF_FAILED(env, status, "");

    if (len >= N) {
      return false;
    }

    status = napi_get_value_string_utf8(env, value, buf, N, nullptr);
    NAPI_THROW_IF_FAILED(env, status, "");

    return true;
  }

  inline std::string_view GetFastString() { return std::string_view({buf, len}); }

  inline std::string GetSlowString(napi_env env, napi_value value) {
    std::string result;
    result.resize(len);
    napi_status status = napi_get_value_string_utf8(env, value, result.data(), len + 1, nullptr);
    NAPI_THROW_IF_FAILED(env, status, "");
    return result;
  }
};