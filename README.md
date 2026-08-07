# Sift

Sift is a local-first tool that analyzes application audit events and identifies unusual behavior. It ranks activity for human review and does not label users as malicious.

## Current prototype

The prototype reads Parquet events, maps source columns through YAML, validates the input schema, and calculates daily export counts for each entity.

## Build and run

Sift requires C++23, CMake, Apache Arrow with Parquet support, and yaml-cpp.

```bash
cmake -S . -B build -G Ninja
cmake --build build
./build/sift_fixture /tmp/sift-events.parquet
./build/Sift /tmp/sift-events.parquet sift.yaml
```

Example output:

```text
user-007 daily_exports: 4 4 4 4 4 4 4 4 4 4
user-142 daily_exports: 4 4 4 4 4 4 4 4 4 20
```
