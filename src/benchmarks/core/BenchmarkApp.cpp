// Copyright (c) 2016, Dmitry Koplyarov <koplyarov.da@gmail.com>
//
// Permission to use, copy, modify, and/or distribute this software for any purpose with or without fee is hereby granted,
// provided that the above copyright notice and this permission notice appear in all copies.
//
// THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS.
// IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
// WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.


#include <benchmarks/core/BenchmarkApp.hpp>
#include <benchmarks/core/ipc/MessageQueue.hpp>
#include <benchmarks/core/utils/ThreadPriority.hpp>

#include <boost/program_options.hpp>
#include <boost/regex.hpp>
#include <boost/scope_exit.hpp>


namespace benchmarks
{

	using OperationTimesMap = std::map<std::string, double>	;
	using MemoryConsumptionMap = std::map<std::string, int64_t>;


	class BenchmarksResultsReporter : public IBenchmarksResultsReporter
	{
	private:
		static NamedLogger		s_logger;
		OperationTimesMap		_operationTimes;
		MemoryConsumptionMap	_memoryConsumption;

	public:
		virtual void ReportOperationDuration(const std::string& name, double ns)
		{
			s_logger.Debug() << name << ": " << ns << " ns";
			_operationTimes[name] = ns;
		}

		virtual void ReportMemoryConsumption(const std::string& name, int64_t bytes)
		{
			s_logger.Debug() << name << ": " << bytes << " bytes";
			_memoryConsumption[name] = bytes;
		}

		const OperationTimesMap& GetOperationTimes() const { return _operationTimes; }
		const MemoryConsumptionMap& GetMemoryConsumption() const { return _memoryConsumption; }
	};
	BENCHMARKS_LOGGER(BenchmarksResultsReporter);


	BENCHMARKS_LOGGER(BenchmarkApp);

	BenchmarkApp::BenchmarkApp(const BenchmarkSuite& suite)
		: _suite(suite)
	{ }


	namespace
	{
		struct CmdLineException : public std::runtime_error
		{ CmdLineException(const std::string& msg) : std::runtime_error(msg) { } };
	}

