# Task hook integration

Enable **Tools → Task activity → Enable task hooks** first. This records a local preference; no provider logs or prompts are read. The hook reports command lifecycle, not an AI provider's internal progress or usage.

The task page includes a **Copy command** button. After installation:

```sh
kestrel-task --label "Build" -- cmake --build build -j 4
```

The command receives its original arguments and standard input/output/error. Nonzero exit status and terminating signals are preserved. Kestrel being unavailable or disabled does not prevent the command from running.

The wrapper emits running, a heartbeat every 15 seconds, then completed/failed. A missing heartbeat for 120 seconds is shown as **stale**, never silently completed. Completed/failed entries expire after 60 seconds or earlier dismissal. Attention remains until dismissed. Up to eight task entries are retained.

## Explicit event contract

`kestrel-island --task-event-file /absolute/path/to/event.json` sends to the already-running instance. It does not start an overlay or toggle its state. The file must be a JSON object no larger than 16 KiB. Missing instance, disabled hooks, invalid data or transport failure returns nonzero.

```json
{"version":1,"id":"build:example","state":"running","label":"Build","seq":1}
```

Required fields: version 1, id (1–128 ASCII letters/numbers/`.`/`_`/`:`/`-`), state (`running`, `completed`, `failed`, `attention`), and a printable label up to 160 characters. Optional project is a basename up to 80 characters; optional seq is monotonically increasing per id. Unknown fields are rejected. Do not include prompts, command output, tokens, credentials, or private paths. Generate a new id for each run.

The interface is `/Island`, `io.github.kestrel.Island.ReportTask(a{sv}) → bool` on the session bus. This is a same-user convenience interface, not a security sandbox. Events and elapsed state remain in memory; no task history database is created.

No automatic Codex, Claude or other provider configuration is installed. A provider-specific adapter requires a documented event/hook contract and separate validation.
