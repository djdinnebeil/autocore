# Wake

`wake_ac.exe` logs last-wake output from `powercfg`. `config/wake.ini` exists
so the shared config contract applies; there are no tunables yet. Defaults are
`app/components/wake/shared/defaults.ixx` (`[wake]` with no keys). Extra wake
logs stay under `logs/components/wake/`.

Only `wake_config.exe` writes the file. Missing or malformed: `wake_ac.exe`
reports and continues.

See [Runtime configuration](configuration.md).
