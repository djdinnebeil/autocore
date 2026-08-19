#include "catch_amalgamated.hpp"

import auto_core.logging.protocol;

namespace {

    constexpr std::size_t header_size = 15;

    void write_uint32(
        std::string& frame,
        const std::size_t offset,
        const std::uint32_t value
    ) {
        for (unsigned int index = 0; index < 4; ++index) {
            frame[offset + index] = static_cast<char>(
                (value >> (index * 8)) & 0xffu
            );
        }
    }

    std::string valid_frame() {
        const auto encoded = ac::logging::encode(ac::logging::Event {});
        REQUIRE(encoded.has_value());
        return *encoded;
    }

} // namespace

TEST_CASE("Log events have stable version 1 wire bytes", "[log_protocol][unit]") {
    SECTION("message event") {
        const ac::logging::Event event {
            .type = ac::logging::EventType::message,
            .component = "core",
            .message = "ready",
            .newline = true
        };

        const auto encoded = ac::logging::encode(event);
        REQUIRE(encoded.has_value());

        const std::string expected {
            'A', 'C', 'L', 'G',
            '\x01',
            '\x00',
            '\x01',
            '\x04', '\x00', '\x00', '\x00',
            '\x05', '\x00', '\x00', '\x00',
            'c', 'o', 'r', 'e',
            'r', 'e', 'a', 'd', 'y'
        };

        CHECK(*encoded == expected);
    }

    SECTION("shutdown event") {
        const ac::logging::Event event {
            .type = ac::logging::EventType::shutdown,
            .newline = false
        };

        const auto encoded = ac::logging::encode(event);
        REQUIRE(encoded.has_value());

        const std::string expected {
            'A', 'C', 'L', 'G',
            '\x01',
            '\x01',
            '\x00',
            '\x00', '\x00', '\x00', '\x00',
            '\x00', '\x00', '\x00', '\x00'
        };

        CHECK(*encoded == expected);
    }
}

TEST_CASE("Every event type and newline state round-trips", "[log_protocol][unit]") {
    for (const auto type : {
        ac::logging::EventType::message,
        ac::logging::EventType::shutdown
    }) {
        for (const bool newline : {false, true}) {
            const ac::logging::Event original {
                .type = type,
                .component = std::string {'a', '\0', 'b'},
                .message = std::string {'x', '\0', 'y'},
                .newline = newline
            };

            const auto encoded = ac::logging::encode(original);
            REQUIRE(encoded.has_value());

            const auto decoded = ac::logging::decode(*encoded);
            REQUIRE(decoded.has_value());
            CHECK(decoded->type == original.type);
            CHECK(decoded->component == original.component);
            CHECK(decoded->message == original.message);
            CHECK(decoded->newline == original.newline);
        }
    }
}

TEST_CASE("Empty event fields round-trip", "[log_protocol][unit]") {
    const ac::logging::Event original {};
    const auto encoded = ac::logging::encode(original);
    REQUIRE(encoded.has_value());
    CHECK(encoded->size() == header_size);

    const auto decoded = ac::logging::decode(*encoded);
    REQUIRE(decoded.has_value());
    CHECK(decoded->component.empty());
    CHECK(decoded->message.empty());
}

TEST_CASE("Payload bytes are not text-validated", "[log_protocol][unit]") {
    const ac::logging::Event original {
        .component = std::string {static_cast<char>(0xff)},
        .message = std::string {
            static_cast<char>(0xc0),
            static_cast<char>(0xaf)
        }
    };

    const auto encoded = ac::logging::encode(original);
    REQUIRE(encoded.has_value());

    const auto decoded = ac::logging::decode(*encoded);
    REQUIRE(decoded.has_value());
    CHECK(decoded->component == original.component);
    CHECK(decoded->message == original.message);
}

TEST_CASE("The maximum frame size is accepted", "[log_protocol][unit]") {
    ac::logging::Event event {
        .component = "component",
        .message = std::string(
            ac::logging::maximum_encoded_size - header_size - 9,
            'x'
        )
    };

    const auto encoded = ac::logging::encode(event);
    REQUIRE(encoded.has_value());
    CHECK(encoded->size() == ac::logging::maximum_encoded_size);

    const auto decoded = ac::logging::decode(*encoded);
    REQUIRE(decoded.has_value());
    CHECK(decoded->component == event.component);
    CHECK(decoded->message == event.message);
}

