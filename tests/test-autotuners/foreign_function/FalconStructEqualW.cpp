#include <falcon-typing/FFIHelpers.hpp>

using namespace falcon::typing;
using namespace falcon::typing::ffi::wrapper;

extern "C" {

void STRUCTPointEqual(const FalconParamEntry *params, int32_t count,
                      FalconResultSlot *out, int32_t *out_count) {

  auto pm = unpack_params(params, count);

  auto lhs = std::get<std::shared_ptr<StructInstance>>(pm.at("this"));

  auto rhs = std::get<std::shared_ptr<StructInstance>>(pm.at("other"));

  bool equal = std::get<int64_t>(lhs->get_field("x")) ==
                   std::get<int64_t>(rhs->get_field("x")) &&
               std::get<int64_t>(lhs->get_field("y")) ==
                   std::get<int64_t>(rhs->get_field("y"));

  pack_single(equal, out, out_count);
}
}
