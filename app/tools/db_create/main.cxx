import std;
import <sqlite3.h>;

namespace {

    bool execute_sql(sqlite3* database, const char* statement) {
        char* error_message = nullptr;

        const int result = sqlite3_exec(
            database,
            statement,
            nullptr,
            nullptr,
            &error_message
        );

        if (result == SQLITE_OK) {
            return true;
        }

        std::cerr << "SQL error: "
            << (error_message != nullptr
                ? error_message
                : sqlite3_errmsg(database))
            << '\n';

        sqlite3_free(error_message);
        return false;
    }

}

int main() {
    sqlite3* database_handle = nullptr;
    const int open_result = sqlite3_open(
        "star.db",
        &database_handle
    );

    const std::unique_ptr<sqlite3, decltype(&sqlite3_close)> database {
        database_handle,
        &sqlite3_close
    };

    if (open_result != SQLITE_OK) {
        std::cerr << "Error opening or creating the database: "
            << (database != nullptr
                ? sqlite3_errmsg(database.get())
                : "SQLite did not provide a database handle")
            << '\n';
        return open_result;
    }

    std::cout << "Opened or created star.db successfully\n";

    if (!execute_sql(
        database.get(),
        "CREATE TABLE IF NOT EXISTS counter (value INTEGER);"
    )) {
        return 1;
    }

    std::cout << "Created or verified the counter table\n";

    if (!execute_sql(
        database.get(),
        "INSERT INTO counter (value) VALUES (1);"
    )) {
        return 1;
    }

    std::cout << "Inserted the initial counter value\n"
        << "Press Enter to exit...\n";
    std::cin.get();
    return 0;
}
