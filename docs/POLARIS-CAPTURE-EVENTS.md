# Polaris local JPEG publication events

This branch adds a best-effort publication ledger hook to `gp_file_save()`.
After a JPEG under `/app/sd/` has been written and its camera mtime restored,
libgphoto2 appends:

```text
epoch<TAB>absolute-path<TAB>bytes<TAB>mtime
```

The default ledger is `/app/sd/polaris-astro/capture-events.tsv`. A process may
override that ledger with `POLARIS_CAPTURE_EVENT_LEDGER`. The optional
`POLARIS_CAPTURE_EVENT_MEDIA_ROOT` exists for deterministic tests and
alternate mounted media; it must end in `/`. With no override, the published
file root remains restricted to `/app/sd/` so ordinary desktop saves do not
emit Polaris events.

The hook is telemetry only. A ledger failure does not turn a successful camera
download into a libgphoto2 error. The Polaris watcher independently checks
file stability, JPEG structure, identity, and a byte checksum before solving.

This proves the generic `gp_file_save()` boundary only. It does not prove that
every pgphoto capture mode uses that boundary; device qualification must still
capture a JPEG in each supported Astro mode and verify the matching ledger row.
