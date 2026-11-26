# Keymap command descriptions

Catalog text for keymap commands. The description is not part of the command name, and it does not change lookup or behavior.

## Storage and publication

[`command_registry.ixx`](../app/shared/command_registry.ixx) stores an optional `description` on each `add` and `add_factory` entry. `catalog_lines()` publishes `name`, or `name - description`. `hello_lines()` uses the same description. The merged `keymap/keymap_commands.txt` uses `format_merged_catalog_line`: `name [Provider] - description`, or `name [Provider]` when the description is empty. A plain command's hello identity is its name. A factory's hello identity is its autocomplete text, such as `make_print_choice("", false)`.

Hosted components send `hello_lines()` in the `ac.component.v2` hello. [`parse_catalog_line`](../app/shared/protocols/component_protocol.ixx) keeps the description. [`ac::main::components::register_with`](../app/main/runtime/components/ac_components.cxx) copies it into `add` or `add_factory`. A name already in the registry is skipped, so the earlier description remains, including when that description is empty.

`auto_core_keymap.exe --build` runs each provider as `<name>_ac.exe --run --export-keymap-commands`. Hello and that export may carry a declared Taskbar application key as `name @key`. Parsing keeps the command name and the key separate. The component protocol and the invocation name stay the original `activate_<program>` command. `keymap/components/<name>.keymap_commands.txt` and `taskbar.keymap_commands.txt` keep that provider's description and replace the shared command with the explicit alias when the build published that alias, such as `itunes_activate`, `journal_activate_word`, `spotify_activate`, or `writer_activate_excel`. A command with no published alias stays under its original name. The merged `keymap/keymap_commands.txt` is rebuilt from the commands the current providers still offer. A shared `activate_<program>` name uses the provider selected in `activate_commands.map`. Any other shared name uses `shared_commands.map`. `taskbar_<program>` is Taskbar's ordinary command for that application key. An explicit blank, while two or more providers still offer the command, is one `activate_program [Unset] - <description>` row. The description is the first non-empty provider sentence in provider-id order. A saved blank or an unavailable provider that leaves the raw name unregistered omits that raw row. Registered aliases stay, labeled with the provider that owns each alias. A command that no provider still offers is omitted, including its description and any alias that is no longer published. A leftover map entry does not keep that row.

A shared `activate_<program>` name is a command offered by one or more eligible providers, including the built-in `activate_auto_core` offer from provider `auto_core`. [`update_activation_map`](../app/core/taskbar/activation_map.cxx) rewrites `# Choices:` from the providers discovered in that successful build. One provider is selected, including a row whose saved value was `taskbar`, blank, or `<key>.map`. Several providers keep a saved value only when that provider is still offered; a stale value becomes blank. A command with no current provider is removed. The direct alias compares the provider with the name after `activate_`: matching names use `<provider>_activate`, and different names use `<provider>_activate_<program>`. Examples are `auto_core_activate`, `itunes_activate`, and `writer_activate_excel`. Taskbar does not publish `taskbar_activate_<program>`. An alias that is already a command is omitted and reported. [`build_keymap_commands`](../app/main/builder/keymap/keymap_commands.cxx) writes the merged names, without descriptions, into `keymap.map` `[commands]`. A failed export returns before the map or the merged catalog is replaced. Startup applies the same selection in memory and does not write the map. One provider resolves even when the saved row is missing or stale, so `numpad_1 = activate_auto_core` runs provider `auto_core` before a rebuild.

An ordinary shared name is any other command offered by two or more providers, or a command that already has a saved line in `keymap/shared_commands.map`. [`update_shared_command_map`](../app/core/taskbar/activation_map.cxx) uses the same choice comment, blank preservation, and single-provider rule while a discovered provider still registers the command. A new ordinary collision is left blank. Its alias is `<provider>_<command>`. After every configured provider exports successfully, a saved command that none of those providers registers is removed with its generated `# Choice:` or `# Choices:` comment. The empty `[shared_commands]` section stays, with the header `# Selects the provider for a shared command.`, `# Format: <command_name> = <provider>`, and `# A blank value leaves the shared name unresolved.` A disabled component is still exported, and a failed export leaves the file unchanged, so neither is evidence that the command was removed. The unprefixed name runs the provider selected in that file. Each alias runs its own provider. [`reconcile_provider_aliases`](../app/core/taskbar/activation_map.cxx) builds one namespace from the existing command names, both maps' command names, and the aliases either resolver accepted. Activation aliases are claimed first. A rejected alias is reported and omitted from the component catalogs, the merged catalog, and the runtime registry. `--build` and startup call that same function.

