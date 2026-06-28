# Vision Widgets Planning Questions

**Purpose:** collect product and technical answers before planning the vision widget implementation.

**Implementation handoff:** [vision_widgets_implementation_plan.md](vision_widgets_implementation_plan.md)

**How to use:** answer directly under each `Your answer:` line. Short answers are fine; unknowns can be marked `TBD`.

## 1. Target Workflow

Which workflow should be implemented first?

- Pattern setup/editing offline
- Runtime dashboard read-only visualization
- Task/device settings
- All of the above, but phased

Your answer:
- Should be pattern setup/editing offline and then Runtime dashboard read-only visualization.

## 2. Required Widgets

Which widgets are required in the first implementation phase?

- ROI editor
- Polygon mask editor
- Result overlay viewer
- Image/layer viewer
- Numeric inspector for coordinates
- Tool palette for draw/edit/delete
- Other

Your answer:
- ROI editor, Result overlay viewer, Image/layer viewer, Numeric inspector for coordinates.
- Tool palette for draw/edit/delete is included in this phase.
- Polygon mask editor is for the next phase.

PM clarification:
- Tool palette is the canvas command surface for select/move, draw ROI, edit handles, delete selected item, pan/zoom/fit, undo/redo, and overlay visibility toggles. It is frontend UI control only, not backend logic.


## 3. ROI Types

Which ROI shapes are needed?

- Axis-aligned rectangle
- Rotated rectangle
- Circle/ellipse
- Free polygon
- Multiple ROIs per image

Your answer:
- Phase 1: axis-aligned rectangle ROI and rotated rectangle ROI.
- Later phases: circle/ellipse, free polygon, multiple ROIs per image, and hierarchical ROI for inspection tasks.

## 4. Polygon Mask Behavior

What does polygon mask mean in this project?

- Exclude masked area from matching/detection
- Include only masked area
- Multiple masks
- Mask per camera
- Mask per pattern/group

Your answer:
- Include only masked area for matching and detection, multiple masks, mask per pattern but will implement in the next phase


## 5. Existing Backend Contract

Which existing model/config classes should own the data?

Potential candidates:

- `TaskLocalization`
- `TaskLocalizeConfig`
- `MatchConfig`
- `MatchPatternConfig`
- `ImageMatcher`
- Other

Your answer:
- this plan for front end only, keep backend as designed before.


## 6. Persistence Format

Where should ROI/mask settings be saved?

- Project file
- Task config
- Pattern config
- Runtime only
- Existing JSON schema
- New schema

Your answer:
- Use existing field/config if it is already available.
- If no suitable existing field/config exists, do not change backend ownership in phase 1. Define a UI-facing adapter/serialization boundary and document the missing backend integration as follow-up.


## 7. Image Source

What images should the editor/viewer support?

- Live camera frame
- Pattern image
- Last runtime result image
- Loaded local image file
- Debug/binary image layer

Your answer:
- Depends on the caller widget.
- Pattern setup should support manual image loading and raw image from camera.
- Runtime result viewer should use raw image from the runtime path and render overlays in the frontend widget.

## 8. Coordinate System

Which coordinate systems must be visible or editable?

- Image pixel coordinates
- ROI-local coordinates
- World/robot coordinates
- Calibration board coordinates

Your answer:
- Image pixel coordinates


## 9. Interaction Requirements

Which mouse/keyboard interactions are required?

- Drag to move ROI
- Resize handles
- Rotate handle
- Add/remove polygon point
- Pan/zoom
- Fit to view
- Undo/redo
- Copy/paste ROI or mask
- Numeric edit

Your answer:
- Drag to move ROI
- Resize handles
- Rotate handle
- Add/remove polygon point
- Pan/zoom
- Fit to view
- Undo/redo
- Copy/paste ROI or mask
- Numeric edit


## 10. Result Visualization

What result overlays are needed?

- Bounding box
- Picking point
- Rotation angle
- Score label
- Pattern/group id
- Rejected candidates
- Fault/invalid markers
- Sent-to-output marker
- Runtime signal values

Your answer:
- Bounding box
- Picking point
- Score label, Rotation angle, Pattern/group id, Rejected candidates, Fault/invalid markers, Sent-to-output marker, Runtime signal values: option for user choose to show.

## 11. Performance Constraints

What image sizes and runtime frequency should be assumed?

Examples:

- 1920x1080 still images
- 5MP camera frames
- 10 FPS live preview
- Only last result, no streaming

Your answer:
- resolution depend on camera.
- Only last result, no streaming


## 12. UI Placement

Where should the widgets live?

- `LocalizationPatternsWidget`
- `LocalizationDashboardWidget`
- `LocalizationSettingWidget`
- Dialog/wizard
- Reusable standalone widget under `src/widgets`

Your answer:
- Reusable standalone widget under `src/widgets`

## 13. First Acceptance Criteria

What would make phase 1 acceptable?

Examples:

- Can draw/edit/save one rectangle ROI on a pattern image
- Can draw/edit/save multiple polygon masks
- Can show runtime result overlays read-only
- Can reload saved config and reproduce overlay

Your answer:
- Can draw/edit/save one axis-aligned rectangle ROI and one rotated rectangle ROI on a pattern image
- Can show runtime result overlays read-only
- Can reload saved config and reproduce overlay

## 14. Out Of Scope

What should explicitly not be implemented in the first phase?

Your answer:
- Phase đầu tiên không chỉnh sửa backend, chỉ tạo ra các generic widget có khả năng reuse, và thay thế cho các widget hoặc canvas hiện tại đang dùng. Ví dụ như show kết quả matching, hiện tại do ImageMatcher trả ra ảnh kết quả, nhưng kết thúc phase thì sẽ không còn dùng ảnh cho ImageMatcher cung cấp nữa mà sẽ dùng ảnh raw và widget show kết quả sẽ render kết quả.
- It is allowed to create UI-facing result DTOs/adapters so the frontend widget can render overlays without depending on result images generated by `ImageMatcher`.

## PM Notes

Recommended implementation order after answers are confirmed:

1. Define UI-facing data contracts for ROI and result overlays.
2. Build a reusable vision canvas with stable image-coordinate transforms.
3. Add tool palette, numeric inspector, and layer/overlay toggles.
4. Add axis-aligned rectangle and rotated rectangle ROI editing.
5. Add read-only runtime result overlay rendering.
6. Integrate first into pattern setup, then runtime result dashboard.
7. Wire existing config persistence where available and document any missing backend integration.
8. Add focused tests/architecture contracts.

## Confirmed Phase 1 Scope

- Build reusable frontend widgets under `src/widgets`.
- Replace the current pattern setup canvas/viewer path first.
- Replace the runtime result display path so raw images plus frontend-rendered overlays are used instead of result images generated by `ImageMatcher`.
- Use `ImageViewOnly`/`ImageWidget` according to the existing architecture only when they remain useful; otherwise introduce the new reusable vision widgets and migrate callers.
- Include tool palette in phase 1.
- Include axis-aligned rectangle ROI and rotated rectangle ROI in phase 1.
- Keep polygon masks, circle/ellipse ROI, free polygon ROI, multiple ROI workflows, and hierarchical ROI for later phases unless a later plan explicitly pulls them forward.
