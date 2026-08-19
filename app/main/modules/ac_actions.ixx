export module ac_actions;

export import command_registry;

export namespace ac_actions::runtime_commands {
    void register_with(command_registry::Registry& registry);
}

export {
    void print_timestamp();
    void print_date_iso();
    void print_date_compact();
    void print_date_iso_with_timestamp();
    void print_date_iso_with_timestamp_w();
    void print_today_is_day();
    void add_brackets_around_clipboard();
    void send_alt_f12();
    void print_and_insert_special_utf8();
    void print_and_insert_special_utf16();
    void print_and_insert_testing();
    void encoding_test();
    void send_crash_command();
}