`--build` writes the Main `activate` factory into the merged catalog from [`activate_factory.hpp`](../app/main/runtime/taskbar/activate_factory.hpp), and startup registers the same fields.

At startup, [`create_runtime_command_registry`](../app/main/runtime/keyboard/keymap_runtime.cxx) reads both maps and applies the same provider rules in memory. It does not rewrite `activate_commands.map`. One activation provider resolves even when its saved row is missing or stale. Several providers honor a saved selection that is still offered and otherwise leave the unprefixed command unresolved. A keymap binding that uses an ordinary shared name while its selection is blank is reported once, names `shared_commands.map` and the available aliases, and does not stop the rest of the keymap from loading. A binding of an unresolved activation command does not print the incorrect-value diagnostic at load or on press. Press prints a blank-selection or unavailable-provider alert that names the command and any saved provider, and that side does not count as a usable keymap entry. [`refresh_keymap_commands`](../app/main/runtime/keyboard/keymap_runtime.cxx) rewrites `keymap/keymap_commands.txt` from the commands startup registered, plus one `[Unset]` row for an explicit blank that still has two or more providers, when the bytes differ. It does not rewrite `keymap.map`, `activate_commands.map`, or `shared_commands.map`. A raw name that the previous build resolved can remain in `[commands]` until the next `--build` even though startup omits it from the catalog and does not run it.

Taskbar's export starts from `create_taskbar_command_registry()` and then applies [`configured_activation_catalog_from_disk`](../app/core/taskbar/taskbar.cxx). A non-empty disk description replaces the compiled description for the same name. Startup registers those disk commands before child hello, so the disk description is the one that runs when both exist.

## Ordinary command

Pass the description to `Registry::add`. Omit it, or pass an empty string, to publish the bare name.

```cpp
registry.add(
    "close_program",
    &::close_program,
    "Shuts down Auto Core and its running components."
);
```

That registration is in [`main_component.cxx`](../app/main/runtime/components/main_component.cxx). A hosted component uses the same `add` call. Main receives the text through hello and does not need its own copy.

## Factory and generated commands

Give the factory one description on `add_factory`. Generate each concrete command from the factory arguments when it is registered. Do not store descriptions in the alias list.

Journal keeps the short builder label in `FactoryDescriptor::menu_label` and the catalog sentence in `catalog_description` ([`journal_factories.ixx`](../app/components/journal/shared/journal_factories.ixx)). [`register_factory_commands`](../app/components/journal/shared/journal_factories.cxx) passes `catalog_description` to `add_factory`. [`load_alias_commands`](../app/components/journal/shared/journal_factories.cxx) passes `alias_description(definition)` to `add`.

`alias_description` dispatches to the generator for that factory:

- `describe_print_choice` includes the configured name and lower bound. `make_print_choice("Tabby", 0)` becomes `Prompts for an upper bound and inserts "Tabby selects N.", where N ranges from 0 through that bound.`
- `describe_insert_text` quotes the configured text. `print_and_insert_into_journal("1 is selected.")` becomes `Inserts the string "1 is selected." into the active textbox.` The alias name stays outside that sentence, so the catalog line is `print_one_is_selected - Inserts the string "1 is selected." into the active textbox.`
- `describe_dice` includes the die count and sides. `roll_dice(7, 8)` becomes `Rolls 7d8 and inserts each roll and the total.`

A name or insert string that is empty after catalog normalization publishes the bare alias. The factory catalog line is `make_print_choice - Journal factory function. Bind directly in keymap.map as make_print_choice("Name", starting_range) or create an alias in make_print_choice.list.` The hello line keeps the autocomplete identity in front of that same sentence. `print_and_insert_into_journal` and `roll_dice` use the same form, with `print_and_insert_into_journal("Text")` and `roll_dice(number_of_dice, number_of_sides)`, and name their own `.list` files.

Writer file commands are the other generator. [`writer_registry.cxx`](../app/components/writer/runtime/writer_registry.cxx) registers `insert_<stem>` as `Inserts the contents of <filename>.`

## Taskbar activation

[`activation_catalog_description`](../app/core/taskbar/activation_catalog_description.hpp) is the only selector. Call it with the command name, the application key, and the map's `display_name`. The header is included directly. iTunes does not compile the Taskbar module.

Selection order:

1. `activate_auto_core_<digits>` and `taskbar_auto_core_<digits>` are `Activates another Auto Core or Auto Core Component.` This wins over `display_name`. `activate_auto_core` and a nonnumeric suffix do not use that sentence.
2. A generated `taskbar_<application>` command uses the same product sentence as `activate_<application>`. Any other command that does not start with `activate_` stays undescribed.
3. A non-empty `[application] display_name` is the product name. Otherwise the key is looked up in `known_activations`.
4. An unknown key with no display name stays undescribed.

