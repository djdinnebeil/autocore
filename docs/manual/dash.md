# Dash

Dash stores named secrets and inserts a selected value into the window that was in front when Dash opened. It is for secrets you can revoke and replace. It is not a password manager.

Bind `launch_dash`. Dash opens only when it is enabled. The window that was in front is the insertion target. If Dash is already open, that launch updates the target window.

## Configuration

`config\dash.ini`

`[dash]`

| Setting | Default | Values | Description |
| --- | --- | --- | --- |
| `logging` | `off` | `on`, `off` | Controls logging for the Dash component. |

## Commands

| Command | What it does |
| --- | --- |
| `launch_dash` | Opens Dash. Choose a secret to insert, or exit. |

Inserting a secret lists names, asks for Windows Hello, and types the value into the destination window. The value is not shown, copied to the clipboard, or written to a log. If the destination window changes before insertion, the secret is not inserted.

## Files and data

The vault is `%LOCALAPPDATA%\Auto Core\dash.vault`. There is no path setting for it. Adding a secret creates the file. The vault is tied to the current Windows user and does not decrypt on another Windows installation.

Secret names are stored in the clear. Values are protected for the current user. A missing vault means no secrets are stored.

Windows Hello is checked before a secret is inserted or removed. It is not cryptographically bound to that protection. Software already running as the same Windows user may be able to read a value without Windows Hello.

There is no export and no recovery key. Copying the vault to another Windows installation or user profile does not make it decryptable there. A lost vault, Windows profile, or computer can make the stored values unrecoverable. Retrieve or rotate the secret at the service that issued it, then add it again. Deleting `dash.vault` permanently removes Dash's stored copies. Do not put sensitive information in a secret name.

Dash and the destination application must run at compatible Windows integrity levels. Windows may block simulated input into an elevated application. Confirm the window title shown before Windows Hello.

## Component tools

| Tool | What it is for |
| --- | --- |
| `dash_ac.exe` | Select and insert a secret. |
| `dash_editor.exe` | Add, remove, or list secrets. List shows names only. Add does not ask for Windows Hello. Remove asks for Windows Hello and then the word `yes`. |
| `dash_config.exe` | Edits `dash.ini`. |
| `dash_settings.exe` | Opens configuration, secret management, and enablement. |
