/**
 * \file log_protocol.ixx
 * \brief Defines the versioned wire representation of main-log events.
 */
module;

#include "ac_api.hpp"

export module auto_core.core.logging.protocol;

import std;

export namespace ac::logging {

    /**
     * \brief The only wire-protocol version accepted by this build.
     */
    inline constexpr std::uint8_t protocol_version = 1;

    /**
     * \brief Size of the fixed header that precedes component and message bytes.
     *
     * Layout: bytes 0–3 `ACLG` magic, 4 protocol version, 5 `EventType`,
     * 6 newline flag (`0` or `1`), 7–10 little-endian component length,
     * 11–14 little-endian message length.
     */
    inline constexpr std::size_t encoded_header_size = 15;

    /**
     * \brief Maximum encoded frame size, including `encoded_header_size`.
     */
    inline constexpr std::size_t maximum_encoded_size = 1024 * 1024;

    /**
     * \brief Identifies the operation represented by a log event.
     */
    enum class EventType : std::uint8_t {
        message = 0,
        shutdown = 1
    };

    /**
     * \brief A message or control event exchanged with the main logger.
     *
     * `component` and `message` are encoded byte-for-byte. They conventionally
     * contain UTF-8, but the protocol does not validate their encoding.
     */
    struct Event {
        EventType type = EventType::message;
        std::string component;
        std::string message;
        bool newline = true;
    };

    /**
     * \brief Describes a failure to encode or decode a protocol frame.
     */
    enum class ProtocolError {
        frame_too_large,
        malformed_frame,
        unsupported_version,
        invalid_event_type
    };

    /**
     * \brief Returns a stable, human-readable description of a protocol error.
     *
     * \param error The protocol error to describe.
     * \return A static string for `error`, or an unknown-error description
     * when the value is not a declared `ProtocolError` enumerator.
     */
    [[nodiscard]]
    AC_API std::string_view
        error_message(ProtocolError error) noexcept;

    /**
     * \brief Encodes an event as a logger-protocol frame.
     *
     * The header is `encoded_header_size` bytes (`ACLG`, version, type,
     * newline `0` or `1`, and two little-endian 32-bit lengths). The two byte
     * strings follow without terminators. The complete frame is the payload of
     * one `pipes::send_string` / `read_string` frame; `logger_ac.exe` decodes that
     * pipe payload, not the raw pipe bytes.
     *
     * \param event The event to encode.
     * \return The encoded frame, `invalid_event_type` for an unknown type, or
     * `frame_too_large` when the complete frame would exceed
     * `maximum_encoded_size`.
     */
    [[nodiscard]]
    AC_API std::expected<std::string, ProtocolError>
        encode(const Event& event);

    /**
     * \brief Decodes and validates one complete logger-protocol frame.
     *
     * Trailing bytes are not permitted. Versions other than
     * `protocol_version`, newline values other than `0` or `1`, and values
     * outside the declared `EventType` range are rejected. Component and
     * message bytes are preserved without text validation.
     *
     * \param data The complete encoded frame (one pipe string payload).
     * \return The decoded event, or the error describing why the frame was
     * rejected.
     */
    [[nodiscard]]
    AC_API std::expected<Event, ProtocolError>
        decode(std::string_view data);

} // namespace ac::logging
