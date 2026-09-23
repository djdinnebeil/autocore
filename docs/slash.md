# Slash

`slash_ac.exe` is a one-shot recycle-bin helper. `config/slash.ini` exists so
the shared config contract applies; there are no tunables yet. Defaults are
`app/components/slash/shared/defaults.ixx` (`[slash]` with no keys).

Only `slash_config.exe` writes the file (default bytes). Missing or malformed:
`slash_ac.exe` reports and continues with the same empty schema.

See [Runtime configuration](configuration.md).
