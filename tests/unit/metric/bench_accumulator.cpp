import std;
import synodic.honesty.test;
import synodic.honesty.metric;
import synodic.periapsis;

using namespace honesty::test;
using namespace honesty::test::literals;
using namespace honesty::metric;

namespace
{
	// Baseline naive implementation for comparison
	// Uses simple sum-then-divide for mean, two-pass for variance
	class NaiveAccumulator
	{
	public:
		void Push(double value)
		{
			values_.push_back(value);
		}

		std::size_t Count() const { return values_.size(); }

		double Sum() const
		{
			double sum = 0.0;
			for (double v : values_) sum += v;
			return sum;
		}

		double Mean() const
		{
			if (values_.empty()) return 0.0;
			return Sum() / static_cast<double>(values_.size());
		}

		double Variance() const
		{
			if (values_.size() < 2) return 0.0;
			double m = Mean();
			double sum = 0.0;
			for (double v : values_)
			{
				double diff = v - m;
				sum += diff * diff;
			}
			return sum / static_cast<double>(values_.size() - 1);
		}

		double Min() const
		{
			if (values_.empty()) return 0.0;
			return *std::ranges::min_element(values_);
		}

		double Max() const
		{
			if (values_.empty()) return 0.0;
			return *std::ranges::max_element(values_);
		}

	private:
		std::vector<double> values_;
	};

	// Online accumulator baseline (Welford's algorithm, manual implementation)
	class OnlineAccumulator
	{
	public:
		void Push(double value)
		{
			++count_;
			double delta = value - mean_;
			mean_ += delta / static_cast<double>(count_);
			double delta2 = value - mean_;
			m2_ += delta * delta2;

			if (count_ == 1 || value < min_) min_ = value;
			if (count_ == 1 || value > max_) max_ = value;
		}

		std::size_t Count() const { return count_; }
		double Mean() const { return mean_; }
		double Variance() const { return count_ > 1 ? m2_ / static_cast<double>(count_ - 1) : 0.0; }
		double Min() const { return min_; }
		double Max() const { return max_; }

	private:
		std::size_t count_ = 0;
		double mean_ = 0.0;
		double m2_ = 0.0;
		double min_ = 0.0;
		double max_ = 0.0;
	};

