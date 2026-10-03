/**
 * \file itunes_formatting_detail.hpp
 * \brief Pure text formatting used by iTunes component commands.
 */
#pragma once

#include "../shared/library_format_detail.hpp"

#include <string>
#include <string_view>

namespace itunes::formatting::detail {

    [[nodiscard]] std::string format_queue_item(
        std::string_view input,
        const itunes::library_format::detail::CompiledLibraryFormat& format,
        int column_count
    );

} // namespace itunes::formatting::detail