TEST_CASE("Oversized frames are rejected", "[log_protocol][unit]") {
    const ac::logging::Event event {
        .message = std::string(
            ac::logging::maximum_encoded_size - header_size + 1,
            'x'
        )
    };

    const auto encoded = ac::logging::encode(event);
    REQUIRE_FALSE(encoded.has_value());
    CHECK(encoded.error() == ac::logging::ProtocolError::frame_too_large);

    const std::string oversized(
        ac::logging::maximum_encoded_size + 1,
        '\0'
    );
    const auto decoded = ac::logging::decode(oversized);
    REQUIRE_FALSE(decoded.has_value());
    CHECK(decoded.error() == ac::logging::ProtocolError::frame_too_large);
}

TEST_CASE("Encoding rejects unknown event types", "[log_protocol][unit]") {
    const ac::logging::Event event {
        .type = static_cast<ac::logging::EventType>(0xff)
    };

    const auto result = ac::logging::encode(event);
    REQUIRE_FALSE(result.has_value());
    CHECK(result.error() == ac::logging::ProtocolError::invalid_event_type);
}

TEST_CASE("Frames shorter than the header are malformed", "[log_protocol][unit]") {
    for (std::size_t size = 0; size < header_size; ++size) {
        INFO("frame size: " << size);
        const auto result = ac::logging::decode(std::string(size, '\0'));
        REQUIRE_FALSE(result.has_value());
        CHECK(result.error() == ac::logging::ProtocolError::malformed_frame);
    }
}

TEST_CASE("Decoder validates fixed header fields", "[log_protocol][unit]") {
    SECTION("magic") {
        std::string frame = valid_frame();
        frame[0] = 'X';

        const auto result = ac::logging::decode(frame);
        REQUIRE_FALSE(result.has_value());
        CHECK(result.error() == ac::logging::ProtocolError::malformed_frame);
    }

    SECTION("version") {
        std::string frame = valid_frame();
        frame[4] = static_cast<char>(ac::logging::protocol_version + 1);

        const auto result = ac::logging::decode(frame);
        REQUIRE_FALSE(result.has_value());
        CHECK(result.error() == ac::logging::ProtocolError::unsupported_version);
    }

    SECTION("event type") {
        std::string frame = valid_frame();
        frame[5] = static_cast<char>(0xff);

        const auto result = ac::logging::decode(frame);
        REQUIRE_FALSE(result.has_value());
        CHECK(result.error() == ac::logging::ProtocolError::invalid_event_type);
    }

    SECTION("newline flag") {
        std::string frame = valid_frame();
        frame[6] = '\x02';

        const auto result = ac::logging::decode(frame);
        REQUIRE_FALSE(result.has_value());
        CHECK(result.error() == ac::logging::ProtocolError::malformed_frame);
    }
}

TEST_CASE("Decoder validates declared field lengths", "[log_protocol][unit]") {
    SECTION("component exceeds remaining data") {
        std::string frame = valid_frame();
        write_uint32(frame, 7, 1);

        const auto result = ac::logging::decode(frame);
        REQUIRE_FALSE(result.has_value());
        CHECK(result.error() == ac::logging::ProtocolError::malformed_frame);
    }

    SECTION("message exceeds remaining data") {
        std::string frame = valid_frame();
        write_uint32(frame, 11, 1);

        const auto result = ac::logging::decode(frame);
        REQUIRE_FALSE(result.has_value());
        CHECK(result.error() == ac::logging::ProtocolError::malformed_frame);
    }

    SECTION("trailing bytes are present") {
        std::string frame = valid_frame();
        frame.push_back('x');

        const auto result = ac::logging::decode(frame);
        REQUIRE_FALSE(result.has_value());
        CHECK(result.error() == ac::logging::ProtocolError::malformed_frame);
    }
}

TEST_CASE("Protocol errors have stable descriptions", "[log_protocol][unit]") {
    using enum ac::logging::ProtocolError;

    CHECK(ac::logging::error_message(frame_too_large) ==
        "Logging protocol frame is too large");
    CHECK(ac::logging::error_message(malformed_frame) ==
        "Logging protocol frame is malformed");
    CHECK(ac::logging::error_message(unsupported_version) ==
        "Logging protocol version is unsupported");
    CHECK(ac::logging::error_message(invalid_event_type) ==
        "Logging protocol event type is invalid");
    CHECK(ac::logging::error_message(
        static_cast<ac::logging::ProtocolError>(0xff)
    ) == "Unknown logging protocol error");
}
