module auto_core.main.test_commands;

import std;
import auto_core.main.application;
import auto_core.core.encoding;

void encoding_test() {
    auto_core.print("Entering encoding_test");

    const std::wstring original =
        L"Testing UTF-16: caf\u00e9\u2014Auto Core \U0001f600";
    const std::string utf8 = ac::encoding::to_utf8(original);
    const std::wstring round_trip = ac::encoding::to_utf16(utf8);

    auto_core.print(
        "UTF-16 -> UTF-8 -> UTF-16: {}",
        round_trip == original ? "PASS" : "FAIL"
    );
    auto_core.print(round_trip);

    const std::string invalid_utf8 {
        static_cast<char>(0xC3),
        static_cast<char>(0x28)
    };
    try {
        (void)ac::encoding::to_utf16(invalid_utf8);
        auto_core.print("Invalid UTF-8 conversion succeeded unexpectedly");
    }
    catch (const std::exception& error) {
        auto_core.print("Invalid UTF-8 conversion failed: {}", error.what());
    }

    const std::string user_like_input {
        'c', 'a', 'f', static_cast<char>(0xE9)
    };
    try {
        (void)ac::encoding::to_utf16(user_like_input);
        auto_core.print("User-like input conversion succeeded");
    }
    catch (const std::exception& error) {
        auto_core.print(
            "User-like input conversion failed: {}", error.what()
        );
    }
}

void send_crash_command() {
    *static_cast<volatile int*>(nullptr) = 0;
}

void test_commands::runtime_commands::register_with(
    command_registry::Registry& registry
) {
    registry.add("encoding_test", &::encoding_test);
    registry.add("send_crash_command", &::send_crash_command);
}
