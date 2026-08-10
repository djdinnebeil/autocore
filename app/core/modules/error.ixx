export module error;

import std;

export namespace ac::error {

    void log(std::string_view message);

    template<typename... Args>
    void log(
        const char* format_string,
        Args&&... args
    ) {
        try {
            auto convert_arg = [](auto&& arg) -> decltype(auto) {
                using Arg = std::decay_t<decltype(arg)>;

                if constexpr (
                    std::is_same_v<Arg, std::filesystem::path>
                    ) {
                    return arg.string();
                }
                else {
                    return std::forward<decltype(arg)>(arg);
                }
                };

            auto converted_args = std::make_tuple(
                convert_arg(std::forward<Args>(args))...
            );

            const std::string message = std::apply(
                [&](auto&&... unpacked_args) {
                    return std::vformat(
                        format_string,
                        std::make_format_args(unpacked_args...)
                    );
                },
                converted_args
            );

            log(message);
        }
        catch (const std::format_error& exception) {
            log(
                std::string("Error message formatting failed: ") +
                exception.what()
            );
        }
    }

}

