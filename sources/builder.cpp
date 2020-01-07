#include "builder.hpp"
namespace
{
    const size_t ERROR_IN_COMMAND_LINE = 1;
    const size_t SUCCESS = 0;
    const size_t ERROR_UNHANDLED_EXCEPTION = 2;
}

int build(int argc, char* argv[]) {
options_description desc("Allowed options");
    desc.add_options()
        ("help", "выводим вспомогательное сообщение")
        ("config", value<std::string>(), "указываем конфигурацию сборки (по умолчанию Debug)")
        ("install", "добавляем этап установки (в директорию _install)")
        ("pack", "добавляем этап упаковки (в архив формата tar.gz)")
        ("timeout", value<time_t>(), "указываем время ожидания (в секундах)")
        ;

    variables_map vm;
    try {
        store(parse_command_line(argc, argv, desc), vm);


        if (vm.count("help") && !vm.count("config") && !vm.count("pack")
            && !vm.count("timeout") && !vm.count("install")) {
            std::cout << desc << "\n";
            return SUCCESS;
        }
        else {
            std::string config = "Debug";
            time_t timeout = time_now();
            time_t time_spent = 0;
            if (vm.count("timeout")) {
                timeout = vm["timeout"].as<time_t>();
            }
            notify(vm);
            if (vm.count("config")) {
                config = vm["config"].as<std::string>();
            }

            std::string command_1 = "cmake -H. -B_build -DCMAKE_INSTALL_" +
                std::string("PREFIX=_install -DCMAKE_BUILD_TYPE=");
            std::string command_2 = "cmake --build _build";
            std::string command_3 = "cmake --build _build --target install";
            std::string command_4 = "cmake --build _build --target package";


            int res_1 = 0;
            int res_2 = 0;

            if (config == "Debug" || config == "Release") {

                command_1 += config;
                auto t1 = async::spawn([&res_1, config, timeout, &time_spent,
                    command_1, command_2]() mutable {
                        time_t start_1 = time_now();

                        create_child(command_1, timeout);
                        time_t end_1 = time_now();

                        time_spent += end_1 - start_1;
                        res_1 = Prob(command_2, res_1, timeout, time_spent);
                    });
            }
            if (vm.count("install") && res_1 == 0) {
                auto t2 = async::spawn([&res_1, &res_2, command_3,
                    timeout, &time_spent]() mutable {
                        res_2 = Prob(command_3, res_1, timeout, time_spent);
                    });
                std::cout << "install" << std::endl;
            }
            if (vm.count("pack") && res_2 == 0) {
                auto t3 = async::spawn([&res_2, command_4, timeout, &time_spent]() mutable {
                    res_2 = Prob(command_4, res_2, timeout, time_spent);
                    });
                std::cout << "package" << std::endl;
            }


        }

    }


catch (boost::program_options::error & e) {
        std::cerr << "ERROR: " << std::endl << std::endl;
        std::cerr << desc << std::endl;
        return ERROR_IN_COMMAND_LINE;
}
    catch (std::exception & e)
    {
        std::cerr << "Unhandled Exception reached the top of main: "<< ", application will now exit" << std::endl;
        return ERROR_UNHANDLED_EXCEPTION;
    }

}

void create_child(const std::string& command, const time_t& period) {
std::string line;
ipstream out;

child process(command, std_out > out);

std::thread checkTime(check_time, std::ref(process), std::ref(period));

while (out && std::getline(out, line) && !line.empty())
    std::cerr << line << std::endl;

checkTime.join();
}


void create_child(const std::string& command, const time_t& period, int& res) {
std::string line;
ipstream out;

child process(command, std_out > out);

std::thread checkTime(check_time, std::ref(process), std::ref(period));

while (out && std::getline(out, line) && !line.empty())
    std::cerr << line << std::endl;

checkTime.join();

res = process.exit_code();
}

void check_time(child& process, const time_t& period){
time_t start = time_now();

while (true) {
    if ((time_now() - start > period) && process.running()) {
        std::error_code ec;
        process.terminate(ec);
        std::cout << ec;
        break;
    }
    else if (!process.running()) {
        break;
    }
}
}

time_t time_now() {
return std::chrono::system_clock::to_time_t(
    std::chrono::system_clock::now()
);
}

int Prob(std::string command1, int& res, time_t& timeout, time_t& time_spent) {

time_t period = timeout - time_spent;
time_t start = time_now();

create_child(command1, period, res);
time_t end = time_now();
time_spent += end - start;

return res;
}

