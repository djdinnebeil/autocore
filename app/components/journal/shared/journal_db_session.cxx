module journal_db_session;

import std;
import auto_core.core.pipes;
import journal_db_protocol;

namespace {

std::optional<int> parse_int(const std::string_view text) {
    int value = 0;
    const auto parsed = std::from_chars(
        text.data(),
        text.data() + text.size(),
        value
    );
    if (parsed.ec != std::errc {} || parsed.ptr != text.data() + text.size()) {
        return std::nullopt;
    }
    return value;
}

std::expected<ac::pipes::Pipe, std::string> connect() {
    auto pipe = ac::pipes::connect_to_pipe_server(
        std::wstring {journal::db::pipe_name}
    );
    if (!pipe) {
        return std::unexpected("Journal database request failed.");
    }
    if (const auto sent = ac::pipes::send_string(*pipe, journal::db::protocol_id);
        !sent) {
        return std::unexpected("Journal database request failed.");
    }
    auto status = ac::pipes::read_string(*pipe);
    if (!status) {
        return std::unexpected("Journal database request failed.");
    }
    if (*status == "protocol") {
        (void)ac::pipes::read_string(*pipe);
        return std::unexpected("Journal database protocol mismatch.");
    }
    if (*status != "ok") {
        return std::unexpected("Journal database request failed.");
    }
    return std::move(*pipe);
}

std::expected<std::string, std::string> read_status(ac::pipes::Pipe& pipe) {
    auto status = ac::pipes::read_string(pipe);
    if (!status) {
        return std::unexpected("Journal database request failed.");
    }
    if (*status == "error") {
        auto message = ac::pipes::read_string(pipe);
        if (!message) {
            return std::unexpected("Journal database request failed.");
        }
        return std::unexpected(std::move(*message));
    }
    if (*status != "ok") {
        return std::unexpected("Journal database request failed.");
    }
    return std::move(*status);
}

std::expected<journal::db::Series, std::string> read_series(ac::pipes::Pipe& pipe) {
    auto name = ac::pipes::read_string(pipe);
    auto next_text = ac::pipes::read_string(pipe);
    auto padding_text = ac::pipes::read_string(pipe);
    if (!name || !next_text || !padding_text) {
        return std::unexpected("Journal database request failed.");
    }
    const auto next_episode = parse_int(*next_text);
    const auto padding = parse_int(*padding_text);
    if (!next_episode || !padding) {
        return std::unexpected("Journal database request failed.");
    }
    return journal::db::Series {std::move(*name), *next_episode, *padding};
}

} // namespace

