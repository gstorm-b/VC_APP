# Localization Setting Widget

- `cbb_vision_output_device` selects the assigned `VisionOutput` device used by
  the localization task. The selected device id is written to
  `TaskLocalizeConfigPrivate::m_deviceBindings` with role `vision_output`.
- `cbb_comm_device` selects the assigned PLC communication device. The selected
  device id is written to `TaskLocalizeConfigPrivate::m_deviceBindings` with
  role `primary_plc`.
- `listView_cameras_map` owns the logical camera-number mapping. Its mapping is
  stored in `TaskLocalizeConfigPrivate::m_deviceBindings` using
  `camera_number` role entries.
- `listView_signals_map` owns signal-tag mappings for the remaining
  `TaskLocalizeConfig` signal fields. Available signal tags are loaded from the
  selected communication device through PLC tag capability interfaces:
  `IDigitalIoProvider::availableDigitalIoNames()` for boolean rows and
  `IWordIoProvider::availableWordIoNames()` for number rows.
- If the selected communication device changes and a previous signal tag is not
  available anymore, the widget may display the stale value with warning style.
  A later validation trigger should call `SignalsMapWidget::checkEmpty()`.
