# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [0.11.0] - 2021-03-10

### Added

- Added support for Windows 10 20H2.

### Changed

- Fixed code correctness issues (h/t to [@JohnLaTwC](https://twitter.com/JohnLaTwC)).

## [0.10.0] - 2020-08-31

### Added

- Added column sorting for tables.
- Added highlighting for more ETW providers.

### Changed

- Notable list items will appear on top by default.
- Fixed bug where device handles were not closed after usage (h/t to [@DanShaqFu](https://twitter.com/DanShaqFu/status/1299322813640253442)).

## [0.9.1] - 2020-07-31

### Changed

- Fixed elevation check so the driver loads when the SYSTEM account is used.

## [0.9.0] - 2020-07-31

### Added

- Initial release.

[0.10.0]: https://github.com/jthuraisamy/TelemetrySourcerer/compare/v0.9.1...v0.10.0
[0.9.1]: https://github.com/jthuraisamy/TelemetrySourcerer/compare/v0.9.0...v0.9.1
[0.9.0]: https://github.com/jthuraisamy/TelemetrySourcerer/releases/tag/v0.9.0
