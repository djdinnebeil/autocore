# Keymap command descriptions

Catalog text for keymap commands. The description is not part of the command name, and it does not change lookup or behavior.

## Storage and publication

[`command_registry.ixx`](../app/shared/command_registry.ixx) stores an optional `description` on each `add` and `add_factory` entry. `catalog_lines()` publishes `name`, or `name - description`. `hello_lines()` uses the same description. A plain command's hello identity is its name. A factory's hello identity is its autocomplete text, such as `make_print_choice("", false)`.

Hosted components send `hello_lines()` in the `ac.component.v2` hello. [`parse_catalog_line`](../app/shared/protocols/component_protocol.ixx) keeps the description. [`ac::main::components::register_with`](../app/main/runtime/components/ac_components.cxx) copies it into `add` or `add_factory`. A name already in the registry is skipped, so the earlier description remains, including when that description is empty.

`auto_core_keymap.exe --build` runs each provider as `<name>_ac.exe --run --export-keymap-commands` and writes `keymap/components/<name>.keymap_commands.txt` plus the merged `keymap/keymap_commands.txt`. Provider order is dash, itunes, journal, logger, server, slash, spotify, taskbar, wake, writer. The first description for a name is kept. A later provider fills that entry only when the stored description is still empty. [`build_keymap_commands`](../app/main/builder/keymap/keymap_commands.cxx) then writes those names, without descriptions, into `keymap.map` `[commands]`.

At startup, [`create_runtime_command_registry`](../app/main/runtime/keyboard/keymap_runtime.cxx) builds one registry: test commands, Main commands, reserved Taskbar commands, configured Taskbar activations, hosted hello catalogs and launch-descriptor commands, then the remaining Main Taskbar commands. [`refresh_keymap_commands`](../app/main/runtime/keyboard/keymap_runtime.cxx) rewrites `keymap/keymap_commands.txt` from `catalog_lines()` when the bytes differ. It does not rewrite `keymap.map`.

Taskbar's export starts from `create_taskbar_command_registry()` and then applies [`configured_activation_catalog_from_disk`](../app/core/taskbar/taskbar.cxx). A non-empty disk description replaces the compiled description for the same name. Startup registers those disk commands before child hello, so the disk description is the one that runs when both exist.

## Ordinary command

Pass the description to `Registry::add`. Omit it, or pass an empty string, to publish the bare name.

```cpp
registry.add(
    "close_program",
    &::close_program,
    "Shuts down hosted components using config/shutdown.ini and exits."
);
```

That registration is in [`main_component.cxx`](../app/main/runtime/components/main_component.cxx). A hosted component uses the same `add` call. Main receives the text through hello and does not need its own copy.

## Factory and generated commands

Give the factory one description on `add_factory`. Generate each concrete command from the factory arguments when it is registered. Do not store descriptions in the alias list.

Journal keeps the short builder label in `FactoryDescriptor::menu_label` and the catalog sentence in `catalog_description` ([`journal_factories.ixx`](../app/components/journal/shared/journal_factories.ixx)). [`register_factory_commands`](../app/components/journal/shared/journal_factories.cxx) passes `catalog_description` to `add_factory`. [`load_alias_commands`](../app/components/journal/shared/journal_factories.cxx) passes `alias_description(definition)` to `add`.

`alias_description` dispatches to the generator for that factory:

- `describe_print_choice` includes the configured name and lower bound. `make_print_choice("Tabby", 0)` becomes `Prompts for an upper bound and inserts "Tabby selects N.", where N ranges from 0 through that bound.`
- `describe_insert_text` quotes the configured text. A final `.`, `!`, or `?` is not followed by another period, so `"1 is selected."` stays `Prints and inserts "1 is selected."`
- `describe_dice` includes the die count and sides. `roll_dice(7, 8)` becomes `Rolls 7d8 and inserts each roll and the total.`

A name or insert string that is empty after catalog normalization publishes the bare alias. The factory catalog line is `make_print_choice - A Journal factory function that creates a numbered print-choice command.` The hello line keeps the autocomplete identity in front of that same sentence.

Writer file commands are the other generator. [`writer_registry.cxx`](../app/components/writer/runtime/writer_registry.cxx) registers `insert_<stem>` as `Inserts the contents of <filename>.`

