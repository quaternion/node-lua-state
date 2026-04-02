#pragma once

#include <napi.h>
#include <string_view>

template <size_t N> struct NapiStringBuffer {
  char buf[N];
  size_t len;

  inline bool TryFastString(napi_env env, napi_value value) {
    napi_status status = napi_get_value_string_utf8(env, value, buf, N, &len);
    NAPI_THROW_IF_FAILED(env, status, "");

    if (len >= N) {
      return false;
    }

    return true;
  }

  inline std::string_view GetFastString() { return std::string_view({buf, len}); }

  inline std::string GetSlowString(napi_env env, napi_value value) {
    std::string result;

    result.reserve(len + 1);
    result.resize(len);

    napi_status status = napi_get_value_string_utf8(env, value, &result[0], result.capacity(), nullptr);

    NAPI_THROW_IF_FAILED(env, status, "");

    return result;
  }
};