	Suite SUITE(
		"benchmark_accumulator",
		[]() -> Generator
		{
			// ============================================================
			// Single Push Benchmarks
			// ============================================================

			co_yield "single_push"_bench = []() -> BenchmarkGenerator
			{
				co_yield BASELINE / "naive"_bench = [](RegressionContext& ctx)
				{
					NaiveAccumulator accumulator;
					double value = 42.5;

					ctx.Measure([&]()
					{
						accumulator.Push(value);
					});
				};

				co_yield "online"_bench = [](RegressionContext& ctx)
				{
					OnlineAccumulator accumulator;
					double value = 42.5;

					ctx.Measure([&]()
					{
						accumulator.Push(value);
					});
				};

				co_yield "benchmark_accumulator"_bench = [](RegressionContext& ctx)
				{
					BenchmarkAccumulator accumulator;
					double value = 42.5;

					ctx.Measure([&]()
					{
						accumulator.Push(value);
					});
				};
			};

			// ============================================================
			// Batch Push Benchmarks (10 values)
			// ============================================================

			co_yield "batch_push_10"_bench = []() -> BenchmarkGenerator
			{
				co_yield BASELINE / "naive"_bench = [](RegressionContext& ctx)
				{
					ctx.Measure([]()
					{
						NaiveAccumulator accumulator;
						for (int i = 0; i < 10; ++i)
						{
							accumulator.Push(static_cast<double>(i) * 1.5);
						}
					});
				};

				co_yield "online"_bench = [](RegressionContext& ctx)
				{
					ctx.Measure([]()
					{
						OnlineAccumulator accumulator;
						for (int i = 0; i < 10; ++i)
						{
							accumulator.Push(static_cast<double>(i) * 1.5);
						}
					});
				};

				co_yield "benchmark_accumulator"_bench = [](RegressionContext& ctx)
				{
					ctx.Measure([]()
					{
						BenchmarkAccumulator accumulator;
						for (int i = 0; i < 10; ++i)
						{
							accumulator.Push(static_cast<double>(i) * 1.5);
						}
					});
				};
			};

			// ============================================================
			// Batch Push Benchmarks (100 values)
			// ============================================================

			co_yield "batch_push_100"_bench = []() -> BenchmarkGenerator
			{
				co_yield BASELINE / "naive"_bench = [](RegressionContext& ctx)
				{
					ctx.Measure([]()
					{
						NaiveAccumulator accumulator;
						for (int i = 0; i < 100; ++i)
						{
							accumulator.Push(static_cast<double>(i) * 1.5);
						}
					});
				};

				co_yield "online"_bench = [](RegressionContext& ctx)
				{
					ctx.Measure([]()
					{
						OnlineAccumulator accumulator;
						for (int i = 0; i < 100; ++i)
						{
							accumulator.Push(static_cast<double>(i) * 1.5);
						}
					});
				};

				co_yield "benchmark_accumulator"_bench = [](RegressionContext& ctx)
				{
					ctx.Measure([]()
					{
						BenchmarkAccumulator accumulator;
						for (int i = 0; i < 100; ++i)
						{
							accumulator.Push(static_cast<double>(i) * 1.5);
						}
					});
				};
			};

			// ============================================================
			// Result Retrieval Benchmarks (after 100 samples)
			// ============================================================

			co_yield "get_mean"_bench = []() -> BenchmarkGenerator
			{
				co_yield BASELINE / "naive"_bench = [](RegressionContext& ctx)
				{
					NaiveAccumulator accumulator;
					for (int i = 0; i < 100; ++i)
					{
						accumulator.Push(static_cast<double>(i) * 1.5);
					}

					ctx.Measure([&]()
					{
						auto mean = accumulator.Mean();
						DoNotOptimize(mean);
					});
				};

				co_yield "online"_bench = [](RegressionContext& ctx)
				{
					OnlineAccumulator accumulator;
					for (int i = 0; i < 100; ++i)
					{
						accumulator.Push(static_cast<double>(i) * 1.5);
					}

					ctx.Measure([&]()
					{
						auto mean = accumulator.Mean();
						DoNotOptimize(mean);
					});
				};

				co_yield "benchmark_accumulator"_bench = [](RegressionContext& ctx)
				{
					BenchmarkAccumulator accumulator;
					for (int i = 0; i < 100; ++i)
					{
						accumulator.Push(static_cast<double>(i) * 1.5);
					}

					ctx.Measure([&]()
					{
						auto mean = accumulator.Get<peri::statistics::Mean>().Result();
						DoNotOptimize(mean);
					});
				};
			};

			// ============================================================
			// Variance Retrieval (demonstrates two-pass vs online difference)
			// ============================================================

			co_yield "get_variance"_bench = []() -> BenchmarkGenerator
			{
				co_yield BASELINE / "naive"_bench = [](RegressionContext& ctx)
				{
					NaiveAccumulator accumulator;
					for (int i = 0; i < 100; ++i)
					{
						accumulator.Push(static_cast<double>(i) * 1.5);
					}

					ctx.Measure([&]()
					{
						auto variance = accumulator.Variance();
						DoNotOptimize(variance);
					});
				};

				co_yield "online"_bench = [](RegressionContext& ctx)
				{
					OnlineAccumulator accumulator;
					for (int i = 0; i < 100; ++i)
					{
						accumulator.Push(static_cast<double>(i) * 1.5);
					}

					ctx.Measure([&]()
					{
						auto variance = accumulator.Variance();
						DoNotOptimize(variance);
					});
				};

				co_yield "benchmark_accumulator"_bench = [](RegressionContext& ctx)
				{
					BenchmarkAccumulator accumulator;
					for (int i = 0; i < 100; ++i)
					{
						accumulator.Push(static_cast<double>(i) * 1.5);
					}

					ctx.Measure([&]()
					{
						auto variance = accumulator.Get<peri::statistics::Variance>().Result();
						DoNotOptimize(variance);
					});
				};
			};

			// ============================================================
			// Full Statistics Retrieval
			// ============================================================

			co_yield "get_all_stats"_bench = []() -> BenchmarkGenerator
			{
				co_yield BASELINE / "naive"_bench = [](RegressionContext& ctx)
				{
					NaiveAccumulator accumulator;
					for (int i = 0; i < 100; ++i)
					{
						accumulator.Push(static_cast<double>(i) * 1.5);
					}

					ctx.Measure([&]()
					{
						auto count = accumulator.Count();
						auto mean = accumulator.Mean();
						auto variance = accumulator.Variance();
						auto min = accumulator.Min();
						auto max = accumulator.Max();
						DoNotOptimize(count);
						DoNotOptimize(mean);
						DoNotOptimize(variance);
						DoNotOptimize(min);
						DoNotOptimize(max);
					});
				};

				co_yield "online"_bench = [](RegressionContext& ctx)
				{
					OnlineAccumulator accumulator;
					for (int i = 0; i < 100; ++i)
					{
						accumulator.Push(static_cast<double>(i) * 1.5);
					}

					ctx.Measure([&]()
					{
						auto count = accumulator.Count();
						auto mean = accumulator.Mean();
						auto variance = accumulator.Variance();
						auto min = accumulator.Min();
						auto max = accumulator.Max();
						DoNotOptimize(count);
						DoNotOptimize(mean);
						DoNotOptimize(variance);
						DoNotOptimize(min);
						DoNotOptimize(max);
					});
				};

				co_yield "benchmark_accumulator"_bench = [](RegressionContext& ctx)
				{
					BenchmarkAccumulator accumulator;
					for (int i = 0; i < 100; ++i)
					{
						accumulator.Push(static_cast<double>(i) * 1.5);
					}

					ctx.Measure([&]()
					{
						auto count = accumulator.Get<peri::statistics::Count>().Result();
						auto mean = accumulator.Get<peri::statistics::Mean>().Result();
						auto variance = accumulator.Get<peri::statistics::Variance>().Result();
						auto min = accumulator.Get<peri::statistics::Min>().Result();
						auto max = accumulator.Get<peri::statistics::Max>().Result();
						DoNotOptimize(count);
						DoNotOptimize(mean);
						DoNotOptimize(variance);
						DoNotOptimize(min);
						DoNotOptimize(max);
					});
				};
			};
		});

	SuiteRegistrar _(SUITE);
}
