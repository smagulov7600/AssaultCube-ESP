#pragma once
#include <Windows.h> // RPM + WPM
#include <TlHelp32.h> // getting processIds.
#include <Psapi.h> // enumprocessmodules
#include <string> // generally nicer to work with than chars, but that's your preference.
#include <string_view>
#include <vector> // std::vector

#include <iostream>

/*
Created By https://github.com/carlgwastaken/
Please Support Open Source and leave this message here if you're using in your own source
Besides that, enjoy!
*/

// the ReadProcessMemory is basically just a pointer to this function + some error handling, so we save some performance.
// 5% according to the post below:
// unknowncheats.me/forum/general-programming-and-reversing/230813-readprocessmemory-vs-ntreadvirtualmemory-performance-benchmark-comparison.html
typedef NTSTATUS(WINAPI* pNtReadVirtualMemory)(
	HANDLE ProcessHandle,
	PVOID BaseAddress,
	PVOID Buffer,
	ULONG NumberOfBytesToRead,
	PULONG NumberOfBytesRead
	);
typedef NTSTATUS(WINAPI* pNtWriteVirtualMemory)(
	HANDLE Processhandle,
	PVOID BaseAddress,
	PVOID Buffer,
	ULONG NumberOfBytesToWrite,
	PULONG NumberOfBytesWritten
	);

class memify
{
private:
	// initalize at 0 so we can check later
	HANDLE handle = 0;
	DWORD processID = 0;
	std::string process_name;

	// define Virtual Read + Virtual Write
	pNtReadVirtualMemory VRead;
	pNtWriteVirtualMemory VWrite;

	uintptr_t GetProcessId(std::string_view processName)
	{
		// define processentry32
		PROCESSENTRY32 pe;
		pe.dwSize = sizeof(PROCESSENTRY32);

		// create a snapshot handle
		HANDLE ss = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);

		// loop through all process
		while (Process32Next(ss, &pe)) {
			// compare program names to processName
			if (!processName.compare(pe.szExeFile)) {
				processID = pe.th32ProcessID;
				return processID;
			}
		}
	}

	// uses already open handle to enumerate, you could also use PROCESSENTRY32 here.
	DWORD GetBaseModule(std::string_view moduleName) {
		// Take a snapshot of all modules in the process.
		HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, processID);

		if (hSnapshot == INVALID_HANDLE_VALUE) {
			std::cerr << "Failed to take snapshot of modules.\n";
			return 0;
		}

		MODULEENTRY32 moduleEntry;
		moduleEntry.dwSize = sizeof(MODULEENTRY32);

		// Start iterating through modules.
		if (Module32First(hSnapshot, &moduleEntry)) {
			do {
				// Compare the module name.
				if (moduleName.compare(moduleEntry.szModule) == 0) {
					uintptr_t baseAddress = reinterpret_cast<uintptr_t>(moduleEntry.modBaseAddr);
					std::cout << "Found module: " << moduleEntry.szModule << " at address: " << std::hex << baseAddress << std::endl;

					CloseHandle(hSnapshot); // Clean up the snapshot handle.
					return baseAddress;
				}
			} while (Module32Next(hSnapshot, &moduleEntry));
		}
		else {
			std::cerr << "Module32First failed. No modules found.\n";
		}

		CloseHandle(hSnapshot); // Clean up the snapshot handle.
		return 0; // Module not found.
	}
public:
	// if you have an array of possible process names you want to loop through, and if one matches it will grab the handle for that.
	// this could be used if for example working with a mod that uses the base game for its gameplay, or a game that you usually play on multiple versions on.
	// keep in mind, this can get annoying with offsets VERY fast.
	memify(std::vector<std::string> processes) {
		VRead = (pNtReadVirtualMemory)GetProcAddress(GetModuleHandleA("ntdll.dll"), "NtReadVirtualMemory");
		VWrite = (pNtWriteVirtualMemory)GetProcAddress(GetModuleHandleA("ntdll.dll"), "NtWriteVirtualMemory");

		for (auto& name : processes) {
			processID = GetProcessId(name);

			if (processID != 0) {
				handle = OpenProcess(PROCESS_ALL_ACCESS, 0, processID);
				if (handle) {
					process_name = name;
					std::cout << "[>>] Found Process: " << process_name << std::endl;
					break;
				}
				else {
					// somehow we got a valid processID but not a valid handle?
					continue;
				}
			}
			continue;
		}
	}

	// constructor opens handle and you save one line!!!! (will make your spaghetti code 10x better)
	memify(std::string_view processName)
	{
		VRead = (pNtReadVirtualMemory)GetProcAddress(GetModuleHandleA("ntdll.dll"), "NtReadVirtualMemory");
		VWrite = (pNtWriteVirtualMemory)GetProcAddress(GetModuleHandleA("ntdll.dll"), "NtWriteVirtualMemory");

		processID = GetProcessId(processName);

		if (processID != 0)
		{
			handle = OpenProcess(PROCESS_ALL_ACCESS, 0, processID);
			if (handle) {
				process_name = processName;
				std::cout << "[>>] Found Process: " << processName << std::endl;
			}
		}
	}

	// deconstructor, called automatically when closing.
	~memify()
	{
		if (handle)
			CloseHandle(handle);
	}

	std::string GetProcessName() { return process_name; }

	// shorten name here
	uintptr_t GetBase(std::string_view moduleName)
	{
		return GetBaseModule(moduleName);
	}

	// read
	template <typename T> // use types which are defined later on, so it's compatible with alot of shit.
	T Read(uintptr_t address)
	{
		T buffer{ };
		VRead(handle, (void*)address, &buffer, sizeof(T), 0);
		return buffer;
	}

	template <typename T>
	T Write(uintptr_t address, T value)
	{
		VWrite(handle, (void*)address, &value, sizeof(T), NULL);
		return value;
	}

	// for reading structs and strings and shit
	bool ReadRaw(uintptr_t address, void* buffer, size_t size)
	{
		SIZE_T bytesRead;
		if (VRead(handle, (void*)address, buffer, static_cast<ULONG>(size), (PULONG)&bytesRead))
			return bytesRead = size;

		return false;
	}

	// utilities, shit that isn't required but nice to have

	bool ProcessIsOpen(const std::string_view processName)
	{
		return GetProcessId(processName) != 0;
	}

	bool InForeground(const std::string& windowName)
	{
		// just takes Counter-Strike 2 but change to whatever u want, or implement an input you can do that too
		// maybe get the foreground window and then compare it to your own window, use processID, anything u want
		HWND current = GetForegroundWindow();

		char title[256];
		GetWindowText(current, title, sizeof(title));

		if (strstr(title, windowName.c_str()) != nullptr)
			return true;

		return false;
	}
};