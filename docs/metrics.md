# Metrics

The metric module provides benchmarking and performance measurement capabilities with statistical analysis and OpenTelemetry integration.

## Table of Contents

- [Overview](#overview)
- [Measurement Contexts](#measurement-contexts)
- [Results](#results)
- [Resource Collection](#resource-collection)
- [OpenTelemetry Export](#opentelemetry-export)
- [Usage Examples](#usage-examples)

---

## Overview

The metric system is built around:

1. **Contexts** - Control how measurements are collected (time-based or iteration-based)
2. **Accumulators** - Collect samples using Welford's online algorithm for numerical stability
3. **Results** - Type-safe statistical summaries using `std::chrono` durations
4. **Sinks** - Export results to various backends (OpenTelemetry, console, etc.)

---

## Measurement Contexts

### `TimedContext`

Runs measurements for a specified duration, automatically scaling iteration counts to collect meaningful samples.

```cpp
import synodic.honesty.metric;

using namespace honesty::metric;

// Run benchmark for 1 second
TimedContext context(std::chrono::seconds(1));

Results results = context.Measure([]() {
    // Code to benchmark
});
```

### `RegressionContext`

Inherits from `TimedContext` but automatically determines the target sample duration based on clock resolution. Useful for regression testing where consistent measurement methodology is important.

```cpp
RegressionContext context;

Results results = context.Measure([]() {
    // Code to benchmark
});
```

#### Clock Resolution

`RegressionContext` automatically measures the actual clock resolution at startup (typically ~20-100ns on modern systems) and sets the target sample duration to `resolution × 1000`. This ensures each timing sample is long enough to be accurate relative to clock granularity.

You can adjust this via configuration:

```cpp
// For very fast operations, increase the multiplier for more stable results
RegressionContext context;
context.ClockResolutionMultiple(2000);  // 2000× clock resolution per sample
```

---

## Results

The `Results` struct contains statistical measurements using type-safe `std::chrono` durations:

| Field | Type | Description |
|-------|------|-------------|
| `iterations` | `std::size_t` | Total number of benchmark iterations |
| `samples` | `std::size_t` | Number of timing samples collected |
| `totalDuration` | `Duration` | Total wall-clock measurement time |
| `mean` | `StatsDuration` | Mean time per iteration |
| `variance` | `StatsDuration` | Sample variance |
| `stddev` | `StatsDuration` | Standard deviation |
| `min` | `StatsDuration` | Minimum observed time |
| `max` | `StatsDuration` | Maximum observed time |
| `iterations_per_second` | `double` | Throughput metric |

### Statistical Accumulator

Results are populated from a `BenchmarkAccumulator` which uses [synodic-template-library](https://github.com/Synodic-Software/Synodic-Template-Library)'s compile-time composable statistics:

```cpp
using BenchmarkAccumulator = synodic::statistics::Accumulator<
    double,
    synodic::statistics::count,
    synodic::statistics::sum,
    synodic::statistics::mean,
    synodic::statistics::variance,
    synodic::statistics::min,
    synodic::statistics::max>;
```

Features:
- **Welford's algorithm** for numerically stable mean and variance
- **Online computation** - no need to store all samples
- **Compile-time composition** - only pay for features you use

---

## Resource Collection

The `ResourceCollector` class provides point-in-time sampling of system resource usage (CPU, memory) for the current process. This is useful for monitoring resource consumption during benchmarks or any code execution.

### Platform Implementation

| Platform | CPU Measurement | Memory Measurement |
|----------|-----------------|-------------------|
| Windows | `GetProcessTimes` | `GetProcessMemoryInfo` |
| Linux | `getrusage` | `VmRSS` and `VmHWM` in `/proc/self/status` |
| macOS | `getrusage` | `task_info` |

### Basic Usage

```cpp
import synodic.honesty.metric;

using namespace honesty::metric;

ResourceCollector collector;
collector.Initialize();

// Take a point-in-time sample
ResourceSample sample = collector.Sample();

std::println("CPU: {:.2f}%", sample.cpuUsagePercent);
std::println("Memory: {} bytes", sample.memoryUsageBytes);
std::println("Peak Memory: {} bytes", sample.peakMemoryBytes);
```

### ResourceSample

| Field | Type | Description |
|-------|------|-------------|
| `timestamp` | `Clock::time_point` | When the sample was taken |
| `cpuUsagePercent` | `double` | Process CPU usage (0-100+, may exceed 100% on multi-core) |
| `memoryUsageBytes` | `std::size_t` | Current working set / RSS |
| `peakMemoryBytes` | `std::size_t` | Peak memory usage |

### Configuration

```cpp
ResourceConfig config;
config.collectCpu = true;     // Enable CPU collection (default: true)
config.collectMemory = true;  // Enable memory collection (default: true)

ResourceCollector collector(config);
```

### Scoped Sampling

Use `ScopedResourceSampler` to automatically capture resource usage across a scope:

```cpp
ResourceCollector collector;
collector.Initialize();

{
    ScopedResourceSampler sampler(collector);

    // ... code to measure ...

    ResourceSample delta = sampler.End();
    std::println("Memory delta: {} bytes", delta.memoryUsageBytes);
}
```

### Integration with Benchmarks

```cpp
ResourceCollector collector;
collector.Initialize();

TimedContext context(std::chrono::seconds(1));

// Sample before
ResourceSample before = collector.Sample();

Results results = context.Measure([]() {
    // Code to benchmark
});

// Sample after
ResourceSample after = collector.Sample();

std::println("Benchmark mean: {} ns", results.mean.count());
std::println("Memory used: {} bytes", after.memoryUsageBytes - before.memoryUsageBytes);
```

---

## OpenTelemetry Export

The metric module integrates with OpenTelemetry for observability. Both benchmark timing results and resource metrics can be exported.

```cpp
import synodic.honesty.metric;

using namespace honesty::metric;

// Initialize the metrics pipeline (call once at startup)
InitMetricExport(std::chrono::milliseconds(5000));

// Run benchmarks...
TimedContext context(std::chrono::seconds(1));
Results results = context.Measure(myFunction);

// Export results
ExportResults("my_benchmark", results);

// Cleanup (call at shutdown)
CleanupMetricExport();
```

### Exported Benchmark Metrics

| Metric | Type | Unit | Description |
|--------|------|------|-------------|
| `benchmark.duration` | Histogram | ns | Mean iteration duration |
| `benchmark.iterations` | Counter | - | Total iteration count |
| `benchmark.throughput` | Histogram | ops/s | Iterations per second |

All metrics include a `benchmark.name` label for identification.

### Exporting Resource Metrics

Export standalone resource samples:

```cpp
ResourceCollector collector;
collector.Initialize();

ResourceSample sample = collector.Sample();
ExportResourceSample("my_context", sample);
```

| Metric | Type | Unit | Description |
|--------|------|------|-------------|
| `process.cpu.usage` | Histogram | % | Process CPU usage |
| `process.memory.usage` | Histogram | By | Process memory (working set) |
| `process.memory.peak` | Histogram | By | Peak memory usage |

### Combined Benchmark + Resource Export

Export benchmark results with correlated resource data in a single call:

```cpp
ResourceCollector collector;
collector.Initialize();

ResourceSample before = collector.Sample();

TimedContext context(std::chrono::seconds(1));
Results results = context.Measure(myFunction);

ResourceSample after = collector.Sample();

// Export everything together with correlated labels
ExportResultsWithResources("my_benchmark", results, before, after);
```

| Metric | Type | Unit | Description |
|--------|------|------|-------------|
| `benchmark.duration` | Histogram | ns | Mean iteration duration |
| `benchmark.iterations` | Counter | - | Total iteration count |
| `benchmark.throughput` | Histogram | ops/s | Iterations per second |
| `benchmark.cpu.usage` | Histogram | % | CPU usage during benchmark |
| `benchmark.memory.usage` | Histogram | By | Memory at benchmark end |
| `benchmark.memory.delta` | Histogram | By | Memory change during benchmark |
| `benchmark.memory.peak` | Histogram | By | Peak memory during benchmark |

---

## Usage Examples

### Preventing Optimization

Use `DoNotOptimize` and `Clobber` to prevent the compiler from optimizing away benchmark code:

```cpp
import synodic.honesty.metric;

using namespace honesty::metric;

ctx.Measure([]() {
    int sum = 0;
    for (int i = 0; i < 1000; ++i) {
        sum += i;
    }
    DoNotOptimize(sum);  // Ensures sum is computed
});
```

- `DoNotOptimize(value)` - Prevents dead code elimination of computed results
- `Clobber(value)` - Prevents constant propagation; forces the compiler to treat the value as potentially modified

### Basic Benchmark

```cpp
import synodic.honesty.metric;

using namespace honesty::metric;

void benchmark_example()
{
    TimedContext context(std::chrono::milliseconds(100));

    Results results = context.Measure([]() {
        int sum = 0;
        for (int i = 0; i < 1000; ++i) {
            sum += i;
        }
        DoNotOptimize(sum);
    });

    // Access results
    auto mean_us = std::chrono::duration_cast<std::chrono::microseconds>(results.mean);
    std::println("Mean: {} μs", mean_us.count());
    std::println("Stddev: {} ns", results.stddev.count());
    std::println("Throughput: {} ops/s", results.iterations_per_second);
}
```

### Integration with Test Framework

The test framework provides first-class benchmark support via the `_benchmark` literal. Benchmarks receive an injected `RegressionContext` and results are automatically reported.

```cpp
import synodic.honesty.test;
import synodic.honesty.metric;

using namespace honesty::test;
using namespace honesty::test::literals;
using namespace honesty::metric;

Suite SUITE("benchmarks", []() -> Generator {
    // Dedicated benchmark using _benchmark literal
    co_yield "vector_push_back"_benchmark = [](RegressionContext& ctx) {
        ctx.Measure([]() {
            std::vector<int> v;
            for (int i = 0; i < 100; ++i) {
                v.push_back(i);
            }
        });
    };

    // Traditional test with manual benchmarking (for assertions on results)
    co_yield "vector_with_assertions"_test = [](const Requirements& req) {
        TimedContext context(std::chrono::milliseconds(500));

        Results results = context.Measure([]() {
            std::vector<int> v;
            for (int i = 0; i < 100; ++i) {
                v.push_back(i);
            }
        });

        // Assert performance requirements
        req.Expect(results.mean < std::chrono::microseconds(10));
    };
});
```

#### Benchmark Features

- **Automatic reporting**: Results are sent to reporters without manual code
- **Context injection**: `RegressionContext` auto-calibrates measurement duration
- **Filterable**: Benchmarks can be skipped via `ExecuteParameters::runBenchmarks = false`
- **Tag support**: Combine with tags like `SKIP / "slow_benchmark"_benchmark`