namespace journal::db::session {

std::expected<void, std::string> probe() {
    auto pipe = connect();
    if (!pipe) {
        return std::unexpected(pipe.error());
    }
    return {};
}

std::expected<Episode, std::string> allocate_episode(const std::string_view series_key) {
    auto pipe = connect();
    if (!pipe) {
        return std::unexpected(pipe.error());
    }
    if (const auto sent = ac::pipes::send_pipe_command(
            *pipe,
            to_wire(Request::allocate_episode)
        );
        !sent) {
        return std::unexpected("Journal database request failed.");
    }
    if (const auto sent = ac::pipes::send_string(*pipe, series_key); !sent) {
        return std::unexpected("Journal database request failed.");
    }
    if (const auto status = read_status(*pipe); !status) {
        return std::unexpected(status.error());
    }
    auto name = ac::pipes::read_string(*pipe);
    auto allocated_text = ac::pipes::read_string(*pipe);
    auto next_text = ac::pipes::read_string(*pipe);
    auto padding_text = ac::pipes::read_string(*pipe);
    if (!name || !allocated_text || !next_text || !padding_text) {
        return std::unexpected("Journal database request failed.");
    }
    const auto allocated = parse_int(*allocated_text);
    const auto next_episode = parse_int(*next_text);
    const auto padding = parse_int(*padding_text);
    if (!allocated || !next_episode || !padding) {
        return std::unexpected("Journal database request failed.");
    }
    return Episode {std::move(*name), *allocated, *next_episode, *padding};
}

std::expected<std::vector<Series>, std::string> list_series() {
    auto pipe = connect();
    if (!pipe) {
        return std::unexpected(pipe.error());
    }
    if (const auto sent = ac::pipes::send_pipe_command(
            *pipe,
            to_wire(Request::list_series)
        );
        !sent) {
        return std::unexpected("Journal database request failed.");
    }
    if (const auto status = read_status(*pipe); !status) {
        return std::unexpected(status.error());
    }
    auto count_text = ac::pipes::read_string(*pipe);
    if (!count_text) {
        return std::unexpected("Journal database request failed.");
    }
    const auto count = parse_int(*count_text);
    if (!count || *count < 0) {
        return std::unexpected("Journal database request failed.");
    }
    std::vector<Series> series;
    series.reserve(static_cast<std::size_t>(*count));
    for (int index = 0; index < *count; ++index) {
        auto entry = read_series(*pipe);
        if (!entry) {
            return std::unexpected(entry.error());
        }
        series.push_back(std::move(*entry));
    }
    return series;
}

std::expected<void, std::string> add_series(
    const std::string_view name,
    const int padding
) {
    auto pipe = connect();
    if (!pipe) {
        return std::unexpected(pipe.error());
    }
    if (const auto sent = ac::pipes::send_pipe_command(*pipe, to_wire(Request::add_series));
        !sent) {
        return std::unexpected("Journal database request failed.");
    }
    if (const auto sent = ac::pipes::send_string(*pipe, name); !sent) {
        return std::unexpected("Journal database request failed.");
    }
    if (const auto sent = ac::pipes::send_string(*pipe, std::to_string(padding));
        !sent) {
        return std::unexpected("Journal database request failed.");
    }
    if (const auto status = read_status(*pipe); !status) {
        return std::unexpected(status.error());
    }
    return {};
}

std::expected<Series, std::string> set_counter(
    const std::string_view name,
    const int counter
) {
    auto pipe = connect();
    if (!pipe) {
        return std::unexpected(pipe.error());
    }
    if (const auto sent = ac::pipes::send_pipe_command(
            *pipe,
            to_wire(Request::set_counter)
        );
        !sent) {
        return std::unexpected("Journal database request failed.");
    }
    if (const auto sent = ac::pipes::send_string(*pipe, name); !sent) {
        return std::unexpected("Journal database request failed.");
    }
    if (const auto sent = ac::pipes::send_string(*pipe, std::to_string(counter));
        !sent) {
        return std::unexpected("Journal database request failed.");
    }
    if (const auto status = read_status(*pipe); !status) {
        return std::unexpected(status.error());
    }
    return read_series(*pipe);
}

std::expected<Series, std::string> set_padding(
    const std::string_view name,
    const int padding
) {
    auto pipe = connect();
    if (!pipe) {
        return std::unexpected(pipe.error());
    }
    if (const auto sent = ac::pipes::send_pipe_command(
            *pipe,
            to_wire(Request::set_padding)
        );
        !sent) {
        return std::unexpected("Journal database request failed.");
    }
    if (const auto sent = ac::pipes::send_string(*pipe, name); !sent) {
        return std::unexpected("Journal database request failed.");
    }
    if (const auto sent = ac::pipes::send_string(*pipe, std::to_string(padding));
        !sent) {
        return std::unexpected("Journal database request failed.");
    }
    if (const auto status = read_status(*pipe); !status) {
        return std::unexpected(status.error());
    }
    return read_series(*pipe);
}

std::expected<Series, std::string> find_series(const std::string_view series_key) {
    auto pipe = connect();
    if (!pipe) {
        return std::unexpected(pipe.error());
    }
    if (const auto sent = ac::pipes::send_pipe_command(
            *pipe,
            to_wire(Request::find_series)
        );
        !sent) {
        return std::unexpected("Journal database request failed.");
    }
    if (const auto sent = ac::pipes::send_string(*pipe, series_key); !sent) {
        return std::unexpected("Journal database request failed.");
    }
    if (const auto status = read_status(*pipe); !status) {
        return std::unexpected(status.error());
    }
    return read_series(*pipe);
}

std::expected<void, std::string> shutdown() {
    auto pipe = connect();
    if (!pipe) {
        return std::unexpected(pipe.error());
    }
    if (const auto sent = ac::pipes::send_pipe_command(*pipe, to_wire(Request::shutdown));
        !sent) {
        return std::unexpected("Journal database request failed.");
    }
    return {};
}

} // namespace journal::db::session
