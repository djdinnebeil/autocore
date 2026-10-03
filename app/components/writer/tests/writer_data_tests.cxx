#include "catch_amalgamated.hpp"
#include "../shared/writer_data_detail.hpp"

#include <filesystem>

import writer_defaults;

TEST_CASE("writer.ini seed is directory, notes, and logging", "[writer][data][unit]") {
    const auto seeded = std::string {writer::defaults::ini_text};
    const auto rewritten = writer::defaults::ini_for(
        writer::defaults::directory,
        writer::defaults::notes_subdirectory,
        true
    );
    const auto moved = writer::defaults::ini_for(R"(D:\WriterData)", "daily", true);

    CHECK(writer::defaults::directory == "writer");
    CHECK(writer::defaults::notes_subdirectory == "notes");
    CHECK(seeded == rewritten);
    CHECK(seeded.find("directory = writer\n") != std::string::npos);
    CHECK(seeded.find("notes_subdirectory = notes\n") != std::string::npos);
    CHECK(seeded.find("logging = on\n") != std::string::npos);
    CHECK(moved.find(R"(directory = D:\WriterData)") != std::string::npos);
    CHECK(moved.find("notes_subdirectory = daily\n") != std::string::npos);
    CHECK(moved.find("logging = on\n") != std::string::npos);
}

TEST_CASE("Writer seed files are the recommended starter lines", "[writer][data][unit]") {
    const std::string prompts {
        std::string {writer::defaults::session_prompt_primary} + "\n" +
        std::string {writer::defaults::session_prompt_secondary} + "\n"
    };
    const std::string task {
        std::string {writer::defaults::task_line} + "\n"
    };

    CHECK(prompts == writer::defaults::session_prompts_text);
    CHECK(task == writer::defaults::task_list_text);
    CHECK(prompts ==
        "I am running Auto Core.\n"
        "I am installing a new component in Auto Core.\n");
    CHECK(task == "Add a new task to task_list.txt to show up here.\n");
}

TEST_CASE("Writer stores resolve under the installation root", "[writer][data][unit]") {
    const std::filesystem::path root {R"(C:\Example\Auto Core)"};
    const auto layout = writer::data::resolve_layout(
        std::nullopt,
        std::nullopt,
        {},
        root
    );

    CHECK(layout.directory == (root / "writer").lexically_normal());
    CHECK(layout.session_prompts == layout.directory / "session_prompts.list");
    CHECK(layout.task_list == layout.directory / "task_list.txt");
    CHECK(layout.notes == (layout.directory / "notes").lexically_normal());
    CHECK(layout.notes_subdirectory == "notes");
}

TEST_CASE("configured Writer directory is not the compiled default", "[writer][data][unit]") {
    const std::filesystem::path root {R"(C:\Example\Auto Core)"};
    const auto layout = writer::data::resolve_layout(
        std::filesystem::path {R"(D:\WriterData)"},
        std::filesystem::path {"daily"},
        "daily",
        root
    );

    CHECK(layout.directory == std::filesystem::path {R"(D:\WriterData)"});
    CHECK(layout.session_prompts ==
        std::filesystem::path {R"(D:\WriterData\session_prompts.list)"});
    CHECK(layout.task_list ==
        std::filesystem::path {R"(D:\WriterData\task_list.txt)"});
    CHECK(layout.notes == std::filesystem::path {R"(D:\WriterData\daily)"});
    CHECK(layout.notes_subdirectory == "daily");
    CHECK(layout.directory != (root / "writer").lexically_normal());
}

TEST_CASE("non-relative notes_subdirectory falls back to notes", "[writer][data][unit]") {
    const std::filesystem::path root {R"(C:\Example\Auto Core)"};
    const auto layout = writer::data::resolve_layout(
        std::filesystem::path {R"(D:\WriterData)"},
        std::filesystem::path {R"(D:\Notes)"},
        R"(D:\Notes)",
        root
    );

    CHECK(layout.directory == std::filesystem::path {R"(D:\WriterData)"});
    CHECK(layout.notes == std::filesystem::path {R"(D:\WriterData\notes)"});
    CHECK(layout.notes_subdirectory == "notes");
    CHECK_FALSE(writer::data::is_relative_subdirectory(
        std::filesystem::path {R"(D:\Notes)"}
    ));
    CHECK(writer::data::is_relative_subdirectory(std::filesystem::path {"daily"}));
}

TEST_CASE("Writer resolution does not create directories", "[writer][data][unit]") {
    const auto root =
        std::filesystem::temp_directory_path() / "ac_writer_directory_resolution";
    std::error_code error;
    std::filesystem::remove_all(root, error);

    const auto layout = writer::data::resolve_layout(
        std::nullopt,
        std::nullopt,
        {},
        root
    );

    CHECK(layout.directory == (root / "writer").lexically_normal());
    CHECK(layout.notes == (root / "writer" / "notes").lexically_normal());
    CHECK_FALSE(std::filesystem::exists(layout.directory));
    CHECK_FALSE(std::filesystem::exists(layout.notes));
}

TEST_CASE("required Writer baseline ignores the notes directory", "[writer][data][unit]") {
    const auto root =
        std::filesystem::temp_directory_path() / "ac_writer_baseline";
    std::error_code error;
    std::filesystem::remove_all(root, error);
    const auto writer = root / "writer";
    std::filesystem::create_directories(writer / "notes", error);

    CHECK(writer::data::baseline_incomplete(writer));

    {
        std::ofstream prompts(
            writer / "session_prompts.list",
            std::ios::binary
        );
        prompts << "kept\n";
    }
    CHECK(writer::data::baseline_incomplete(writer));

    {
        std::ofstream tasks(writer / "task_list.txt", std::ios::binary);
        tasks << "kept\n";
    }
    CHECK_FALSE(writer::data::baseline_incomplete(writer));

    std::filesystem::remove_all(writer / "notes", error);
    CHECK_FALSE(writer::data::baseline_incomplete(writer));
    CHECK_FALSE(std::filesystem::exists(writer / "notes"));

    std::filesystem::remove_all(root, error);
}
