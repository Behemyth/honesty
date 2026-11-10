export module synodic.honesty.test:suite;

import synodic.honesty.utility;
import synodic.honesty.test.backend;

import std;

namespace honesty::test
{
	export template<std::size_t... NameSizes>
	class SuiteRegistrar
	{
	public:
		explicit SuiteRegistrar(const Suite<NameSizes>&... suites)
		{
			(AddSuite(suites), ...);
		}
	};
}
