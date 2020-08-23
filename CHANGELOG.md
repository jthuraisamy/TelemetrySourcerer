# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## Unreleased

### Added

- Added column sorting for tables.
- Added highlighting for the `Microsoft-Windows-Sysmon` ETW provider.

### Changed

- Disabled ETW providers will no longer disappear, but will be marked as disabled.

## [0.9.1] - 2020-07-31

### Changed

- Fixed elevation check so the driver loads when the SYSTEM account is used.

## [0.9.0] - 2020-07-31

### Added

- Initial release.

[0.9.1]: https://github.com/jthuraisamy/TelemetrySourcerer/compare/v0.9.0...v0.9.1
[0.9.0]: https://github.com/jthuraisamy/TelemetrySourcerer/releases/tag/v0.9.0
