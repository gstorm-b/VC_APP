# ADR-0002: SI units (meter/radian) with no implicit conversion

## Status
Accepted

## Date
2026-06-19

## Context
Robotics mixes unit systems constantly: CAD and teach pendants speak millimeters and degrees;
math libraries and most kinematics literature use meters and radians. Silent or ambiguous
conversions are a classic source of bugs that are off by exactly 1000× or 57.3× and pass code
review because the types look fine.

## Decision
All core values are **SI**: meters for length, radians for angle. The API performs **no
implicit unit conversion**. Conversions exist only at explicit, unit-named boundaries:
`units::mm/deg/toMm/toDeg`, `JointVector::fromDegrees/toDegrees`, and the `*_mm_deg` /
`translation_m()` `Pose` helpers. A function/parameter name always makes the unit unambiguous.

## Alternatives Considered
- **A `Unit`-tagged scalar type** (e.g. strong typedefs for meters vs millimeters). Pros:
  compiler-enforced. Cons: heavy ergonomics cost across an Eigen-based API, friction with raw
  `Eigen::Vector3d`. Rejected as overkill for this library's surface.
- **Accept mm/deg at the boundary and auto-convert internally.** Rejected: "auto" is exactly the
  ambiguity we want to eliminate; explicit named helpers are clearer and cheap.

## Consequences
- Reading the call site tells you the units. If a result is 1000× off, you mixed mm and m.
- Presets and JSON are SI-only (`units: { length: "m", angle: "rad" }`); the loader rejects
  other unit declarations.
- A small amount of boilerplate at IO boundaries, traded for the elimination of an entire bug
  class.
