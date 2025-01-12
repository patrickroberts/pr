#pragma once

#include <climits>
#include <cmath>
#include <format>

#if __cpp_lib_constexpr_cmath >= 202202L
#define PR_CONSTEXPR_CMATH constexpr
#else
#define PR_CONSTEXPR_CMATH static thread_local const
#endif

namespace pr {
namespace detail {

template <std::integral>
struct fast;

template <std::integral T>
using fast_t = fast<T>::type;

template <std::unsigned_integral T>
struct fast<T> : std::make_unsigned<fast_t<std::make_signed_t<T>>> {};

template <>
struct fast<std::int8_t> : std::type_identity<std::int_fast8_t> {};

template <>
struct fast<std::int16_t> : std::type_identity<std::int_fast16_t> {};

template <>
struct fast<std::int32_t> : std::type_identity<std::int_fast32_t> {};

template <>
struct fast<std::int64_t> : std::type_identity<std::int_fast64_t> {};

template <std::size_t BitWidthV>
inline constexpr std::size_t least_v{
    std::max<std::size_t>(CHAR_BIT, std::bit_ceil(BitWidthV))};

template <std::size_t BitWidthV>
struct int_least : int_least<least_v<BitWidthV>> {};

template <>
struct int_least<8> : std::type_identity<std::int8_t> {};
template <>
struct int_least<16> : std::type_identity<std::int16_t> {};
template <>
struct int_least<32> : std::type_identity<std::int32_t> {};
template <>
struct int_least<64> : std::type_identity<std::int64_t> {};

template <std::size_t BitWidthV>
using int_least_t = int_least<BitWidthV>::type;

template <std::size_t BitWidthV>
using uint_least_t = std::make_unsigned_t<int_least_t<BitWidthV>>;

template <std::size_t BitWidthV>
using int_fast_t = fast_t<int_least_t<BitWidthV>>;

template <std::size_t BitWidthV>
using uint_fast_t = fast_t<uint_least_t<BitWidthV>>;

template <std::size_t BitWidthV, std::size_t BitOffsetV>
inline constexpr uint_fast_t<BitWidthV + BitOffsetV> mask_v{
    ((1ULL << BitWidthV) - 1) << BitOffsetV};

template <std::size_t BitWidthV, std::size_t BitOffsetV>
inline constexpr auto to_integer =
    [](std::unsigned_integral auto repr) noexcept -> int_fast_t<BitWidthV + 1> {
  return static_cast<int_fast_t<BitWidthV + 1>>(
      (repr & mask_v<BitWidthV, BitOffsetV>) >> BitOffsetV);
};

inline constexpr std::int_fast8_t signs_v[]{1, -1};

template <std::size_t ExponentWidthV>
inline constexpr int_fast_t<ExponentWidthV + 1> bias_v{
    (1LL << (ExponentWidthV - 1)) - 1};

template <class T>
inline constexpr fast_t<int> exponent_width_v{
    std::bit_width<unsigned>(std::numeric_limits<T>::max_exponent)};

template <class T>
inline constexpr fast_t<int> fraction_width_v{std::numeric_limits<T>::digits -
                                              1};

template <class FromT, class ToT>
inline constexpr bool is_narrowing_v{
    exponent_width_v<FromT> > exponent_width_v<ToT> or
    fraction_width_v<FromT> > fraction_width_v<ToT>};

} // namespace detail

template <std::size_t ExponentWidthV, std::size_t FractionWidthV>
class fp final {
  static constexpr std::size_t size_bits =
      detail::least_v<1 + ExponentWidthV + FractionWidthV>;

  detail::uint_least_t<size_bits> storage;

public:
  template <std::floating_point ValueT>
  [[nodiscard]] constexpr explicit(detail::is_narrowing_v<fp, ValueT>)
  operator ValueT() const noexcept {
    constexpr auto to_sign_bit = detail::to_integer<1, size_bits - 1>;
    constexpr auto to_exponent =
        detail::to_integer<ExponentWidthV, size_bits - 1 - ExponentWidthV>;
    constexpr auto to_fraction =
        detail::to_integer<FractionWidthV,
                           size_bits - 1 - ExponentWidthV - FractionWidthV>;
    constexpr auto bias = detail::bias_v<ExponentWidthV>;

    const auto sign = detail::signs_v[to_sign_bit(storage)];
    const auto exponent = to_exponent(storage) - bias;
    const auto fraction = to_fraction(storage);

    constexpr auto is_subnormal = -bias;
    constexpr auto is_infinity_or_nan = 1 + bias;
    constexpr auto is_infinity = 0;
    constexpr auto divisor = 1LL << FractionWidthV;
    constexpr auto multiplier = ValueT(1) / divisor;
    PR_CONSTEXPR_CMATH auto subnormal_ldexp = std::ldexp(multiplier, 1 - bias);
    constexpr auto infinity = std::numeric_limits<ValueT>::infinity();
    constexpr auto nan = std::numeric_limits<ValueT>::quiet_NaN();

    switch (exponent) {
    default:
      [[likely]] return sign * (divisor + fraction) *
          std::ldexp(multiplier, exponent);
    case is_subnormal:
      return sign * fraction * subnormal_ldexp;
    case is_infinity_or_nan:
      switch (fraction) {
      case is_infinity:
        return sign * infinity;
      default:
        return nan;
      }
    }
  }
};

namespace detail {

template <std::size_t ExponentWidthV, std::size_t FractionWidthV>
inline constexpr fast_t<int>
    exponent_width_v<fp<ExponentWidthV, FractionWidthV>>{ExponentWidthV};

template <std::size_t ExponentWidthV, std::size_t FractionWidthV>
inline constexpr fast_t<int>
    fraction_width_v<fp<ExponentWidthV, FractionWidthV>>{FractionWidthV};

} // namespace detail

using fp4e2m1_t = fp<2, 1>;
using fp4e3m0_t = fp<3, 0>;
using fp8e4m3_t = fp<4, 3>;
using fp8e5m2_t = fp<5, 2>;
using fp16_t = fp<5, 10>;
using bf16_t = fp<8, 7>;
using tf32_t = fp<8, 10>;
using fp32_t = fp<8, 23>;
using fp64_t = fp<11, 52>;

} // namespace pr

template <std::size_t ExponentWidthV, std::size_t FractionWidthV, class CharT>
struct std::formatter<pr::fp<ExponentWidthV, FractionWidthV>, CharT>
    : std::formatter<double, CharT> {};
