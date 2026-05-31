# PLC Signal Contract

This document defines the logical PLC contract for localization runtime.

`TaskLocalizeConfig` stores PLC tag names for each logical signal. The
`LocalizationSignalMapper` maps incoming PLC tag values to logical signal
names and maps outgoing logical signal names back to PLC tags.

Source:

- `src/model/task_localization_config.h`
- `src/model/localization_signal_mapper.*`
- `src/model/localization_runtime_controller.*`
- `src/model/localization_fault_code.h`

## Signal Names

| Signal | Type | Direction | Meaning |
| --- | --- | --- | --- |
| `nActiveCamera` | number | PLC -> task and task -> PLC | Active logical camera number. |
| `nActivePatternGroup` | number | PLC -> task and task -> PLC | Active pattern group number. |
| `nDetectedNumber` | number | task -> PLC | Number of detected possible pick positions. |
| `nFaultCode` | number | task -> PLC | Stable fault code. |
| `bCameraValid` | bool | task -> PLC | Active camera binding and calibration are valid. |
| `bPatternValid` | bool | task -> PLC | Active pattern group is usable. |
| `bTaskReady` | bool | task -> PLC | Runtime is ready to accept a new trigger. |
| `bExecuteTrigger` | bool | PLC -> task | Rising-edge cycle trigger. |
| `bMatchingFinished` | bool | task -> PLC | Accepted trigger has been handled. |
| `bMatchingBusy` | bool | task -> PLC | Cycle is currently running. |
| `bMatchingDetected` | bool | task -> PLC | Last cycle detected at least one object. |
| `bMatchingLowArea` | bool | task -> PLC | Last result was below configured work-area threshold. |
| `bTaskFault` | bool | task -> PLC | Task has an active fault or last cycle faulted. |

`nActivePattern` is intentionally not supported. Use
`nActivePatternGroup`.

## Handshake Behavior

### Ready

When runtime is healthy and ready:

```text
bTaskReady = true
bMatchingBusy = false
bMatchingFinished = false
bMatchingDetected = false
bMatchingLowArea = false
bTaskFault = false
nDetectedNumber = 0
nFaultCode = 0
```

### Trigger

`bExecuteTrigger` is rising-edge triggered. A cycle starts only when:

- previous trigger state was false
- new trigger state is true
- internal state is `ReadyForTrigger`

If the PLC keeps `bExecuteTrigger = true`, the task must not enqueue another
cycle.

### Cycle Start

When a trigger is accepted:

```text
bTaskReady = false
bMatchingBusy = true
bMatchingFinished = false
bMatchingDetected = false
bMatchingLowArea = false
bTaskFault = false
nDetectedNumber = 0
nFaultCode = 0
```

### Successful Cycle

After matching and VisionOutput send succeed:

```text
bMatchingBusy = false
bMatchingFinished = true
bMatchingDetected = (detectedNumber > 0)
bMatchingLowArea = <matchResult.isAreaLessThanLimits>
bTaskFault = false
nDetectedNumber = <detectedNumber>
nFaultCode = 0
```

If `bExecuteTrigger` is still true, the controller waits in
`WaitingTriggerReset`.

When `bExecuteTrigger` falls to false:

```text
bMatchingFinished = false
bTaskReady = true
```

### Faulted Cycle

For an abnormal cycle failure:

```text
bMatchingBusy = false
bMatchingFinished = true
bTaskReady = false
bMatchingDetected = false
bMatchingLowArea = false
nDetectedNumber = 0
bTaskFault = true
nFaultCode = <specific fault code>
```

`bMatchingFinished = true` means the accepted trigger has been handled, even
when it failed. The PLC must check `bTaskFault` and `nFaultCode`.

## Fault Codes

Stable numeric values:

| Code | Name | Meaning |
| ---: | --- | --- |
| 0 | `None` | No active fault. |
| 100 | `CameraLost` | Active camera lost connection or runner is unavailable. |
| 101 | `CameraConnectFailed` | Active camera could not connect. |
| 102 | `CameraGrabTimeout` | Single-shot grab failed, timed out, or returned no valid frame. |
| 200 | `VisionOutputLost` | Vision-output device lost connection or runner is unavailable. |
| 201 | `VisionOutputSendFailed` | Result payload could not be sent. |
| 300 | `PlcLost` | Primary PLC lost connection. |
| 400 | `PatternInvalid` | Active pattern group is missing or has no train image. |
| 401 | `CalibrationInvalid` | Active camera has no valid calibration for world-coordinate output. |
| 500 | `InternalError` | Unexpected internal runtime error. |

Do not renumber existing codes. Add new codes in a documented range and update
tests.

## PLC Write Path

Runtime output publishing calls:

- `PlcRunner::requestWriteDigitalIo(tag, value)`
- `PlcRunner::requestWriteWordIo(tag, value)`

`PlcRunner` resolves the optional `IPlcIoWriter` capability on the concrete PLC
device. `McProtocolDevice` implements the writer and parses tag families such
as `Mxxxx` and `Dxxxx`.

If an output signal has no mapped PLC tag, the runtime still emits
`signalChanged(name, value)` for UI/dashboard consumers and skips the PLC write.

## Recovery Behavior

When a required device role becomes unhealthy:

- The controller enters recovery.
- Recoverable statuses are retried according to role policy.
- If retry limit is exceeded, the controller raises a runtime fault.

If a device is lost during `Running`, the cycle is aborted with the role-specific
fault code and stale matching results are ignored.

If the PLC is disconnected, app task state and logs remain the source of truth.
On recovery, current runtime state is republished instead of silently clearing a
fault.

