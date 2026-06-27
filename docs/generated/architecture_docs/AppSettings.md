# AppSettings Singleton Documentation

## Overview

`AppSettings` is a thread-safe singleton that persists application-wide preferences to disk with integrity protection and encryption.

Features:
- **Singleton pattern**: Single instance shared across the entire application
- **XOR obfuscation**: Payload obfuscated with compile-time key
- **SHA-256 checksum**: Detects file corruption or tampering
- **CBOR encoding**: Compact binary format for efficient storage
- **Backward compatibility**: Schema versioning for future migrations
- **Type-safe accessors**: Compile-time keys prevent typos

## Location

- **Header**: `src/app_settings/app_settings.h`
- **Implementation**: `src/app_settings/app_settings.cpp`
- **Base Class**: `QObject`

## Dependencies

**Qt Modules:**
- QtCore (QObject, QVariant, QVariantMap)

**External:**
- CBOR encoding/decoding (Qt internal)
- SHA-256 hashing

## File Format

The settings file is stored in the OS app-data directory with the following binary format:

| Offset | Size | Field | Value |
|--------|------|-------|-------|
| 0 | 4 bytes | Magic number | `0x4E435253` (ASCII: "NCRS") |
| 4 | 4 bytes | Schema version | Current version (uint32, big-endian) |
| 8 | 32 bytes | SHA-256 checksum | Checksum of plain (pre-obfuscation) CBOR payload |
| 40 | N bytes | XOR-obfuscated payload | CBOR-encoded QVariantMap |

**Note:** All multi-byte integers use big-endian byte order.

## Singleton Access

#### #### static AppSettings* instance()
Returns the global singleton instance. Creates the instance on first call.

**Returns:**
- Pointer to the global `AppSettings` instance (never nullptr)

**Usage:**
```cpp
auto settings = AppSettings::instance();
settings->setTheme("dark");
```

## Known Settings (Type-Safe Accessors)

### Theme

#### #### QString theme() const
Returns the current UI theme identifier.

**Default value:** `"light"`

**Returns:**
- Theme name (e.g., "light", "dark")

#### #### void setTheme(const QString& styleId)
Sets the UI theme and persists it to disk.

**Emits:** `settingChanged("app.theme", styleId)` signal

### Language / Locale

#### #### QString language() const
Returns the current application language/locale code.

**Default value:** System locale (e.g., "en_US", "ja_JP")

**Returns:**
- Locale code (POSIX format: language_COUNTRY)

#### #### void setLanguage(const QString& localeCode)
Sets the application language and persists it to disk.

**Emits:** `settingChanged("app.language", localeCode)` signal

### File Access Directories

#### #### QString lastFolderAccessDir() const
Returns the last directory accessed in a file/folder browser dialog.

**Default value:** User's home directory

**Returns:**
- Absolute file path

#### #### void setLastFolderAccessDir(const QString& dir)
Saves the last accessed directory for folder dialogs.

**Emits:** `settingChanged("app.lastFolderAccessDir", dir)` signal

#### #### QString lastImageAccessDir() const
Returns the last directory accessed in an image picker dialog.

**Default value:** User's pictures directory (or home if not available)

**Returns:**
- Absolute file path

#### #### void setLastImageAccessDir(const QString& dir)
Saves the last accessed directory for image dialogs.

**Emits:** `settingChanged("app.lastImageAccessDir", dir)` signal

## Generic API for Future Settings

#### #### QVariant value(const QString& key, const QVariant& fallback = {}) const
Retrieves a setting value by key. Used for settings not yet implemented as typed accessors.

**Parameters:**
- `key` — Setting key (e.g., "app.myCustomSetting")
- `fallback` — Default value if key is not found

**Returns:**
- Setting value, or `fallback` if not found

**Example:**
```cpp
int maxAttempts = settings->value("system.maxAttempts", 3).toInt();
```

#### #### void setValue(const QString& key, const QVariant& val)
Stores a setting value by key and persists it to disk.

**Parameters:**
- `key` — Setting key
- `val` — Value to store (any QVariant-compatible type)

**Emits:** `settingChanged(key, val)` signal

**Example:**
```cpp
settings->setValue("system.maxAttempts", 5);
```

## Signals

| Signal | Parameters | Emitted When |
|--------|-----------|--------------|
| `settingChanged(QString key, QVariant newValue)` | Key and value | Any setting is changed and persisted |

