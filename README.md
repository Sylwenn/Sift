# Sift

Sift is a local-first tool that analyzes application audit events and identifies unusual behavior. It ranks activity for human review and does not label users as malicious.

## Current prototype

Sift reads Parquet events, maps source columns through YAML, validates the input schema, normalizes events, and computes deterministic per-entity export measurements. It then runs the selected analysis mode.

## Build and run

Sift requires C++23, CMake, Apache Arrow with Parquet support, and yaml-cpp.

```bash
cmake -S . -B build -G Ninja
cmake --build build
./build/sift_fixture /tmp/sift-events.parquet
./build/Sift /tmp/sift-events.parquet sift.yaml
```

The default mode is `native`. Native mode keeps the current output:

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

Or override it on the command line:

```bash
./build/Sift /tmp/sift-events.parquet sift.yaml --analysis-mode jev
```

The CLI override wins over the YAML value. Native is used when `analysis` is absent.

## Jev mode (experimental)

Jev mode is an optional integration with the TypeSafe System One HTTP API. It is experimental.

Sift computes deterministic descriptive measurements first, such as the current export count, the prior observation count, the prior mean, and the current-minus-prior-max. Jev then makes one narrow typed `choice` judgment over those precomputed facts. The judgment does not become a finding, rank, priority, or severity.

The current `Event` schema has only a timestamp, an event type, and a primary entity ID. It carries almost no semantic resource context. The first Jev judgment is therefore an experimental characterization of measurements, not a rich semantic security judgment.

### Build support

Jev support is off by default. Enable it to build `sift_jev`:

```bash
cmake -S . -B build -G Ninja -DSIFT_ENABLE_JEV=ON
cmake --build build
```

This uses libcurl and a header-only JSON library. When Jev support is off, `sift_jev` and its dependencies are not built, and native mode is unchanged. Selecting `jev` without support fails with a clear message.

### Credentials

Jev mode reads the API key from the environment only:

```bash
export TYPESAFE_API_KEY=...
```

Do not put credentials in `sift.yaml`. Jev mode fails before any request when the key is missing. Native mode never reads the key.

Optional Jev settings live in YAML:

```yaml
analysis:
  mode: jev
  jev:
    model: jev-1.13.0
    timeout_ms: 10000
    max_retries: 2
```

The base endpoint defaults to `https://api.typesafe.ai`. Set `SIFT_TYPESAFE_BASE_URL` to redirect it, for example to a local mock server.

## Tests

```bash
ctest --test-dir build --output-on-failure
```

Normal tests never make a live API call. Unit tests for Jev use a fake transport.

Live Jev tests require an explicit configure option, a runtime flag, and a real key:

```bash
cmake -S . -B build -G Ninja -DSIFT_ENABLE_JEV=ON -DSIFT_RUN_LIVE_JEV_TESTS=ON
export TYPESAFE_API_KEY=...
export SIFT_RUN_LIVE_JEV_TESTS=1
ctest --test-dir build -L jev-live --output-on-failure
```

Live tests make real, paid API calls.
