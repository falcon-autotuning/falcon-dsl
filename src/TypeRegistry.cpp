#include "falcon-dsl/TypeRegistry.hpp"
#include <algorithm>
#include <falcon-typing/PrimitiveTypes.hpp>
#include <stdexcept>
#include <string>
#include <unordered_set>

namespace falcon::dsl {

std::shared_ptr<TypeRegistry> TypeRegistry::create_default() {
  // The default registry no longer hard-codes any built-in Array methods.
  // Array<T> is now a first-class generic struct defined in
  // libs/collections/array/array.fal with FFI-backed routines in
  // array-wrapper.cpp.  The TypeRegistry for arrays is populated at
  // load-time when the user imports that library (or when falconCore
  // loads its wrappers, which still use ArrayValue directly for internal
  // data transfer).
  //
  // Map<K,V> is similarly defined in libs/collections/map/map.fal.
  return std::make_shared<TypeRegistry>();
}

void TypeRegistry::register_method(const std::string &type_name,
                                   const std::string &method_name,
                                   typing::TypeMethod method) {
  type_methods_[type_name][method_name] = std::move(method);
}

void TypeRegistry::register_ffi_method(const std::string &type_name,
                                       const std::string &method_name,
                                       typing::TypeMethod method) {
  ffi_methods_[type_name][method_name] = std::move(method);
}

const typing::TypeMethod *
TypeRegistry::lookup_method(const std::string &type_name,
                            const std::string &method_name) const {
  // First, try the exact type name
  auto type_it = type_methods_.find(type_name);
  if (type_it != type_methods_.end()) {
    auto method_it = type_it->second.find(method_name);
    if (method_it != type_it->second.end()) {
      return &method_it->second;
    }
  }
  // If not found, check for generic type (e.g., Array[int] -> Array)
  auto lbracket = type_name.find('[');
  if (lbracket != std::string::npos) {
    std::string generic_type = type_name.substr(0, lbracket);
    auto gen_it = type_methods_.find(generic_type);
    if (gen_it != type_methods_.end()) {
      auto method_it = gen_it->second.find(method_name);
      if (method_it != gen_it->second.end()) {
        return &method_it->second;
      }
    }
  }
  return nullptr;
}

const typing::TypeMethod *
TypeRegistry::lookup_ffi_method(const std::string &type_name,
                                const std::string &method_name) const {
  // First, try the exact type name
  auto type_it = ffi_methods_.find(type_name);
  if (type_it != ffi_methods_.end()) {
    auto method_it = type_it->second.find(method_name);
    if (method_it != type_it->second.end()) {
      return &method_it->second;
    }
  }
  // If not found, check for generic type (e.g., Array<int> -> Array)
  auto angle = type_name.find('<');
  if (angle != std::string::npos) {
    std::string generic_type = type_name.substr(0, angle);
    auto gen_it = ffi_methods_.find(generic_type);
    if (gen_it != ffi_methods_.end()) {
      auto method_it = gen_it->second.find(method_name);
      if (method_it != gen_it->second.end()) {
        return &method_it->second;
      }
    }
  }
  return nullptr;
}

std::optional<bool> TypeRegistry::invoke_comparison_override(
    const std::shared_ptr<typing::StructInstance> &lhs,
    const std::shared_ptr<typing::StructInstance> &rhs,
    const std::string &method_name) const {

  const typing::TypeMethod *method =
      lookup_ffi_method(lhs->type_name, method_name);

  if (method == nullptr) {
    return std::nullopt;
  }

  typing::ParameterMap params;
  params["this"] = lhs;
  params["other"] = rhs;

  auto result = (*method)(lhs, params);

  if (result.size() != 1) {
    throw std::runtime_error(method_name + " must return exactly one value");
  }

  if (!std::holds_alternative<bool>(result.front())) {
    throw std::runtime_error(method_name + " must return bool");
  }

  return std::get<bool>(result.front());
}

std::optional<bool> TypeRegistry::invoke_comparison(
    const std::shared_ptr<typing::StructInstance> &lhs,
    const std::shared_ptr<typing::StructInstance> &rhs, ComparisonOp op) const {

  if (auto result = invoke_comparison_override(
          lhs, rhs, std::string(primary_method_name(op)))) {
    return result;
  }

  if (auto result = invoke_comparison_override(
          lhs, rhs, std::string(inverse_method_name(op)))) {
    return !result;
  }

  return std::nullopt;
}

struct TypeRegistry::CycleGuard {
  std::unordered_map<const void *, falcon::typing::RuntimeValue> copies;
  std::unordered_set<std::pair<const void *, const void *>,
                     boost::hash<std::pair<const void *, const void *>>>
      pairs;

