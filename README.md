# Sift

Sift analyzes application audit events. It prepares behavior measurements for human review. It does not label users as malicious.

## Current prototype

Sift reads Parquet events and uses YAML to map source columns. It validates the input schema and normalizes the events. It then calculates deterministic export measurements for each entity.

The native detection engine is still in development. The experimental Jev mode can send measurements to Jev.

## Build and run

Sift requires C++23, CMake, Apache Arrow with Parquet support, and yaml-cpp.

```bash
cmake -S . -B build -G Ninja
cmake --build build
./build/sift_fixture /tmp/sift-events.parquet
./build/Sift /tmp/sift-events.parquet sift.yaml
```

Sift uses `native` mode by default. Native mode keeps the current output:

```text
user-007 daily_exports: 4 4 4 4 4 4 4 4 4 4
user-142 daily_exports: 4 4 4 4 4 4 4 4 4 20
```

## Analysis modes

Select a mode in `sift.yaml`:

```yaml
analysis:
  mode: native
```

Use `--analysis-mode` to override the YAML value:

```bash
./build/Sift /tmp/sift-events.parquet sift.yaml --analysis-mode jev
```

The command line value has priority. If `analysis` is not present, Sift uses `native` mode.

## Jev mode

Jev mode uses the TypeSafe System One API. This document calls it the System One API. This mode is experimental.

Sift calculates descriptive measurements before it sends data to Jev. Jev receives the measurements and returns one typed `choice` judgment.

A Jev judgment is not a finding, rank, priority, or severity.

The current `Event` schema contains a timestamp, an event type, and a primary entity ID. It contains little semantic resource context. Thus, the first Jev judgment only characterizes the supplied measurements.

### Jev support

CMake disables Jev support by default. To build `sift_jev`, use these commands:

```bash
cmake -S . -B build -G Ninja -DSIFT_ENABLE_JEV=ON
cmake --build build
```

The Jev source uses libcurl and nlohmann/json. When you build Sift without Jev support, Sift does not use the Jev source files.

If you select `jev` without Jev support, Sift stops and reports an error. Sift does not change to `native` mode.

### Credentials

Set the TypeSafe API key in the environment:

```bash
export TYPESAFE_API_KEY=...
```

Do not write the API key in `sift.yaml`. If the key is not set, Jev mode stops before it sends a request. Native mode does not read the key.

You can set optional Jev values in YAML:

```yaml
analysis:
  mode: jev
  jev:
    model: jev-1.13.0
    timeout_ms: 10000
    max_retries: 2
```

The default API endpoint is `https://api.typesafe.ai`. For a local mock server, set `SIFT_TYPESAFE_BASE_URL`.

## Tests

Run the normal test suite:

```bash
ctest --test-dir build --output-on-failure
```

Normal tests do not send a live API request. Jev unit tests use a fake transport.

Live Jev tests need an explicit CMake option, a runtime flag, and a real API key:

```bash
cmake -S . -B build -G Ninja -DSIFT_ENABLE_JEV=ON -DSIFT_RUN_LIVE_JEV_TESTS=ON
export TYPESAFE_API_KEY=...
export SIFT_RUN_LIVE_JEV_TESTS=1
ctest --test-dir build -L jev-live --output-on-failure
```

Live tests can use paid API capacity.
