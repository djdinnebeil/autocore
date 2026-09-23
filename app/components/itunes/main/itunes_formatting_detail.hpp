/**
 * \file itunes_formatting_detail.hpp
 * \brief Pure text formatting used by iTunes component commands.
 */
#pragma once

#include <string>
#include <string_view>

namespace itunes::formatting::detail {

    [[nodiscard]] std::string format_queue_item(
        std::string_view input,
        int tab_end
    );

} // namespace itunes::formatting::detail
