import std;
import synodic.honesty.test;
import synodic.honesty.metric;

using namespace honesty::test;
using namespace honesty::test::literals;
using namespace honesty::metric;

namespace
{
	Suite SUITE(
		"resource",
		[]() -> Generator
		{
			co_yield "default_construction"_test = [](const Requirements& requirements)
			{
				ResourceCollector collector;
				requirements.Expect(!collector.IsInitialized(), "Should not be initialized before Initialize()");
			};

			co_yield "construction_with_config"_test = [](const Requirements& requirements)
			{
				ResourceConfig config;
				config.collectCpu = true;
				config.collectMemory = false;

				ResourceCollector collector(config);

				requirements.Expect(collector.Config().collectCpu, "CPU collection should be enabled");
				requirements.Expect(!collector.Config().collectMemory, "Memory collection should be disabled");
			};

			co_yield "initialization"_test = [](const Requirements& requirements)
			{
				ResourceCollector collector;

				bool success = collector.Initialize();

				requirements.Expect(success, "Initialize() should succeed");
				requirements.Expect(collector.IsInitialized(), "Should be initialized after Initialize()");
			};

			co_yield "double_initialization"_test = [](const Requirements& requirements)
			{
				ResourceCollector collector;

				bool first = collector.Initialize();
				bool second = collector.Initialize();

				requirements.Expect(first, "First Initialize() should succeed");
				requirements.Expect(second, "Second Initialize() should succeed (idempotent)");
			};

			co_yield "sample_before_initialize"_test = [](const Requirements& requirements)
			{
				ResourceCollector collector;

				ResourceSample sample = collector.Sample();

				// Should return default/zero values when not initialized
				requirements.ExpectEquals(sample.cpuUsagePercent, 0.0, "CPU should be 0 when not initialized");
				requirements.ExpectEquals(sample.memoryUsageBytes, std::size_t{0}, "Memory should be 0 when not initialized");
			};

			co_yield "sample_returns_valid_timestamp"_test = [](const Requirements& requirements)
			{
				ResourceCollector collector;
				collector.Initialize();

				auto before = Clock::now();
				ResourceSample sample = collector.Sample();
				auto after = Clock::now();

				requirements.Expect(sample.timestamp >= before, "Timestamp should be >= before");
				requirements.Expect(sample.timestamp <= after, "Timestamp should be <= after");
			};

			co_yield "sample_memory_is_positive"_test = [](const Requirements& requirements)
			{
				ResourceCollector collector;
				collector.Initialize();

				ResourceSample sample = collector.Sample();

				// Any running process should have some memory usage
				requirements.Expect(sample.memoryUsageBytes > 0, "Memory usage should be positive");
				requirements.Expect(sample.peakMemoryBytes >= sample.memoryUsageBytes,
					"Peak memory should be >= current memory");
			};

			co_yield "sample_cpu_in_valid_range"_test = [](const Requirements& requirements)
			{
				ResourceCollector collector;
				collector.Initialize();

				// First sample establishes baseline
				collector.Sample();

				// Do some work to generate CPU usage
				std::uint64_t sum = 0;
				for (std::uint64_t i = 0; i < 100000; ++i)
				{
					sum += i;
				}
				DoNotOptimize(sum);

				ResourceSample sample = collector.Sample();

				// CPU should be non-negative (can be 0 if measured instantly)
				requirements.Expect(sample.cpuUsagePercent >= 0.0, "CPU usage should be non-negative");
			};

			co_yield "scoped_sampler_captures_start"_test = [](const Requirements& requirements)
			{
				ResourceCollector collector;
				collector.Initialize();

				auto before = Clock::now();
				ScopedResourceSampler sampler(collector);
				auto after = Clock::now();

				const auto& start = sampler.StartSample();
				requirements.Expect(start.timestamp >= before, "Start timestamp should be >= before");
				requirements.Expect(start.timestamp <= after, "Start timestamp should be <= after");
			};

			co_yield "scoped_sampler_end_returns_delta"_test = [](const Requirements& requirements)
			{
				ResourceCollector collector;
				collector.Initialize();

				ScopedResourceSampler sampler(collector);

				// Allocate some memory to create a measurable delta
				std::vector<int> data(10000);
				DoNotOptimize(data.data());

				ResourceSample delta = sampler.End();

				// Timestamp should be valid
				requirements.Expect(delta.timestamp >= sampler.StartSample().timestamp,
					"End timestamp should be >= start timestamp");
			};

			co_yield "move_construction"_test = [](const Requirements& requirements)
			{
				ResourceCollector original;
				original.Initialize();

				ResourceCollector moved(std::move(original));

				requirements.Expect(moved.IsInitialized(), "Moved-to collector should be initialized");
			};

			co_yield "move_assignment"_test = [](const Requirements& requirements)
			{
				ResourceCollector original;
				original.Initialize();

				ResourceCollector target;
				target = std::move(original);

				requirements.Expect(target.IsInitialized(), "Move-assigned collector should be initialized");
			};

			co_yield "cpu_only_config"_test = [](const Requirements& requirements)
			{
				ResourceConfig config;
				config.collectCpu = true;
				config.collectMemory = false;

				ResourceCollector collector(config);
				collector.Initialize();

				ResourceSample sample = collector.Sample();

				// Memory should be 0 when collection is disabled
				requirements.ExpectEquals(sample.memoryUsageBytes, std::size_t{0},
					"Memory should be 0 when collection disabled");
			};

			co_yield "memory_only_config"_test = [](const Requirements& requirements)
			{
				ResourceConfig config;
				config.collectCpu = false;
				config.collectMemory = true;

				ResourceCollector collector(config);
				collector.Initialize();

				// First sample to establish CPU baseline (even though disabled)
				collector.Sample();

				ResourceSample sample = collector.Sample();

				// CPU should be 0 when collection is disabled
				requirements.ExpectEquals(sample.cpuUsagePercent, 0.0,
					"CPU should be 0 when collection disabled");
				// Memory should still be collected
				requirements.Expect(sample.memoryUsageBytes > 0,
					"Memory should be positive when collection enabled");
			};
		});

	SuiteRegistrar _(SUITE);
}
