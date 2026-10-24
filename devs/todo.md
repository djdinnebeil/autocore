# Auto Core follow-up work

DLL-specific deferred work is tracked in [app/core/TODO.md](../app/core/TODO.md).
Main-specific deferred work is tracked in [app/main/runtime/TODO.md](../app/main/runtime/TODO.md).

## Prerelease documentation and product-surface follow-ups

- Add an end-user Keymap reference covering supported key names, binding
  syntax, primary and secondary actions, the function-key toggle, command
  catalogs, editing, and when changes are loaded.
- Define a clean prerelease staging procedure for `dist\` that excludes
  machine-specific shortcuts, authorization data, logs, histories, caches,
  personal Writer and Journal data, and an initialization sentinel.

## Windows Console Host and Taskbar Icon

On some systems, Auto Core may display the Windows Terminal icon instead of its own when running without Administrator privileges.

**Solution:**
1. Open Windows Terminal.
2. Navigate to **Settings → Startup**.
3. Set **Default terminal application** to **Windows Console Host**.
4. Restart Auto Core.

This resolves the icon issue without requiring Administrator privileges. Running Auto Core without elevation is recommended, especially during development, to preserve Windows security protections.

**Future investigation:** Explore AppUserModelID (AUMID), shortcut metadata, and other Windows configuration options to preserve the correct icon regardless of the default terminal host.

## Investigate iTunes privilege compatibility and security

**Status:** Privilege matrix documented. Mixed-privilege security and compatibility review remains deferred.

The known combinations are documented in `docs/manual/components/itunes.md` and summarized in `README.md`:

- Non-admin Auto Core / non-admin iTunes: supported and recommended.
- Admin Auto Core / admin iTunes: known working. This is not the recommended runtime.
- Non-admin Auto Core / admin iTunes: compatibility issues may occur.
- Admin Auto Core / non-admin iTunes: compatibility issues may occur.

The two matching privilege combinations are known to work. Mixed privilege combinations are the compatibility concern. This does not claim a broader Windows integrity-level guarantee. Auto Core and the normal iTunes application should both run without Administrator privileges. There is no remaining elevation requirement for that configuration.

The mixed-privilege investigation remains deferred. An elevated Auto Core process has failed to connect to an iTunes instance already running without Administrator privileges, and that failure was associated with the elevation mismatch rather than startup timing. Investigate whether the iTunes component can safely control an existing iTunes process at a different privilege level, and whether a lower-privilege helper or another COM activation strategy would create unacceptable spoofing, IPC, file-access, or privilege-boundary risks. Prefer a least-privilege design. Do not weaken Windows security boundaries, and do not add an elevation workaround, merely to make the connection succeed.