  const falcon::typing::RuntimeValue *find_copy(const void *ptr) const {
    auto iter = copies.find(ptr);
    return iter != copies.end() ? &iter->second : nullptr;
  }

  void record_copy(const void *ptr, falcon::typing::RuntimeValue copy) {
    copies.emplace(ptr, std::move(copy));
  }

  bool enter_pair(const void *lhs, const void *rhs) {
    return pairs.emplace(lhs, rhs).second;
  }
};

bool TypeRegistry::values_equal(const std::shared_ptr<typing::TupleValue> &lhs,
                                const std::shared_ptr<typing::TupleValue> &rhs,
                                CycleGuard &guard) const {
  if (!lhs || !rhs) {
    return lhs == rhs;
  }
  if (lhs.get() == rhs.get()) {
    return true;
  }
  if (!guard.enter_pair(lhs.get(), rhs.get())) {
    return true;
  }
  if (lhs->values.size() != rhs->values.size()) {
    return false;
  }
  for (size_t i = 0; i < lhs->values.size(); ++i) {
    if (!runtime_equals_impl(lhs->values.at(i), rhs->values.at(i), guard)) {
      return false;
    }
  }

  return true;
}

bool TypeRegistry::values_not_equal(
    const std::shared_ptr<typing::TupleValue> &lhs,
    const std::shared_ptr<typing::TupleValue> &rhs, CycleGuard &guard) const {

  if (!lhs || !rhs) {
    return lhs != rhs;
  }
  if (!guard.enter_pair(lhs.get(), rhs.get())) {
    return false;
  }

  if (lhs->values.size() != rhs->values.size()) {
    return true;
  }

  for (size_t i = 0; i < lhs->values.size(); ++i) {
    if (runtime_not_equals_impl(lhs->values.at(i), rhs->values.at(i), guard)) {
      return true;
    }
  }

  return false;
}

bool TypeRegistry::values_equal(const std::shared_ptr<typing::ArrayValue> &lhs,
                                const std::shared_ptr<typing::ArrayValue> &rhs,
                                CycleGuard &guard) const {

  if (!lhs || !rhs) {
    return lhs == rhs;
  }
  if (lhs.get() == rhs.get()) {
    return true;
  }
  if (!guard.enter_pair(lhs.get(), rhs.get())) {
    return true;
  }
  if (lhs->element_type_name != rhs->element_type_name ||
      lhs->elements.size() != rhs->elements.size()) {
    return false;
  }

  for (size_t i = 0; i < lhs->elements.size(); ++i) {
    if (!runtime_equals_impl(lhs->elements.at(i), rhs->elements.at(i), guard)) {
      return false;
    }
  }
  return true;
}

bool TypeRegistry::values_not_equal(
    const std::shared_ptr<typing::ArrayValue> &lhs,
    const std::shared_ptr<typing::ArrayValue> &rhs, CycleGuard &guard) const {

  if (!lhs || !rhs) {
    return lhs != rhs;
  }
  if (!guard.enter_pair(lhs.get(), rhs.get())) {
    return false;
  }
  if (lhs->element_type_name != rhs->element_type_name) {
    return true;
  }
  if (lhs->elements.size() != rhs->elements.size()) {
    return true;
  }
  for (size_t i = 0; i < lhs->elements.size(); ++i) {
    if (runtime_not_equals_impl(lhs->elements.at(i), rhs->elements.at(i),
                                guard)) {
      return true;
    }
  }

  return false;
}

bool TypeRegistry::values_equal(
    const std::shared_ptr<typing::StructInstance> &lhs,
    const std::shared_ptr<typing::StructInstance> &rhs,
    CycleGuard &guard) const {

  if (!lhs || !rhs) {
    return lhs == rhs;
  }

  if (lhs->type_name != rhs->type_name) {
    return false;
  }

  if (!guard.enter_pair(lhs.get(), rhs.get())) {
    return true;
  }

  if (auto result = invoke_comparison(lhs, rhs, ComparisonOp::Equals)) {
    return *result;
  }

  if (lhs->is_native()) {
    throw std::runtime_error(
        "Native type '" + lhs->type_name +
        "' requires Equal or NotEqual defined in FFI library");
  }
  if (rhs->is_native()) {
    throw std::runtime_error(
        "Native type '" + rhs->type_name +
        "' requires Equal or NotEqual defined in FFI library");
  }

  const auto &lhs_fields = *lhs->fields;
  const auto &rhs_fields = *rhs->fields;

  if (lhs_fields.size() != rhs_fields.size()) {
    return false;
  }

  return std::ranges::all_of(lhs_fields, [&](const auto &entry) {
    const auto &[name, lhs_value] = entry;
    auto rhs_it = rhs_fields.find(name);
    return rhs_it != rhs_fields.end() &&
           runtime_equals_impl(lhs_value, rhs_it->second, guard);
  });
}

bool TypeRegistry::values_not_equal(
    const std::shared_ptr<typing::StructInstance> &lhs,
    const std::shared_ptr<typing::StructInstance> &rhs,
    CycleGuard &guard) const {

  if (!lhs || !rhs) {
    return lhs != rhs;
  }

  if (lhs->type_name != rhs->type_name) {
    return true;
  }

  if (!guard.enter_pair(lhs.get(), rhs.get())) {
    return false;
  }

  if (auto result = invoke_comparison(lhs, rhs, ComparisonOp::NotEquals)) {
    return *result;
  }

  if (auto result = invoke_comparison(lhs, rhs, ComparisonOp::Equals)) {
    return !(*result);
  }

  if (lhs->is_native()) {
    throw std::runtime_error(
        "Native type '" + lhs->type_name +
        "' requires Equal or NotEqual defined in FFI library");
  }
  if (rhs->is_native()) {
    throw std::runtime_error(
        "Native type '" + rhs->type_name +
        "' requires Equal or NotEqual defined in FFI library");
  }

  const auto &lhs_fields = *lhs->fields;
  const auto &rhs_fields = *rhs->fields;

  if (lhs_fields.size() != rhs_fields.size()) {
    return true;
  }

  return std::ranges::any_of(lhs_fields, [&](const auto &entry) {
    const auto &[name, lhs_value] = entry;
    auto rhs_it = rhs_fields.find(name);
    return rhs_it == rhs_fields.end() ||
           runtime_not_equals_impl(lhs_value, rhs_it->second, guard);
  });
}

namespace {
template <typename> inline constexpr bool is_shared_ptr_v = false;

template <typename U>
inline constexpr bool is_shared_ptr_v<std::shared_ptr<U>> = true;

} // namespace

bool TypeRegistry::runtime_equals(const typing::RuntimeValue &lhs,
                                  const typing::RuntimeValue &rhs) const {
  CycleGuard guard;
  return runtime_equals_impl(lhs, rhs, guard);
}
bool TypeRegistry::runtime_equals_impl(const typing::RuntimeValue &lhs,
                                       const typing::RuntimeValue &rhs,
                                       CycleGuard &guard) const {
  if (lhs.index() != rhs.index()) {
    return false;
  }

  return std::visit(
      [&](const auto &lhs_val) -> bool {
        using T = std::decay_t<decltype(lhs_val)>;
        const auto &rhs_val = std::get<T>(rhs);

        if constexpr (std::is_same_v<T, std::nullptr_t>) {
          return true;
        } else if constexpr (
            std::is_same_v<T, std::shared_ptr<typing::TupleValue>> ||
            std::is_same_v<T, std::shared_ptr<typing::StructInstance>> ||
            std::is_same_v<T, std::shared_ptr<typing::ArrayValue>>) {
          return values_equal(lhs_val, rhs_val, guard);
        } else {
          static_assert(!is_shared_ptr_v<T>,
                        "Unhandled std::shared_ptr type in runtime_equals");
          return lhs_val == rhs_val;
        }
      },
      lhs);
}

bool TypeRegistry::runtime_not_equals(const typing::RuntimeValue &lhs,
                                      const typing::RuntimeValue &rhs) const {
  CycleGuard guard;
  return runtime_not_equals_impl(lhs, rhs, guard);
}
bool TypeRegistry::runtime_not_equals_impl(const typing::RuntimeValue &lhs,
                                           const typing::RuntimeValue &rhs,
                                           CycleGuard &guard) const {
  if (lhs.index() != rhs.index()) {
    return true;
  }

  return std::visit(
      [&](const auto &lhs_val) -> bool {
        using T = std::decay_t<decltype(lhs_val)>;
        const auto &rhs_val = std::get<T>(rhs);

        if constexpr (std::is_same_v<T, std::nullptr_t>) {
          return false;
        } else if constexpr (
            std::is_same_v<T, std::shared_ptr<typing::TupleValue>> ||
            std::is_same_v<T, std::shared_ptr<typing::StructInstance>> ||
            std::is_same_v<T, std::shared_ptr<typing::ArrayValue>>) {
          return values_not_equal(lhs_val, rhs_val, guard);
        } else {
          static_assert(!is_shared_ptr_v<T>,
                        "Unhandled std::shared_ptr type in runtime_not_equals");
          return lhs_val != rhs_val;
        }
      },
      lhs);
}

namespace {
template <typename T>
inline constexpr bool is_numeric_v =
    std::is_same_v<T, int64_t> || std::is_same_v<T, double>;
} // namespace

bool TypeRegistry::runtime_less_than(const typing::RuntimeValue &lhs,
                                     const typing::RuntimeValue &rhs) const {
  return std::visit(
      [&](const auto &l, const auto &r) -> bool {
        using L = std::decay_t<decltype(l)>;
        using R = std::decay_t<decltype(r)>;

        // Struct overrides
        if constexpr (std::is_same_v<L,
                                     std::shared_ptr<typing::StructInstance>> &&
                      std::is_same_v<R,
                                     std::shared_ptr<typing::StructInstance>>) {
          if (!l || !r) {
            throw std::runtime_error("Cannot order nil struct instances");
          }
          if (l->type_name != r->type_name) {
            throw std::runtime_error("Cannot compare different struct types");
          }
          if (auto result = invoke_comparison_override(l, r, "LessThan")) {
            return *result;
          }
          if (auto result = invoke_comparison_override(l, r, "GreaterThan")) {
            return !(*result) && !runtime_equals(lhs, rhs);
          }
          throw std::runtime_error("Type '" + l->type_name +
                                   "' does not define LessThan or GreaterThan");

          // Same-type int64_t (exact precision) or std::string
        } else if constexpr ((std::is_same_v<L, int64_t> &&
                              std::is_same_v<R, int64_t>) ||
                             (std::is_same_v<L, std::string> &&
                              std::is_same_v<R, std::string>)) {
          return l < r;

          // Mixed numeric promotion (int64_t <-> double, or double <-> double)
        } else if constexpr (is_numeric_v<L> && is_numeric_v<R>) {
          return static_cast<double>(l) < static_cast<double>(r);

          // All unsupported type combinations
        } else {
          throw std::runtime_error("Operator '<' unsupported for types '" +
                                   get_runtime_type_name(lhs) + "' and '" +
                                   get_runtime_type_name(rhs) + "'");
        }
      },
      lhs, rhs);
}

bool TypeRegistry::runtime_greater_than(const typing::RuntimeValue &lhs,
                                        const typing::RuntimeValue &rhs) const {
  return std::visit(
      [&](const auto &l, const auto &r) -> bool {
        using L = std::decay_t<decltype(l)>;
        using R = std::decay_t<decltype(r)>;

        // Struct overrides
        if constexpr (std::is_same_v<L,
                                     std::shared_ptr<typing::StructInstance>> &&
                      std::is_same_v<R,
                                     std::shared_ptr<typing::StructInstance>>) {
          if (!l || !r) {
            throw std::runtime_error("Cannot order nil struct instances");
          }
          if (l->type_name != r->type_name) {
            throw std::runtime_error("Cannot compare different struct types");
          }
          // Explicit GreaterThan wins
          if (auto result = invoke_comparison_override(l, r, "GreaterThan")) {
            return *result;
          }
          // Otherwise derive from LessThan
          if (auto result = invoke_comparison_override(l, r, "LessThan")) {
            return !(*result) && !runtime_equals(lhs, rhs);
          }
          throw std::runtime_error("Type '" + l->type_name +
                                   "' does not define LessThan or GreaterThan");

          // Same-type int64_t (exact precision) or std::string
        } else if constexpr ((std::is_same_v<L, int64_t> &&
                              std::is_same_v<R, int64_t>) ||
                             (std::is_same_v<L, std::string> &&
                              std::is_same_v<R, std::string>)) {
          return l > r;

          // Mixed numeric promotion (int64_t <-> double, or double <-> double)
        } else if constexpr (is_numeric_v<L> && is_numeric_v<R>) {
          return static_cast<double>(l) > static_cast<double>(r);

          // All unsupported type combinations
        } else {
          throw std::runtime_error("Operator '>' unsupported for types '" +
                                   get_runtime_type_name(lhs) + "' and '" +
                                   get_runtime_type_name(rhs) + "'");
        }
      },
      lhs, rhs);
}

typing::RuntimeValue
TypeRegistry::runtime_deep_copy(const typing::RuntimeValue &value) const {
  CycleGuard guard;
  return runtime_deep_copy_impl(value, guard);
}

typing::RuntimeValue
TypeRegistry::runtime_deep_copy_impl(const typing::RuntimeValue &value,
                                     CycleGuard &guard) const {
  return std::visit(
      [&](const auto &v) -> typing::RuntimeValue {
        using T = std::decay_t<decltype(v)>;

        // -------------------------------------------------------------------
        // StructInstance handling
        // -------------------------------------------------------------------
        if constexpr (std::is_same_v<T,
                                     std::shared_ptr<typing::StructInstance>>) {
          if (!v) {
            return std::shared_ptr<typing::StructInstance>{};
          }

          if (const auto *existing = guard.find_copy(v.get())) {
            return *existing;
          }

          // FFI override
          if (const typing::TypeMethod *method =
                  lookup_ffi_method(v->type_name, "DeepCopy")) {
            typing::ParameterMap params;
            typing::FunctionResult result = (*method)(v, params);

            if (result.size() != 1) {
              throw std::runtime_error(
                  "DeepCopy must return exactly one value");
            }
            guard.record_copy(v.get(), result.front());
            return result.front();
          }

          if (v->is_native()) {
            throw std::runtime_error(
                "Native type '" + v->type_name +
                "' requires FFI implementation of DeepCopy");
          }

          // Structural fallback: register empty copy BEFORE recursing on fields
          auto copy = std::make_shared<typing::StructInstance>(v->type_name);
          guard.record_copy(v.get(), copy);

          for (const auto &[name, field_value] : *v->fields) {
            copy->set_field(name, runtime_deep_copy_impl(field_value, guard));
          }
          return copy;

          // -------------------------------------------------------------------
          // TupleValue
          // -------------------------------------------------------------------
        } else if constexpr (std::is_same_v<
                                 T, std::shared_ptr<typing::TupleValue>>) {
          if (!v) {
            return std::shared_ptr<typing::TupleValue>{};
          }

          if (const auto *existing = guard.find_copy(v.get())) {
            return *existing;
          }

          // Pre-register BEFORE recursing so direct TupleValue cycles terminate
          auto copy = std::make_shared<typing::TupleValue>();
          guard.record_copy(v.get(), copy);

          copy->values.reserve(v->values.size());
          for (const auto &element : v->values) {
            copy->values.push_back(runtime_deep_copy_impl(element, guard));
          }
          return copy;

          // -------------------------------------------------------------------
          // ArrayValue support
          // -------------------------------------------------------------------
        } else if constexpr (std::is_same_v<
                                 T, std::shared_ptr<typing::ArrayValue>>) {
          if (!v) {
            return std::shared_ptr<typing::ArrayValue>{};
          }

          if (const auto *existing = guard.find_copy(v.get())) {
            return *existing;
          }

          // Register empty array copy BEFORE recursing on elements
          auto copy =
              std::make_shared<typing::ArrayValue>(v->element_type_name);
          guard.record_copy(v.get(), copy);

          copy->elements.reserve(v->elements.size());
          for (const auto &element : v->elements) {
            copy->elements.push_back(runtime_deep_copy_impl(element, guard));
          }
          return copy;

          // -------------------------------------------------------------------
          // Value / primitive fallback (int64_t, double, bool, string,
          // nullptr_t, ErrorObject)
          // -------------------------------------------------------------------
        } else {
          static_assert(!is_shared_ptr_v<T>,
                        "Unhandled std::shared_ptr type in runtime_deep_copy");
          return v;
        }
      },
      value);
}

} // namespace falcon::dsl