	int BenchmarkApp::Run(int argc, char* argv[])
	{
		try
		{
			using namespace boost::program_options;

			std::string subtask, queue_name = "wigwagMessageQueue", benchmark;
			int64_t num_iterations = 0, count = 1, verbosity = 1;
			std::vector<std::string> params_vec;

			options_description od;
			od.add_options()
				("help", "Show help")
				("verbosity,v", value<int64_t>(&verbosity), "Verbosity in range [0..3], default: 1")
				("count", value<int64_t>(&count), "Measurements count, default: 1")
				("benchmark", value<std::string>(&benchmark), "Benchmark id")
				("params", value<std::vector<std::string>>(&params_vec)->multitoken(), "Benchmark parameters")
				("subtask", value<std::string>(&subtask), "Internal option")
				("queue", value<std::string>(&queue_name), "Internal option")
				("iterations", value<int64_t>(&num_iterations), "Internal option")
				;

			positional_options_description pd;
			pd.add("benchmark", 1).add("params", -1);

			parsed_options parsed = command_line_parser(argc, argv).options(od).positional(pd).run();

			variables_map vm;
			store(parsed, vm);
			notify(vm);

			if (benchmark.empty() || vm.count("help"))
			{
				std::cerr << boost::lexical_cast<std::string>(od) << std::endl;
				return 0;
			}

			switch (verbosity)
			{
			case 0: Logger::SetLogLevel(LogLevel::Error); break;
			case 1: Logger::SetLogLevel(LogLevel::Warning); break;
			case 2: Logger::SetLogLevel(LogLevel::Info); break;
			case 3: Logger::SetLogLevel(LogLevel::Debug); break;
			default: s_logger.Warning() << "Unexpected verbosity value: " << verbosity; break;
			}

			boost::regex benchmark_re(R"(([^.]+)\.([^.]+)\.([^.]+))");
			boost::smatch m;
			if (!boost::regex_match(benchmark, m, benchmark_re))
				throw CmdLineException("Could not parse benchmark id!");

			boost::regex param_re(R"(([^.]+):([^.]+))");
			std::map<std::string, SerializedParam> params;
			for (auto&& param_str : params_vec)
			{
				boost::smatch m;
				if (!boost::regex_match(param_str, m, param_re))
					throw CmdLineException("Could not parse parameter!");
				params[m[1]] = m[2];
			}

			if (subtask == "measureIterationsCount")
			{
				MessageQueue mq(queue_name);
				mq.SendMessage(std::make_shared<IterationsCountMessage>(_suite.MeasureIterationsCount({m[1], m[2], m[3]}, params)));
				return 0;
			}
			else if (subtask == "invokeBenchmark")
			{
				if (vm.count("iterations") == 0)
					throw CmdLineException("Number of iterations is not specified!");
				MessageQueue mq(queue_name);
				SetMaxThreadPriority();
				auto results_reporter = std::make_shared<BenchmarksResultsReporter>();
				_suite.InvokeBenchmark(num_iterations, {m[1], m[2], m[3]}, params, results_reporter);
				mq.SendMessage(std::make_shared<ResultsMessage>(results_reporter->GetOperationTimes(), results_reporter->GetMemoryConsumption()));
				return 0;
			}

			if (!subtask.empty())
				throw CmdLineException("Unknown subtask!");

			MessageQueue mq(queue_name);
			BOOST_SCOPE_EXIT_ALL(&) { MessageQueue::Remove(queue_name); };

			{
				std::stringstream cmd;
				cmd << argv[0] << " --subtask measureIterationsCount --queue " << queue_name << " --verbosity " << verbosity << " " << benchmark;
				for (auto&& p : params_vec)
					cmd << " " << p;

				InvokeSubprocess(cmd.str());
			}

			auto it_msg = mq.ReceiveMessage<IterationsCountMessage>();
			OperationTimesMap operation_times;
			MemoryConsumptionMap memory_consumption;
			for (int64_t i = 0; i < count; ++i)
			{
				std::stringstream cmd;
				cmd << argv[0] << " --subtask invokeBenchmark --queue " << queue_name << " --verbosity " << verbosity << " --iterations " << it_msg->GetCount() << " " << benchmark;
				for (auto&& p : params_vec)
					cmd << " " << p;

				InvokeSubprocess(cmd.str());
				auto r_msg = mq.ReceiveMessage<ResultsMessage>();

				for (auto p : r_msg->GetOperationTimes())
				{
					auto it = operation_times.find(p.first);
					if (it == operation_times.end())
						operation_times.insert({p.first, p.second});
					else
						it->second = std::min(it->second, p.second);
				}

				for (auto p : r_msg->GetMemoryConsumption())
				{
					auto it = memory_consumption.find(p.first);
					if (it == memory_consumption.end())
						memory_consumption.insert({p.first, p.second});
					else
						it->second = std::min(it->second, p.second);
				}
			}

			for (auto p : operation_times)
				s_logger.Info() << p.first << ": " << p.second << " ns";
			for (auto p : memory_consumption)
				s_logger.Info() << p.first << ": " << p.second << " bytes";

			return 0;
		}
		catch (const CmdLineException& ex)
		{
			std::cerr << ex.what() << std::endl;
			return 1;
		}
		catch (const std::exception& ex)
		{
			s_logger.Error() << "Uncaught exception: " << ex.what();
			return 1;
		}
	}


	void BenchmarkApp::InvokeSubprocess(const std::string& cmd)
	{
		s_logger.Debug() << "Invoking " << cmd;
		if (system(cmd.c_str()) != 0)
			throw std::runtime_error(cmd + " failed!");
	}

}