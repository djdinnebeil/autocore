export module notes;

export import command_registry;

export namespace notes::runtime_commands {
    void register_with(command_registry::Registry& registry);
}

export {
	void create_new_note_in_vs_code();
	void create_new_note_in_notepad();
}
