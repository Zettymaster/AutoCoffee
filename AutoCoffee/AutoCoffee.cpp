#define WIN32_LEAN_AND_MEAN
#define NOGDI
#define NOMINMAX
#include <windows.h>
#include <tlhelp32.h>

#include <vector>
#include <string>
#include <print>
#include <chrono>
#include <thread>

template<class T, class U>
T string_cast(U s);

template<>
std::string string_cast(std::wstring s)
{
	const std::size_t size_needed = WideCharToMultiByte(CP_UTF8, 0, s.data(), static_cast<int>(s.size()), nullptr, 0, nullptr, nullptr);
	std::string u8_str(size_needed, '\0');
	WideCharToMultiByte(CP_UTF8, 
		0, 
		s.data(), 
		static_cast<int>(s.size()),
		u8_str.data(), 
		static_cast<int>(u8_str.size()), 
		nullptr,
		nullptr);

	return u8_str;
}

namespace zetty::autocoffee
{
	constexpr EXECUTION_STATE STATE_BUSY = ES_CONTINUOUS | ES_SYSTEM_REQUIRED | ES_DISPLAY_REQUIRED;
	constexpr EXECUTION_STATE STATE_NORMAL = ES_CONTINUOUS;

	bool set_thread_state(EXECUTION_STATE e)
	{
		const auto prevState = SetThreadExecutionState(e);
		return prevState != 0;
	}

	bool activate_busy_state()
	{
		return set_thread_state(STATE_BUSY);
	}

	bool activate_normal_state()
	{
		return set_thread_state(STATE_NORMAL);
	}

	std::vector<std::string> get_running_processes()
	{
		const HANDLE process_snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
		if (process_snapshot == INVALID_HANDLE_VALUE)
			return {};

		std::vector<std::string> processes;

		PROCESSENTRY32 current_process;
		current_process.dwSize = sizeof(PROCESSENTRY32);
		if (!Process32First(process_snapshot, &current_process))
		{
			CloseHandle(process_snapshot);
			return {};
		}

		do {
			auto exe = string_cast<std::string, std::wstring>(current_process.szExeFile);
			processes.emplace_back(exe);
			
		} while (Process32Next(process_snapshot, &current_process));

		CloseHandle(process_snapshot);

		return processes;
	}

	bool is_vs_running()
	{
		const auto processes = get_running_processes();

		const std::string vs{ "devenv.exe" };
		return std::ranges::find(processes, vs) != processes.cend();
	}

	[[noreturn]] void run(auto timeout)
	{
		while (true)
		{
			if (is_vs_running())
			{
				std::print("blocking sleep\n");
				activate_busy_state();
			}
			else
			{
				std::print("not blocking sleep\n");
				activate_normal_state();
			}
			std::this_thread::sleep_for(timeout);
		}
	}

}


int main()
{
	using namespace std::chrono_literals;
	zetty::autocoffee::run(30min);
}
