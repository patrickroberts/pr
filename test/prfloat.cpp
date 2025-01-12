#include <pr/prfloat.hpp>

#include <ranges>
#include <stdfloat>

auto main() -> int {
#if __STDCPP_FLOAT16_T__ and __STDCPP_BFLOAT16_T__
  for (const auto bits : std::views::iota(std::uint16_t{}, 0xFFFFU)) {
    {
      const std::float32_t actual = std::bit_cast<pr::fp16_t>(bits);
      const std::float32_t expected = std::bit_cast<std::float16_t>(bits);

      if (actual != expected and
          (not std::isnan(actual) or not std::isnan(expected))) {
        return EXIT_FAILURE;
      }
    }
    {
      const std::float32_t actual = std::bit_cast<pr::bf16_t>(bits);
      const std::float32_t expected = std::bit_cast<std::bfloat16_t>(bits);

      if (actual != expected and
          (not std::isnan(actual) or not std::isnan(expected))) {
        return EXIT_FAILURE;
      }
    }
  }
#endif

  for (const auto bits : std::views::iota(std::uint32_t{}, 0xFFFFFFFFU)) {
    {
      const double actual = std::bit_cast<pr::fp32_t>(bits);
      const double expected = std::bit_cast<float>(bits);

      if (actual != expected and
          (not std::isnan(actual) or not std::isnan(expected))) {
        return EXIT_FAILURE;
      }
    }

    if ((0x1FFF & bits) == 0) {
      const double actual = std::bit_cast<pr::tf32_t>(bits);
      const double expected = std::bit_cast<float>(bits);

      if (actual != expected and
          (not std::isnan(actual) or not std::isnan(expected))) {
        return EXIT_FAILURE;
      }
    }
  }

  return EXIT_SUCCESS;
}
