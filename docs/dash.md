# Dash secret storage

`dash_ac.exe` is a special non-v1 runtime. It inserts revocable, noncritical
secrets without keeping their values in plaintext files. Dash is not started
with the component session. Auto Core exposes only the `launch_dash` keymap
command, and that command runs on demand when `dash` is enabled in
`components.list`.

Auto Core Dash has not been developed or audited as a credential manager. It
is not intended to replace a dedicated password manager. Do not use it as the
only store for financial credentials, account-recovery credentials, identity
documents, cryptocurrency keys, or other secrets whose loss or disclosure
would cause serious harm.

## Ownership

`dash_config.exe` is the only writer of `config/dash.ini`.

`dash_editor.exe` is the only writer of the vault. It is the secret editor:
add, remove, and list.

`dash_ac.exe` reads `config/dash.ini` and the vault. It lists secret names
while the user selects one, verifies with Windows Hello, decrypts that value,
and inserts it. It does not add, remove, rename, or otherwise rewrite the
vault.

`dash_settings.exe` writes nothing. It delegates configuration to
`dash_config.exe`, vault management to `dash_editor.exe` (Manage secrets),
and enablement to `components_editor.exe`.

`dash_editor.exe` and `dash_ac.exe` both run as the current Windows user.
The split is single-writer ownership: it keeps vault mutation out of the
normal runtime and makes that boundary easier to audit. It is not a
cryptographic boundary and it is not an operating-system security boundary.
`%LOCALAPPDATA%` is the storage location, not a security mechanism.

## User interface

`dash_ac.exe` offers:

1. Select and insert a secret.
2. Exit.

Secret names are shown only as part of that selection.

`dash_editor.exe` offers:

1. Add a secret.
2. Remove a secret.
3. List secrets.
4. Exit.

List shows names only and does not decrypt or display secret values. Add does
not require Windows Hello. Remove requires Windows Hello and the existing
explicit `yes` confirmation.

`dash_settings.exe` offers Initialize when `config/dash.ini` is missing, and
Configure when that file exists. Manage secrets is next, then Enable or
Disable, then Exit. Star does not read or modify the vault.

## Main launch

`dash_ac.exe` embeds `AC_LAUNCH_DESCRIPTOR`. Main reads that resource and
starts `dash_ac.exe` with `CREATE_NEW_CONSOLE` and these arguments:

```text
--target <foreground HWND as integer> --parent-pid <auto_core.exe pid>
```

The process handle is not kept. If Dash is already running, the child
activates that instance and updates its intended destination window; Main
always launches.

`dash_ac.exe` does not reveal stored values in its console, copy them to the
clipboard, export them, or write an operational log of secret values. The
Auto Core log records only the call to `launch_dash`.

## Storage and protection

The vault is stored at:

```text
%LOCALAPPDATA%\Auto Core\dash.vault
```

There is no vault path setting in `dash.ini`. `dash_editor.exe` protects each
secret value with current-user Windows DPAPI. Secret names and basic vault
structure are not encrypted, so anyone who can read the file may learn the
names and number of stored secrets.

Windows Hello is an application-level verification step used before
`dash_ac.exe` retrieves a stored secret and before `dash_editor.exe` removes
one. It is not cryptographically bound to DPAPI. Software already running as
the same Windows user may be able to call DPAPI without going through Dash
or Windows Hello.

`dash_ac.exe` decrypts a value only for insertion, verifies the destination
window again, sends Unicode keyboard input without using the clipboard, and
wipes its plaintext and input buffers afterward. These measures reduce
accidental exposure but cannot protect against malware, debuggers, screen
capture, keyboard hooks, or a compromised Windows account.

## Backup, migration, and recovery

`dash.vault` is local application state, not a portable backup. Copying it to
a different Windows installation or user profile will ordinarily copy the
ciphertext but will not make it decryptable there.

Dash intentionally has no export or recovery-key system. To move to another
computer, retrieve the affected values from the web applications or services
that issued them and add them to Dash on the new computer. If an issuing
service does not display a secret again, generate a replacement and invalidate
the old one.

Users should retain access to each issuing service and be prepared to rotate
its secret. A lost vault, failed Windows profile, forgotten account credential,
or unavailable old computer may make the stored values unrecoverable. Deleting
`dash.vault` permanently removes Dash's stored copies.

## Configuration

`config/dash.ini` is the only Dash initialization store. The compiled default
in `app/components/dash/shared/defaults.ixx` is:

```text
[dash]
logging = off
```

`logging` is the only key. Only `dash_config.exe` writes the file. `--init`
prompts `Enable logging?` when the file is missing. The default answer is
off, and the file is written with `logging = on` or `logging = off`. `--seed`
and `--init --seed` write `logging = off` when the file is missing, without a
prompt. An existing file is left unchanged by those flags. Neither flag
updates `components.list` or the vault.

A missing or malformed `dash.ini`: `dash_ac.exe` reports it and continues in
memory with logging off. The file is not created.

A missing vault stays missing. Insert, list, and remove report that no secrets
are stored. Adding a secret through `dash_editor.exe` is what creates the
vault at `%LOCALAPPDATA%\Auto Core\dash.vault`.

## Operational limitations

- Dash and the destination application must run at compatible Windows
  integrity levels. Windows may block simulated input into an elevated
  application.
- A destination window title is shown before verification. The user should
  confirm it before completing Windows Hello.
- Secret names should not themselves contain sensitive information.
- Dash is designed for secrets whose compromise can be handled by revocation
  and replacement.
