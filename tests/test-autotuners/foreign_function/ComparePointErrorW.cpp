#include <falcon-typing/FFIHelpers.hpp>
#include <falcon-typing/FalconRuntime.hpp>

using namespace falcon::typing;
using namespace falcon::typing::ffi::wrapper;

extern "C" {

// CompareEqual(Point a, Point b) -> bool
void CompareEqual(const FalconParamEntry *params, int32_t param_count,
                  FalconResultSlot *out, int32_t *out_count) {

  auto pm = unpack_params(params, param_count);

  const auto &a = pm.at("a");
  const auto &b = pm.at("b");

  pack_single(a == b, out, out_count);
}

// CompareNotEqual(Point a, Point b) -> bool
void CompareNotEqual(const FalconParamEntry *params, int32_t param_count,
                     FalconResultSlot *out, int32_t *out_count) {

  auto pm = unpack_params(params, param_count);

  const auto &a = pm.at("a");
  const auto &b = pm.at("b");

  pack_single(a != b, out, out_count);
}
}
