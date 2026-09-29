# Pentax capture recovery by failure path

`GP_ERROR_CAMERA_BUSY` (`-110`) is not a diagnosis. This matrix records the
safe recovery for each path that can refuse a Pentax capture or leave its
output state uncertain. The safety rule is: no automatic shutter replay and
no deletion of an unowned camera candidate.

| Path/reason | Mitigation in the driver | Recovery method | Never do |
|---|---|---|---|
| `transfer_state` is non-idle | Refuse concurrent work; preserve the current owner. | Wait for that operation to complete. If its process/owner died, secure logs and candidate identity before a controlled rebind. | Start a concurrent shutter or reset the state flag. |
| GetAllConditions failed / short payload | Arm `recovery_required`; report PTP result and length; refuse before InitiateCapture. | Keep the camera connected; restore a readable PTP session and require a fresh complete strict probe. Rebind only after output ownership is known. | Infer safety from Polaris `state=1`, sleep a fixed interval, then shoot. |
| Unsafe +104, active capture +32, or pending handle +36 | Every capture now runs strict admission before InitiateCapture. Refusal logs exact fields/reason and keeps the recovery barrier armed. | For +104/+32, allow the current camera operation to settle and re-probe. For +36, preserve and recover the candidate under a proven owner/generation before another shutter. | DeleteTransferCandidate, weaken the predicate, or shoot over the candidate. |
| InitiateCapture returns non-OK (including DeviceBusy) | Arm `recovery_required`; report the PTP result; never replay the same command. | Make a later, separately requested attempt only after a fresh strict probe proves complete conditions, no active capture, no unsafe activity and no pending candidate. | Retry InitiateCapture automatically or assume the camera did nothing. |
| Cancellation/error after accepted InitiateCapture but before candidate discovery | Preserve cancellation; terminate only through the characterized capture-termination operation; verify conditions. Failed/unknown verification retains the barrier. | Wait/re-probe. If a candidate appears, switch to candidate-preservation recovery. | Use uncharacterized InterruptFunction or clear recovery state after failed verification. |
| Failure after candidate discovery, during transfer/finalization/publication | Preserve the candidate and set `recovery_required`. | Recover/transfer with the owning capture generation before admitting another shutter. If ownership cannot be proven, stop and preserve. | Generic cleanup deletion or attributing the old image to a new request. |
| Failure before `[stage2-trace] capture-enter` | This is above `gp_camera_capture`; the camlib cannot repair it. | Use pgphoto/Stage-2 request, session-generation and operation-owner logs. Rebind only when no exposure/output owner remains. | Add a Pentax sleep/retry workaround. |

## Diagnostic contract

Refusals emit `[pentax-recovery]` to stderr with a stable path/reason, the
capture ID, relevant PTP result and condition fields, and the recovery action.
The pre-shutter probe is now unconditional, so a fresh session cannot bypass
activity/candidate checks that previously ran only after `recovery_required`
had been armed.

Polaris' generic `state=1` does not prove the Pentax conditions predicate. The
candidate-path reported by Polaris is also not proof of a camera-side or
published file. Correlate Stage-2 capture-enter/return, Pentax admission,
InitiateCapture result, candidate +36, file transfer/publication, and USB/session
generation to identify the first failing boundary.

There is intentionally no automatic recovery transfer for an orphan candidate
whose request owner has died. Publishing it requires a distinct, generation-
aware Polaris API/IPC contract. Until that is implemented and tested, the safe
recovery is preserve-and-refuse.
