# Auto Core follow-up work

DLL-specific deferred work is tracked in [app/core/TODO.md](../app/core/TODO.md).
Main-specific deferred work is tracked in [app/main/runtime/TODO.md](../app/main/runtime/TODO.md).

## Prerelease documentation and product-surface follow-ups

- Add an end-user Keymap reference covering supported key names, binding
  syntax, primary and secondary actions, the function-key toggle, command
  catalogs, editing, and when changes are loaded.
- Review Writer's special/test insertion commands and decide whether they are
  intended end-user features or diagnostics.
- Define a clean prerelease staging procedure for `dist\` that excludes
  machine-specific shortcuts, authorization data, logs, histories, caches,
  personal Writer and Journal data, and an initialization sentinel.

## Investigate Windows application identity and icon behavior

- On some systems, `auto_core.exe` displays the standard Command Prompt icon
  when running normally, while Administrator mode displays the Auto Core icon
  correctly. Determine whether this can be corrected through AppUserModelID
  (AUMID), Registry configuration, shortcut metadata, or another Windows
  setting without requiring Administrator privileges.

## Investigate iTunes privilege compatibility and security

**Status:** Privilege matrix documented. Mixed-privilege security and compatibility review remains deferred.

The known combinations are documented in `docs/manual/components/itunes.md` and summarized in `README.md`:

- Non-admin Auto Core / non-admin iTunes: supported and recommended.
- Admin Auto Core / admin iTunes: known working. This is not the recommended runtime.
- Non-admin Auto Core / admin iTunes: compatibility issues may occur.
- Admin Auto Core / non-admin iTunes: compatibility issues may occur.

The two matching privilege combinations are known to work. Mixed privilege combinations are the compatibility concern. This does not claim a broader Windows integrity-level guarantee. Auto Core and the normal iTunes application should both run without Administrator privileges. There is no remaining elevation requirement for that configuration.

The mixed-privilege investigation remains deferred. An elevated Auto Core process has failed to connect to an iTunes instance already running without Administrator privileges, and that failure was associated with the elevation mismatch rather than startup timing. Investigate whether the iTunes component can safely control an existing iTunes process at a different privilege level, and whether a lower-privilege helper or another COM activation strategy would create unacceptable spoofing, IPC, file-access, or privilege-boundary risks. Prefer a least-privilege design. Do not weaken Windows security boundaries, and do not add an elevation workaround, merely to make the connection succeed.

### Acceptance criteria

- Document the known privilege combinations. **Done.**
- Determine whether Auto Core or its iTunes component can run without Administrator privileges. **Done.** That combination is the supported recommendation.
- If attachment across a privilege mismatch is implemented, preserve process and IPC security boundaries and add regression coverage. **Deferred.**
- Review the security implications of any mixed-privilege approach. **Deferred.**
