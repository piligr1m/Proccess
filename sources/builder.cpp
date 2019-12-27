#include "pch.h"
#include <iostream>

#include <boost/program_options.hpp>

#include "async++.h"

#include <boost/process.hpp>
#include <boost/filesystem.hpp>

#include <chrono>
#include <thread>
#include <atomic>

namespace bp = boost::process;
namespace po = boost::program_options;

int proc_func(const std::string &cmake_path, const std::vector<std::string> &args, const std::atomic_bool &timeout_flag);

int main(int argc, char *argv[])
{
	std::system("chcp 1251");

	po::options_description desc("Allowed options");
	desc.add_options()
		("help,h", "Выводим вспомогательное сообщение")
		("config,c", po::value<std::string>()->default_value("Debug"), "Указываем конфигурацию сборки (по умолчанию Debug)")
		("install,i", "Добавляем этап установки (в директорию _install)")
		("pack,p", "Добавляем этап упаковки (в архив формата tar.gz)")
		("timeout,t", po::value<int>()->default_value(0), "Указываем время ожидания (в секундах)")
		;
	po::variables_map vm;
	po::store(po::parse_command_line(argc, argv, desc), vm);

	try
	{
		po::notify(vm);
	}
	catch (std::exception& ex)
	{
		std::cerr << "Error: " << ex.what() << "\n";
		return EXIT_FAILURE;
	}

	std::string config("-DCMAKE_BUILD_TYPE=Debug");
	int timeout = 0;
	if (vm.count("help"))
	{
		std::cout << desc << "\n";
		return EXIT_SUCCESS;
	}

	std::string cmake_path;
	int result_code = 333;

	bool is_install = false;
	std::atomic_bool timeout_flag(false);

	async::shared_task<void> timeout_task;
	if (vm.count("timeout"))
	{
		int timeout = vm["timeout"].as<int>();
		timeout_task = async::spawn([timeout, &timeout_flag]() mutable
		{
			if (!timeout)
			{
				do
				{
					std::this_thread::yield();
				} while (1);
			}
			else
			{
				std::cout << "Set timeout " << timeout << std::endl;
				auto start = std::chrono::high_resolution_clock::now();
				auto end = start + std::chrono::seconds(timeout);
				do
				{
					std::this_thread::yield();
				} while (std::chrono::high_resolution_clock::now() < end);
				timeout_flag.store(true);
				std::cout << "++++++++++++++ WE HAVE A TIMEOUT !!! ++++++++++++++++++++++" << timeout << std::endl;
			}
		}).share();
	}

	if (vm.count("config"))
	{
		config = "-DCMAKE_BUILD_TYPE=" + vm["config"].as<std::string>();
	}

	boost::filesystem::path path = bp::search_path("cmake");
	cmake_path = path.string();

	auto builder_task = async::spawn([cmake_path, &result_code, &timeout_flag]() mutable
	{
		std::cout << "------------------------- Stage: version CMake ----------------------------------------------" << std::endl;
		std::cout << "cmake_path = " << cmake_path.c_str() << std::endl;
		std::vector<std::string> args;
		args.push_back("--version");
		result_code = proc_func(cmake_path, args, timeout_flag);
		std::cout << "result_code[out, version] = " << result_code << std::endl;
		std::cout << "------------------------------------------------------------------------------------------" << std::endl << std::endl;
	}).share();

	auto b_task = builder_task.then([]()
	{
		std::cout << "++++++++++++++ BUILD SUCCESS !!! ++++++++++++++++++++++" << std::endl;
	}).share();

	async::shared_task<void> tasks[] =
	{
		b_task,
		timeout_task
	};
	async::when_any(tasks).then([](async::when_any_result<std::vector<async::shared_task<void>>> result)
	{
		std::cout << "Task " << result.index << " finished" << std::endl;
		exit(EXIT_SUCCESS);
	});

	if (vm.count("pack"))
	{
		if (!is_install)
		{
			builder_task.then([cmake_path, &result_code, &timeout_flag]() mutable
			{
				if (result_code)
					return;

				std::cout << "------------------------- Stage: install -------------------------------------------------" << std::endl;
				std::vector<std::string> args;
				args.push_back("--target ");
				args.push_back("install");
				result_code = proc_func(cmake_path, args, timeout_flag);
				std::cout << "result_code[out, install] = " << result_code << std::endl;
				std::cout << "------------------------------------------------------------------------------------------" << std::endl << std::endl;
			});
		}

		builder_task.then([cmake_path, &result_code, &timeout_flag]() mutable
		{
			if (result_code)
				return;

			std::cout << "------------------------- Stage: pack ----------------------------------------------------" << std::endl;
			std::cout << "Cmake pack ..." << std::endl;
			std::vector<std::string> args;
			args.push_back("--target ");
			args.push_back("package");
			result_code = proc_func(cmake_path, args, timeout_flag);
			std::cout << "result_code[out, pack] = " << result_code << std::endl;
			std::cout << "------------------------------------------------------------------------------------------" << std::endl << std::endl;
		});
	}

	if (vm.count("install"))
	{
		builder_task.then([cmake_path, &result_code, &timeout_flag]() mutable
		{
			if (result_code)
				return;

			std::cout << "------------------------- Stage: install -------------------------------------------------" << std::endl;
			std::cout << "Cmake install ..." << std::endl;
			std::vector<std::string> args;
			args.push_back("--target ");
			args.push_back("install");
			result_code = proc_func(cmake_path, args, timeout_flag);
			std::cout << "result_code[out, install] = " << result_code << std::endl;
			std::cout << "------------------------------------------------------------------------------------------" << std::endl << std::endl;
		});
		if (!result_code)
			is_install = true;
	}

	builder_task.then([cmake_path, &result_code, &timeout_flag]() mutable
	{
		if (result_code)
			return;

		std::cout << "------------------------- Stage: build ----------------------------------------------------" << std::endl;
		std::vector<std::string> args;
		args.push_back("--build");
		args.push_back("_builds");
		result_code = proc_func(cmake_path, args, timeout_flag);
		std::cout << "result_code[out, build] = " << result_code << std::endl;
		std::cout << "------------------------------------------------------------------------------------------" << std::endl << std::endl;
	});

	builder_task.then([cmake_path, config, &result_code, &timeout_flag]() mutable
	{
		if (result_code)
			return;

		std::cout << "------------------------- Stage: prepare --------------------------------------------------" << std::endl;
		std::cout << "config = " << config << std::endl;
		std::vector<std::string> args;
		args.push_back("-H.");
		args.push_back("-B_builds");
		args.push_back("-DCMAKE_INSTALL_PREFIX=_install");
		args.push_back(config);
		result_code = proc_func(cmake_path, args, timeout_flag);
		std::cout << "result_code[out, prepare] = " << result_code << std::endl;
		std::cout << "------------------------------------------------------------------------------------------" << std::endl << std::endl;
	});

	return EXIT_SUCCESS;
}

int proc_func(const std::string &cmake_path, const std::vector<std::string> &args, const std::atomic_bool &timeout_flag)
{
	bp::child c(cmake_path, bp::args(args));
	while (c.running())
	{
		if (timeout_flag.load())
		{
			c.terminate();
			std::cout << "+++++++++++ TIMEOUT SIGNAL ++++++++++++ " << std::endl;
			return 1;
		}
	}
	c.wait();
	return c.exit_code();
}
