# Changelog
All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](http://keepachangelog.com/en/1.0.0/)
and this project adheres to [Semantic Versioning](http://semver.org/spec/v2.0.0.html).

## [Unreleased]
- Add additional HTTP headers for call to Themis from Convey

## [1.1.4]
- on connect retry, requery jwt only if it failed before 
- put two timestamps in connection health file; start conn and current
- change health file update interval to 240sec
- use jitter in backoff delay
- sendMessage to check cloud status == ONLINE before sending
- when killed with SIGTERM, close will use msg in close reason file.

## [1.1.3]
- Add callback handler for ping status change event
- Fixed nopoll_conn_unref crash
- Update retry timestamp in connection-health-file
- fix so that parodus can be killed, even if in a connection fail loop
- provide signal handlers so we shut down properly when INCLUDE_BREAKPAD active
- send status code and reason in websocket close message
- dont try to install handler for signal 9

## [1.1.2]
- Add pause/resume heartBeatTimer
- parodus event handler to listen to interface_down and interface_up event
- Pause connection retry during interface_down event

## [1.1.1]
- Update to use nanomsg v. 1.1.4
- requestNewAuthToken will clear the token if it fails.
- request auth token on every retry, not just after 403
- update to use nopoll v 1.0.2

## [1.0.4]
- Fix re-registration fails that lose a socket
- Fix mutex error in service alive
- Security: Mutual Authentication (mTLS or two way TLS)
- Rename command line options for MTLS cert and Key

## [1.0.3]
- Security: Added support to use auth token during initial connect to cloud

## [1.0.2] - 2019-02-08
- Refactored connection.c and updated corresponding unit tests
- Additional `/cloud-status` and `/cloud-disconnect` fields.
- Switched from nanomsg (Release 1.1.2) to NNG (Release v1.0.1)
- Memory leak fixes
- Changed connection logic (connection.c) for retries, and added unit test
- Partner-id comparison made case insensitive
- Reverted from NNG to nanomag (v1.1.2)
- reverted temporary CMake reference to https://github.com/bill1600/seshat
- Added log for time difference of parodus connect time and boot time
- added NULL check for device mac id in upstream retrieve message handling
- backoff retry to include find_servers in loop (connection.c)
- backoff max is max count not max delay
- used mutex protection to make client list and nn_sends thread safe
- put mutex lock into get_global_node
- change svc alive from a thread to a function called every 30 sec from main
- shut down tasks properly
- fix memory leaks
- Fixed memory leak in upstream event message flow
- Fixed crash in CRUD request processing
- Fixed issue on RETRIEVE respone processing
- Enabled valgrind
- Fixed main loop to keep calling svc_alive_task during a cloud disconnect and retry
- change svc alive back to a separate thread.  Shut it down with pthread_cond_timedwait
- Refactored Upsteam RETRIEVE flow
- Fix re-registration to call nn_shutdown and nn_close, so we don't lose a socket.

## [1.0.1] - 2018-07-18
### Added
- Added command line arguments for secure token read and acquire.
- DNS check for <device-id>.<URL> to allow custom device handling logic (like test boxes, refurbishing, etc).
- Add check for matching `partner-id` for incoming WRP messages based on command line argument passed.
- CRUD operations to local namespace is now supported.
- Via CRUD calls, the custom metadata tag values are now supported.

### Changed
- Configurable security flag and port.
- Default HTTP port to 80 and HTTPS port to 443.
- Updates to how `nopoll` is called have been implemented.

### Fixed
- 64 bit pointer fixes (#144, #145).
- Handle `403` error from talaria (#147).
- Several additional NULL pointer checks & recovery logic blocks added.
- A scenario where ping requests are not responded to was fixed. 

### Security
- Added command line arguments for secure token read and acquire.  Token presented to cloud for authentication verification.
- Added support for override functions to support drop root capabilities for parodus process

## [1.0.0] - 2017-11-17
### Added
- Added the basic implementation.

## [0.0.1] - 2016-08-15
### Added
- Initial creation

[Unreleased]: https://github.com/Comcast/parodus/compare/1.1.4...HEAD
[1.1.4]: https://github.com/Comcast/parodus/compare/1.1.3...1.1.4
[1.1.3]: https://github.com/Comcast/parodus/compare/1.1.2...1.1.3
[1.1.2]: https://github.com/Comcast/parodus/compare/1.1.1...1.1.2
[1.1.1]: https://github.com/Comcast/parodus/compare/1.0.4...1.1.1
[1.0.4]: https://github.com/Comcast/parodus/compare/1.0.3...1.0.4
[1.0.3]: https://github.com/Comcast/parodus/compare/1.0.2...1.0.3
[1.0.2]: https://github.com/Comcast/parodus/compare/1.0.1...1.0.2
[1.0.1]: https://github.com/Comcast/parodus/compare/1.0.0...1.0.1
[1.0.0]: https://github.com/Comcast/parodus/compare/79fa7438de2b14ae64f869d52f5c127497bf9c3f...1.0.0

