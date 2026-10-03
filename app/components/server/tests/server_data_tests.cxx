#include "catch_amalgamated.hpp"
#include "../shared/server_data_detail.hpp"

#include <filesystem>

import server_defaults;

TEST_CASE("server.ini defaults are directory and logging", "[server][data][unit]") {
    const auto seeded = std::string {server::defaults::ini_text};
    const auto rewritten = server::defaults::ini_for(
        server::defaults::directory,
        true
    );
    const auto moved = server::defaults::ini_for(R"(D:\AutoCoreServer)", true);

    CHECK(server::defaults::port == 8585);
    CHECK(server::defaults::document_root == "site");
    CHECK(seeded == rewritten);
    CHECK(seeded.find("directory = components\\server\n") != std::string::npos);
    CHECK(seeded.find("logging = on\n") != std::string::npos);
    CHECK(seeded.find("port") == std::string::npos);
    CHECK(seeded.find("document_root") == std::string::npos);
    CHECK(moved.find("port") == std::string::npos);
    CHECK(moved.find("document_root") == std::string::npos);
    CHECK(moved.find(R"(directory = D:\AutoCoreServer)") != std::string::npos);
}

TEST_CASE("Server directory resolves against the installation root", "[server][data][unit]") {
    const std::filesystem::path root {R"(C:\Example\Auto Core)"};
    const auto fallback = (root / "components" / "server").lexically_normal();

    CHECK(server::data::resolve_directory(std::nullopt, root) == fallback);
    CHECK(server::data::resolve_directory("", root) == fallback);
    CHECK(server::data::resolve_directory("   ", root) == fallback);
    CHECK(server::data::resolve_directory(R"(components\server)", root) == fallback);
    CHECK(
        server::data::resolve_directory(R"(D:\AutoCoreServer)", root) ==
        std::filesystem::path {R"(D:\AutoCoreServer)"}
    );
}

TEST_CASE("Server directory resolution does not create the directory", "[server][data][unit]") {
    const auto root =
        std::filesystem::temp_directory_path() / "ac_server_directory_resolution";
    std::error_code error;
    std::filesystem::remove_all(root, error);

    const auto resolved = server::data::resolve_directory(R"(components\server)", root);

    CHECK(resolved == (root / "components" / "server").lexically_normal());
    CHECK_FALSE(std::filesystem::exists(resolved));
}

TEST_CASE("port.id accepts one integer from 1 to 65535", "[server][data][unit]") {
    CHECK(server::data::parse_port("8585") == 8585);
    CHECK(server::data::parse_port("  1\n") == 1);
    CHECK(server::data::parse_port("65535") == 65535);
    CHECK_FALSE(server::data::parse_port(""));
    CHECK_FALSE(server::data::parse_port("   "));
    CHECK_FALSE(server::data::parse_port("0"));
    CHECK_FALSE(server::data::parse_port("65536"));
    CHECK_FALSE(server::data::parse_port("8585abc"));
    CHECK_FALSE(server::data::parse_port("85\n85"));
}

TEST_CASE("document_root.id is one scalar path", "[server][data][unit]") {
    CHECK(server::data::parse_document_root("site") == "site");
    CHECK(server::data::parse_document_root("  web\r\n") == "web");
    CHECK_FALSE(server::data::parse_document_root(""));
    CHECK_FALSE(server::data::parse_document_root(" \n "));
    CHECK_FALSE(server::data::parse_document_root("site\nweb"));
}

TEST_CASE("relative document_root.id resolves under the Server directory", "[server][data][unit]") {
    const std::filesystem::path root {R"(C:\Example\Auto Core)"};
    const auto data = server::data::resolve_directory(R"(components\server)", root);
    const auto other = std::filesystem::path {R"(D:\AutoCoreServer)"};

    CHECK(
        server::data::resolve_document_root("site", data) ==
        (data / "site").lexically_normal()
    );
    CHECK(
        server::data::resolve_document_root("site", other) ==
        std::filesystem::path {R"(D:\AutoCoreServer\site)"}
    );
    CHECK(
        server::data::resolve_document_root("web", other) ==
        std::filesystem::path {R"(D:\AutoCoreServer\web)"}
    );
    CHECK(
        server::data::resolve_document_root(R"(C:\DJ\My Folder\)", data) ==
        std::filesystem::path {R"(C:\DJ\My Folder\)"}.lexically_normal()
    );
    CHECK(server::data::document_root_is_absolute(R"(C:\DJ\My Folder\)"));
    CHECK_FALSE(server::data::document_root_is_absolute("site"));
}

TEST_CASE("the builder accepts only a relative document root inside the data directory", "[server][data][unit]") {
    const auto data = std::filesystem::path {R"(D:\AutoCoreServer)"};

    CHECK(
        server::data::builder_document_root("site", data) ==
        std::filesystem::path {R"(D:\AutoCoreServer\site)"}
    );
    CHECK(
        server::data::builder_document_root("web", data) ==
        std::filesystem::path {R"(D:\AutoCoreServer\web)"}
    );
    CHECK_FALSE(server::data::builder_document_root(R"(C:\DJ\My Folder\)", data));
    CHECK_FALSE(server::data::builder_document_root(R"(..\other)", data));
    CHECK_FALSE(server::data::builder_document_root(R"(site\..\..\outside)", data));
    CHECK_FALSE(server::data::builder_document_root("", data));
    CHECK_FALSE(server::data::builder_document_root("site\nweb", data));
}

TEST_CASE("a missing document root fails without launching the builder", "[server][data][unit]") {
    using server::data::DocumentRootStartup;

    CHECK(server::data::document_root_startup(true) == DocumentRootStartup::serve);
    CHECK(server::data::document_root_startup(false) == DocumentRootStartup::fail);
}