## Extending with New Settings

To add a new setting:

1. **Add a key constant** in the `AppKey::` namespace:
   ```cpp
   namespace AppKey {
       inline constexpr char MyNewSetting[] = "app.myNewSetting";
   }
   ```

2. **Add a default value** in the constructor:
   ```cpp
   m_defaults[AppKey::MyNewSetting] = "default_value";
   ```

3. **Add typed accessors** (getter/setter pair):
   ```cpp
   QString myNewSetting() const;
   void setMyNewSetting(const QString& val);
   ```

4. **Implement the accessors**:
   ```cpp
   QString AppSettings::myNewSetting() const {
       return value(AppKey::MyNewSetting, "default_value").toString();
   }
   
   void AppSettings::setMyNewSetting(const QString& val) {
       setValue(AppKey::MyNewSetting, val);
   }
   ```

## Private Methods

#### #### void load()
Loads settings from disk. Called in the constructor.

**Side Effects:**
- Reads the settings file from OS app-data directory
- Decodes and deobfuscates the payload
- Verifies SHA-256 checksum
- Silently resets to defaults if file is corrupted

#### #### void save() const
Persists settings to disk. Called after each `setValue()` call.

**Side Effects:**
- Encodes and obfuscates the payload
- Computes SHA-256 checksum
- Writes to the settings file

#### #### static QByteArray encode(const QVariantMap& map)
Encodes a QVariantMap to CBOR format and obfuscates with XOR.

**Parameters:**
- `map` — Settings map to encode

**Returns:**
- Encoded and obfuscated byte array

#### #### static bool decode(const QByteArray& raw, QVariantMap& out)
Decodes and deobfuscates a CBOR byte array, verifying SHA-256.

**Parameters:**
- `raw` — Encoded byte array (with magic, version, checksum)
- `out` — Output settings map (filled on success)

**Returns:**
- `true` if decode succeeded and checksum is valid
- `false` if format is invalid or checksum failed

#### #### static QByteArray obfuscate(const QByteArray& data)
XOR-obfuscates a byte array with a compile-time key. Symmetric operation.

**Parameters:**
- `data` — Data to obfuscate (or deobfuscate)

**Returns:**
- Obfuscated data

#### #### static QString filePath()
Returns the full path to the settings file (in OS app-data directory).

**Returns:**
- Absolute file path

## Usage Example

```cpp
// Access the singleton
auto settings = AppSettings::instance();

// Read a setting
QString currentTheme = settings->theme();
qDebug() << "Current theme:" << currentTheme;

// Change a setting (automatically persists)
settings->setTheme("dark");

// Listen for changes
connect(settings, &AppSettings::settingChanged,
        this, [](const QString& key, const QVariant& val) {
    qDebug() << "Setting changed:" << key << "=" << val;
});

// Use generic API for custom settings
settings->setValue("myapp.customOption", 42);
int customOption = settings->value("myapp.customOption", 0).toInt();

// Set working directories
settings->setLastFolderAccessDir("/home/user/projects");
settings->setLastImageAccessDir("/home/user/images");

// On app startup, recall last directories
QString lastFolder = settings->lastFolderAccessDir();
QString lastImages = settings->lastImageAccessDir();
```

## Thread Safety

- **Singleton creation**: Thread-safe via Qt's static initialization
- **Getters**: Thread-safe (read-only access)
- **Setters**: Not thread-safe; call from main thread only
- **Signals**: Emitted on the main thread

## Privacy & Security

- **Obfuscation**: XOR with compile-time key (not encryption, but sufficient for user preferences)
- **Checksum**: SHA-256 ensures file integrity
- **Magic number**: Prevents accidental use of corrupted or unrelated files
- **Silent recovery**: Corrupted files are silently reset to defaults

**Note:** This is *not* cryptographic security; do not store sensitive data (passwords, keys) in AppSettings.

## Storage Location

| Platform | Location |
|----------|----------|
| Windows | `%APPDATA%\NCR Picking\settings.dat` |
| macOS | `~/Library/Application Support/NCR Picking/settings.dat` |
| Linux | `~/.config/NCR Picking/settings.dat` |

## Related Classes

- **Project** (`model/project.h`) — Uses AppSettings for theme and language
- **ThemeManager** (`utils/theme_manager.h`) — Applies theme changes

