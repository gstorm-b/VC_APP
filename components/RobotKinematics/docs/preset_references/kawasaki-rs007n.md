# Kawasaki RS007N Preset Reference

## Status

Blocked. Do not implement the Kawasaki RS007N preset until verified source data is provided.

## Required Source Data

Provide one or more trusted sources for each category below.

### Robot Geometry

- Link lengths and offsets.
- Joint axis directions.
- Joint origin transforms or an equivalent DH/URDF/vendor model.
- Flange definition.
- Base frame definition.

### Joint Limits

- J1 lower/upper.
- J2 lower/upper.
- J3 lower/upper.
- J4 lower/upper.
- J5 lower/upper.
- J6 lower/upper.
- Units used by the source.

### Posture Rules

- Lefty/righty definition.
- Above/below definition.
- Flip/non-flip definition.
- Whether each rule is based on joint sign, geometric branch, or controller-specific state.

### Validation Points

At least 10 non-singular joint/pose pairs are recommended.

For each point:

- Joint vector J1..J6.
- TCP or flange pose.
- Tool offset used during measurement.
- Reference frame used during measurement.
- Orientation convention and order, for example RZ/RY/RX or RX/RY/RZ.
- Coordinate decimal precision. If the pose/coordinate source is rounded to 2 decimal places, tests may use the documented relaxed joint-angle comparison tolerance of `0.001 degree`; otherwise use the default `0.0001 degree` joint-angle comparison tolerance.

### Source References

Record exact references:

- Manual name and revision.
- Page/table numbers.
- Controller export file name, if used.
- Teach-pendant measurement date, if measured manually.
- Any assumptions or inferred values.

## Acceptance Gate

Task 6.1 can start when:

- Geometry and joint limits have source references.
- Posture rules are defined or explicitly marked unknown.
- Validation points include orientation convention.
- Validation points record coordinate precision.
- The user confirms the data is the intended source of truth.