## Taskbar activation

[`activation_catalog_description`](../app/core/taskbar/activation_catalog_description.hpp) is the only selector. Call it with the command name, the application key, and the map's `display_name`. The header is included directly. iTunes does not compile the Taskbar module.

Selection order:

1. `activate_auto_core_` followed by one or more digits, such as `activate_auto_core_2`, is `Activates another Auto Core or Auto Core Component`. This wins over `display_name`. `activate_auto_core` and a nonnumeric suffix do not use that sentence.
2. Any other command that does not start with `activate_` stays undescribed.
3. A non-empty `[application] display_name` is the product name. Otherwise the key is looked up in `known_activations`.
4. An unknown key with no display name stays undescribed.
5. `activate_word`, `activate_itunes`, and `activate_spotify` append `. Shows setup instructions when no application map is available.` Those registrations only print setup instructions when no map supplied the name.

`itunes` is the table entry `iTunes`, so the sentence begins `Activates iTunes`. An alias is described from the application key the map binds it to, not from the alias token: `activate_folder` on `file_explorer` is `Activates File Explorer`.

Callers are the disk catalog, reserved `activate_wordpad` and `activate_powershell_in_admin` in [`main_taskbar.cxx`](../app/main/runtime/taskbar/main_taskbar.cxx), compiled `activate_auto_core` in [`taskbar_commands.cxx`](../app/components/taskbar/runtime/taskbar_commands.cxx), and the Word, iTunes, and Spotify fallback registrations.

## Launch-descriptor commands

The descriptor file stays a list of command names. [`catalog_description`](../app/main/shared/launch_descriptor.hpp) maps `launch_dash` and `report_and_empty_recycle_bin` to their sentences. Any other descriptor command stays bare until that function grows an entry.

Dash is not hosted, so its export prints `name - description` from that function ([`dash/runtime/main.cxx`](../app/components/dash/runtime/main.cxx)). Slash passes the same function into `registry.add` ([`slash/runtime/main.cxx`](../app/components/slash/runtime/main.cxx)). Main passes it again from [`register_launch_descriptor_commands`](../app/main/runtime/components/ac_components.cxx). A new on-demand command needs the function entry. Export and startup registration both have to call it, or one catalog loses the sentence.

## Formatting

`catalog_description_separator` is the three characters ` - `. [`normalize_catalog_description`](../app/shared/command_registry.ixx) turns CR and LF into spaces, then trims. [`format_catalog_line`](../app/shared/command_registry.ixx) appends the separator and the normalized text. A missing, empty, or whitespace-only description publishes the identity alone, with no dangling separator.

A plain catalog or hello line splits on the first ` - `. A factory hello line splits only after the matching `)`, then requires ` - `. Text after that first separator may contain further hyphens.

## `keymap.map`

`[commands]` is names only. [`command_section`](../app/main/builder/keymap/keymap_commands.cxx) writes one name per line. `--build` passes `combined_names` into that section and never the descriptions. Runtime keymap loading skips the section. A hand-edited `name - description` there is not a binding and is not a catalog. The next `--build` replaces the section with names.

## Keep these in step

- Activation wording lives only in `activation_catalog_description.hpp`. A new known product name belongs in `known_activations`. Do not copy the iTunes sentence back into the iTunes registry.
- `activate_word`, `activate_itunes`, and `activate_spotify` are described both by the component registry and by the Taskbar disk catalog. With an empty `display_name` the sentences match. A non-empty `display_name` changes the Taskbar sentence. Startup keeps that disk sentence because Taskbar registers first. `--build` keeps the earlier component sentence when it is already non-empty.
- Taskbar export lets a non-empty disk description replace the compiled registry description. The compiled `activate_auto_core` sentence therefore yields to the map's `display_name` on export.
- Launch-descriptor text lives only in `catalog_description`. The descriptor file, Dash's printed separator, Slash's `add`, and Main's registration all have to keep using that function.
- Journal `menu_label` is the builder menu. `catalog_description` is the factory catalog sentence. Alias sentences come from `describe_print_choice`, `describe_insert_text`, and `describe_dice`. The `.list` line stays `alias = factory(arguments)`.
- Main does not replace an empty description already registered. `--build` does fill an empty description from a later provider. Register the description on the command that wins, or both publishers can disagree.