`itunes` is the table entry `iTunes`, so the sentence is `Activates iTunes.` `spotify` is `Activates Spotify.` `word` is `Activates Microsoft Word.` `notepad2` is `Activates Notepad Classic.` An alias is described from the application key the map binds it to, not from the alias token: `activate_folder` on `file_explorer` is `Activates File Explorer.` Every `activate_*` sentence from this function ends with a period. A product or display name that already ends with a period does not gain a second one.

`activate_word`, `activate_itunes`, and `activate_spotify` follow `keymap/activate_commands.map`. The selected provider's description is the shared catalog sentence. Each explicit alias uses the description of the provider that owns it. A sole provider is selected and still publishes its alias. The catalog sentence does not mention a component's setup message.

Callers are the disk catalog, compiled `activate_auto_core` in [`taskbar_commands.cxx`](../app/main/taskbar/runtime/taskbar_commands.cxx), and the Word, iTunes, and Spotify registrations.

## Launch-descriptor commands

The descriptor file stays a list of command names. [`catalog_description`](../app/main/shared/launch_descriptor.hpp) maps `launch_dash` and `report_and_empty_recycle_bin` to their sentences. Any other descriptor command stays bare until that function grows an entry.

Dash is not hosted, so its export prints `name - description` from that function ([`dash/runtime/main.cxx`](../app/components/dash/runtime/main.cxx)). Slash passes the same function into `registry.add` ([`slash/runtime/main.cxx`](../app/components/slash/runtime/main.cxx)). Main passes it again from [`register_launch_descriptor_commands`](../app/main/runtime/components/ac_components.cxx). A new on-demand command needs the function entry. Export and startup registration both have to call it, or one catalog loses the sentence.

## Formatting

`catalog_description_separator` is the three characters ` - `. [`normalize_catalog_description`](../app/shared/command_registry.ixx) turns CR and LF into spaces, then trims. [`format_catalog_line`](../app/shared/command_registry.ixx) appends the separator and the normalized text. A missing, empty, or whitespace-only description publishes the identity alone, with no dangling separator. The merged catalog wraps that formatter: `name [Provider]` when the description is empty, and `name [Provider] - description` when it is not. Provider labels come from [`provider_display_name`](../app/main/shared/provider_display_name.hpp). `[Unset]` is the label for an explicit blank that still has two or more providers. It is not written into `keymap.map` `[commands]`.

A plain catalog or hello line splits on the first ` - `. A factory hello line splits only after the matching `)`, then requires ` - `. Text after that first separator may contain further hyphens.

## `keymap.map`

`[commands]` is names only. [`command_section`](../app/main/builder/keymap/keymap_commands.cxx) writes one name per line. `--build` passes `combined_names` into that section and never the descriptions or provider labels. That name set is still the resolved shared names, the published aliases, and the unique commands. An `[Unset]` row and an omitted unresolved raw name are not inserted. Runtime keymap loading skips the section and does not rewrite it. A hand-edited `name - description` there is not a binding and is not a catalog. The next `--build` replaces the section with names. Startup before that rebuild leaves the previous `[commands]` text in place.

## Keep these in step

- Activation wording lives only in `activation_catalog_description.hpp`. A new known product name belongs in `known_activations`. Do not copy the iTunes sentence back into the iTunes registry.
- The shared `activate_<program>` name uses the selected provider's description. Each explicit alias uses the description of the provider that owns that alias. Both still call `activation_catalog_description`. A `display_name` override replaces the known product name for the caller that passes it. The current Word, iTunes, and Spotify registrations pass an empty display name, and their maps do not set one.
- Taskbar export lists the compiled `activate_auto_core` handler and the disk `taskbar_<application>` commands. A non-empty disk description replaces the compiled description only when the names match. `taskbar_auto_core` carries the map description and does not replace `activate_auto_core`.
- Launch-descriptor text lives only in `catalog_description`. The descriptor file, Dash's printed separator, Slash's `add`, and Main's registration all have to keep using that function.
- Journal `menu_label` is the builder menu. `catalog_description` is the factory catalog sentence. Alias sentences come from `describe_print_choice`, `describe_insert_text`, and `describe_dice`. The `.list` line stays `alias = factory(arguments)`.
- Main does not replace an empty description already registered. `--build` does fill an empty description from a later provider. Register the description on the command that wins, or both publishers can disagree.
