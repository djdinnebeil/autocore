# Dash secret storage

`dash_ac.exe` is a convenience component for storing and inserting revocable,
noncritical secrets without keeping their values in plaintext files.

Dash is not intended to replace a dedicated password manager. Do not use it as
the only store for financial credentials, account-recovery credentials,
identity documents, cryptocurrency keys, or other secrets whose loss or
disclosure would cause serious harm.

## User interface

Dash provides five operations:

1. Add a secret.
2. Select and insert a secret.
3. Remove a secret.
4. List secret names.
5. Exit.

Auto Core exposes only the `launch_dash` keymap command. Dash is not started
with the component session; `launch_dash` is on demand when `dash` is enabled
in `config/components.ini` `[components]`.

## Main launch

Main starts `dash_ac.exe` with `CREATE_NEW_CONSOLE` and these arguments:

```text
--target <foreground HWND as integer> --parent-pid <auto_core.exe pid>
```

The process handle is not kept. If Dash is already running, the child
activates that instance and updates its intended destination window; Main
always launches.

Dash owns all secret selection, protection, verification, and insertion
behavior. It does not reveal stored values in its console, copy them to the
clipboard, export them, or write an operational log. The Auto Core log
records only the call to `launch_dash`.

## Storage and protection

The vault is stored at:

```text
%LOCALAPPDATA%\Auto Core\dash.vault
```

Secret values are protected individually with current-user Windows DPAPI.
Secret names and basic vault structure are not encrypted, so anyone who can
read the file may learn the names and number of stored secrets.

Windows Hello is required before Dash decrypts a selected secret and before it
removes a secret. Windows Hello is an application-level user-presence check; it
is not cryptographically bound to DPAPI. Software already running as the same
Windows user may be able to call DPAPI without going through Dash or Windows
Hello.

Dash decrypts a value only for insertion, verifies the destination window
again, sends Unicode keyboard input without using the clipboard, and wipes its
plaintext and input buffers afterward. These measures reduce accidental
exposure but cannot protect against malware, debuggers, screen capture,
keyboard hooks, or a compromised Windows account.

## Backup, migration, and recovery

`dash.vault` is local application state, not a portable backup. Copying it to a
different Windows installation or user profile will ordinarily copy the
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

`config/dash.ini` exists so the shared config contract applies. There are no
tunables yet; defaults are `app/components/dash/shared/defaults.ixx` (`[dash]`
with no keys). Only `dash_config.exe` writes the file. Missing or malformed:
`dash_ac.exe` reports and continues. The vault stays under `%LOCALAPPDATA%`.

## Operational limitations

- Dash and the destination application must run at compatible Windows
  integrity levels. Windows may block simulated input into an elevated
  application.
- A destination window title is shown before verification. The user should
  confirm it before completing Windows Hello.
- Secret names should not themselves contain sensitive information.
- Dash is designed for secrets whose compromise can be handled by revocation
  and replacement.

