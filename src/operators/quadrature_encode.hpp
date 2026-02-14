#pragma once

#include <core_types.hpp>
#include <operators/cache.hpp>
#include <operators/combine.hpp>
#include <operators/flat_map.hpp>
#include <operators/scan.hpp>

namespace rheo::operators {

  namespace detail {
    static const int8_t encoder_states_lookup[] = { 0, -1, 1, 0, 1, 0, 0, -1, -1, 0, 0, 1, 0, 1, -1, 0 };
  }

  enum QuadratureEncodeDirection {
    Clockwise = 1,
    CounterClockwise = -1,
  };

  template <typename SourceAT, typename SourceBT>
    requires concepts::Source<SourceAT> && concepts::Source<SourceBT>
      && std::is_same_v<bool, typename SourceAT::value_type> && std::is_same_v<bool, typename SourceBT::value_type>
  RHEO_CALLABLE auto quadrature_encode(SourceAT source_a, SourceBT source_b) {
    using source_a_type = typename SourceAT::value_type;
    using source_b_type = typename SourceBT::value_type;
    using acc_type = std::tuple<int8_t, int8_t, uint8_t>;

    return combine(std::move(source_a), std::move(source_b))
      | scan(
        acc_type(0, 0, 3),
        [](acc_type acc, std::tuple<source_a_type, source_b_type> value) {
          int8_t encoder_quarter_clicks = std::get<0>(acc);
          int8_t encoder_click = 0;
          uint8_t last_encoder_state = std::get<2>(acc);
          bool a = static_cast<bool>(std::get<0>(value));
          bool b = static_cast<bool>(std::get<1>(value));

          uint8_t current_state = (a << 1) | b;
          uint8_t combined = ((last_encoder_state & 0x03) << 2) | current_state;
          last_encoder_state = current_state;
          encoder_quarter_clicks += detail::encoder_states_lookup[combined];
          if (encoder_quarter_clicks > 3) {
            encoder_click = 1;
            encoder_quarter_clicks = 0;
          } else if (encoder_quarter_clicks < -3) {
            encoder_click = -1;
            encoder_quarter_clicks = 0;
          }

          return acc_type(encoder_quarter_clicks, encoder_click, last_encoder_state);
        }
      )
      | flat_map([](acc_type v) {
        int8_t clicks = std::get<1>(v);
        if (clicks) {
          return std::vector<QuadratureEncodeDirection>(
            abs(clicks),
            clicks > 0
              ? QuadratureEncodeDirection::Clockwise
              : QuadratureEncodeDirection::CounterClockwise
          );
        } else {
          return std::vector<QuadratureEncodeDirection>();
        }
      });
  }

}