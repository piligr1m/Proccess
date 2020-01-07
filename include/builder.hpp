#pragma once

#include <async++.h>
#include <vector>
#include <iostream>
#include <signal.h>
#include <string>
#include <stdio.h>
#include <thread>
#include <chrono>
#include <boost/process.hpp>
#include <boost/process/extend.hpp>
#include <boost/program_options.hpp>

using namespace boost::asio;
using namespace boost::process;
using namespace boost::process::extend;
using namespace boost::program_options;

int build(int argc, char* argv[]);
void create_child(const std::string& command, const time_t& period);
void create_child(const std::string& command, const time_t& period, int& res);
void check_time(child& process, const time_t& period);
int Prob(std::string command1, int& res, time_t& timeout, time_t& time_spent);
time_t time_now();
