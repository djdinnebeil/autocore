#include "catch_amalgamated.hpp"
#include "../shared/logger_merge_detail.hpp"

namespace detail = ac::logger::config::detail;

TEST_CASE("Logger interval accepts zero and sixty", "[logger-merge][unit]") {
    const auto disabled = detail::resolve_merge(
        {.merge_interval_seconds = "0"}
    );
    const auto standard = detail::resolve_merge(
        {.merge_interval_seconds = "60"}
    );

    CHECK(disabled.merge_interval_seconds == 0);
    CHECK(standard.merge_interval_seconds == 60);
}

TEST_CASE("Invalid logger intervals use sixty", "[logger-merge][unit]") {
    const auto missing = detail::resolve_merge({});
    const auto negative = detail::resolve_merge(
        {.merge_interval_seconds = "-1"}
    );
    const auto malformed = detail::resolve_merge(
        {.merge_interval_seconds = "soon"}
    );
    const auto trailing = detail::resolve_merge(
        {.merge_interval_seconds = "60s"}
    );

    CHECK(missing.merge_interval_seconds == 60);
    CHECK(negative.merge_interval_seconds == 60);
    CHECK(malformed.merge_interval_seconds == 60);
    CHECK(trailing.merge_interval_seconds == 60);
    CHECK(missing.report.find("merge_interval_seconds missing or invalid") !=
        std::string::npos);
}

TEST_CASE("Logger shutdown merge accepts only on and off", "[logger-merge][unit]") {
    const auto missing = detail::resolve_merge({});
    const auto on = detail::resolve_merge(
        {.merge_logs_on_shutdown = "on"}
    );
    const auto off = detail::resolve_merge(
        {.merge_logs_on_shutdown = "off"}
    );
    const auto unrecognized_on = detail::resolve_merge(
        {.merge_logs_on_shutdown = "true"}
    );
    const auto unrecognized_off = detail::resolve_merge(
        {.merge_logs_on_shutdown = "false"}
    );
    const auto invalid = detail::resolve_merge(
        {.merge_logs_on_shutdown = "yes"}
    );

    CHECK(missing.merge_logs_on_shutdown);
    CHECK(on.merge_logs_on_shutdown);
    CHECK_FALSE(off.merge_logs_on_shutdown);
    CHECK(unrecognized_on.merge_logs_on_shutdown);
    CHECK(unrecognized_off.merge_logs_on_shutdown);
    CHECK(invalid.merge_logs_on_shutdown);
    CHECK(unrecognized_on.report.find(
        "merge_logs_on_shutdown missing or invalid") != std::string::npos);
    CHECK(invalid.report.find("merge_logs_on_shutdown missing or invalid") !=
        std::string::npos);
}
