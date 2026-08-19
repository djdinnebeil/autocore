/**
 * \file log_protocol.cxx
 * \brief Implements logger-protocol frame encoding and decoding.
 */
module auto_core.logging.protocol;

import std;

namespace {

    constexpr std::array<char, 4> protocol_magic {
        'A', 'C', 'L', 'G'
    };

    constexpr std::size_t encoded_header_size =
        protocol_magic.size() + 1 + 1 + 1 + 4 + 4;

    void append_uint32_le(
        std::string& output,
        const std::uint32_t value
    ) {
        for (unsigned int shift = 0; shift < 32; shift += 8) {
            output.push_back(static_cast<char>(
                (value >> shift) & 0xffu
            ));
        }
    }

    std::uint32_t read_uint32_le(
        const std::string_view input,
        const std::size_t offset
    ) noexcept {
        std::uint32_t value = 0;

        for (unsigned int index = 0; index < 4; ++index) {
            const auto byte = static_cast<unsigned char>(
                input[offset + index]
            );

            value |= static_cast<std::uint32_t>(byte) << (index * 8);
        }

        return value;
    }

    bool valid_event_type(
        const ac::logging::EventType type
    ) noexcept {
        switch (type) {
        case ac::logging::EventType::message:
        case ac::logging::EventType::shutdown:
            return true;
        }

        return false;
    }

} // namespace

namespace ac::logging {

    std::string_view error_message(
        const ProtocolError error
    ) noexcept {
        switch (error) {
        case ProtocolError::frame_too_large:
            return "Logging protocol frame is too large";
        case ProtocolError::malformed_frame:
            return "Logging protocol frame is malformed";
        case ProtocolError::unsupported_version:
            return "Logging protocol version is unsupported";
        case ProtocolError::invalid_event_type:
            return "Logging protocol event type is invalid";
        }

        return "Unknown logging protocol error";
    }

    std::expected<std::string, ProtocolError>
        encode(const Event& event) {
        if (!valid_event_type(event.type)) {
            return std::unexpected(
                ProtocolError::invalid_event_type
            );
        }

        if (
            event.component.size() >
                (std::numeric_limits<std::uint32_t>::max)() ||
            event.message.size() >
                (std::numeric_limits<std::uint32_t>::max)()
        ) {
            return std::unexpected(
                ProtocolError::frame_too_large
            );
        }

        const std::size_t body_size =
            event.component.size() + event.message.size();

        if (body_size > maximum_encoded_size - encoded_header_size) {
            return std::unexpected(
                ProtocolError::frame_too_large
            );
        }

        std::string encoded;
        encoded.reserve(encoded_header_size + body_size);
        encoded.append(protocol_magic.data(), protocol_magic.size());
        encoded.push_back(static_cast<char>(protocol_version));
        encoded.push_back(static_cast<char>(event.type));
        encoded.push_back(event.newline ? '\x01' : '\x00');
        append_uint32_le(
            encoded,
            static_cast<std::uint32_t>(event.component.size())
        );
        append_uint32_le(
            encoded,
            static_cast<std::uint32_t>(event.message.size())
        );
        encoded.append(event.component);
        encoded.append(event.message);

        return encoded;
    }

    std::expected<Event, ProtocolError>
        decode(const std::string_view data) {
        if (data.size() > maximum_encoded_size) {
            return std::unexpected(
                ProtocolError::frame_too_large
            );
        }

        if (data.size() < encoded_header_size) {
            return std::unexpected(
                ProtocolError::malformed_frame
            );
        }

        const std::string_view expected_magic {
            protocol_magic.data(),
            protocol_magic.size()
        };

        if (data.substr(0, protocol_magic.size()) != expected_magic) {
            return std::unexpected(
                ProtocolError::malformed_frame
            );
        }

        const auto version = static_cast<std::uint8_t>(data[4]);

        if (version != protocol_version) {
            return std::unexpected(
                ProtocolError::unsupported_version
            );
        }

        const auto type = static_cast<EventType>(
            static_cast<std::uint8_t>(data[5])
        );

        if (!valid_event_type(type)) {
            return std::unexpected(
                ProtocolError::invalid_event_type
            );
        }

        const auto newline = static_cast<std::uint8_t>(data[6]);

        if (newline > 1) {
            return std::unexpected(
                ProtocolError::malformed_frame
            );
        }

        const std::uint32_t component_size = read_uint32_le(data, 7);
        const std::uint32_t message_size = read_uint32_le(data, 11);
        const std::size_t remaining_size =
            data.size() - encoded_header_size;

        if (
            component_size > remaining_size ||
            message_size > remaining_size - component_size ||
            component_size + message_size != remaining_size
        ) {
            return std::unexpected(
                ProtocolError::malformed_frame
            );
        }

        const std::size_t component_offset = encoded_header_size;
        const std::size_t message_offset =
            component_offset + component_size;

        return Event {
            .type = type,
            .component = std::string {
                data.substr(component_offset, component_size)
            },
            .message = std::string {
                data.substr(message_offset, message_size)
            },
            .newline = newline == 1
        };
    }

} // namespace ac::logging
