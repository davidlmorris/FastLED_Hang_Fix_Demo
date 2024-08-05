
[//]: # (https://keepachangelog.com/en/1.1.0/)

# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## 1.1.2 - 2024-08-05

- Upped the pathology even more!  This should give Jams within a minute.

## 1.1.1 - 2024-08-05

- Upped the pathology a tad! (Ok at severe risk of guru meditation errors or worse lockups.)  But at least you get a chance to see a fastled hang earlier than typical...  finding the error sweet spot requires a bit of finessing, and you might want to play with the code a bit, since who knows, each Esp32 or board might be slightly different.

## 1.1.0 - 2024-08-05

- Added average loop count to show pathological timer interrupt load.
- Fixed time display so we no longer have decimal minutes!
- Increased the pathological condition by adding a hardware write to pins (Commented out as this will cause guru meditation as well!).
- Added change log.

## 1.0.0 - 2024-08-04

- Initial release.
