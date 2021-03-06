#include "stdafx.h"
#include <stdio.h>
#include <iostream>
#include <fstream>
#include <sstream>

using namespace std;

string puush;
string hook;
bool readSettings() {
	ifstream f;
	string line;
	f.open("settings.ini", ios::in);
	if(f.is_open()) {
		for(int i = 0; f.good(); i++ ){			
			getline(f, line);
			switch(i) {
			case 0: 
				puush = line;
				break; 
			case 1:
				hook = line;
				break;
			}
		}
		f.close();
		return true;
	} else {
		cout << "Could not find settings.ini" << endl;
		return false;
	}
	return true;
}
int main()
{
	if (readSettings() == false)
	{
		//return 0;
	}
	STARTUPINFOA lpStartupInfo = {sizeof(STARTUPINFOA)};
	PROCESS_INFORMATION lpProcessInfo={0};
	memset(&lpProcessInfo, 0, sizeof(lpProcessInfo));
	memset(&lpStartupInfo, 0, sizeof(lpStartupInfo));
	const char* name = puush.c_str();
	cout << CreateProcessA(name, NULL, NULL, NULL, FALSE, NORMAL_PRIORITY_CLASS, NULL, NULL, &lpStartupInfo, &lpProcessInfo) << endl;
	cout << GetLastError() << endl;
	HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE,lpProcessInfo.dwProcessId);
	if (hProcess == INVALID_HANDLE_VALUE)
	{
		fprintf(stderr, "cannot open that pid\n");
		return 1;
	}
	const char* path = hook.c_str();
	PVOID mem = VirtualAllocEx(hProcess, NULL, strlen(path) + 1, MEM_COMMIT, PAGE_READWRITE);
	if (mem == NULL)
	{
		fprintf(stderr, "can't allocate memory in that pid\n");
		CloseHandle(hProcess);
		return 1;
	}	
	if (WriteProcessMemory(hProcess, mem, (void*)path, strlen(path) + 1, NULL) == 0)
	{
		fprintf(stderr, "can't write to memory in that pid\n");
		VirtualFreeEx(hProcess, mem, strlen(path) + 1, MEM_RELEASE);
		CloseHandle(hProcess);
		return 1;
	}
	HANDLE hThread = CreateRemoteThread(hProcess, NULL, 0, (LPTHREAD_START_ROUTINE) GetProcAddress(GetModuleHandleA("KERNEL32.DLL"),"LoadLibraryA"), mem, 0, NULL);
	if (hThread == INVALID_HANDLE_VALUE)
	{
		fprintf(stderr, "can't create a thread in that pid\n");
		VirtualFreeEx(hProcess, mem, strlen(path) + 1, MEM_RELEASE);
		CloseHandle(hProcess);
		return 1;
	}
	WaitForSingleObject(hThread, INFINITE);
	HMODULE hLibrary = NULL;
	if (!GetExitCodeThread(hThread, (LPDWORD)&hLibrary))
	{
		printf("can't get exit code for thread GetLastError() = %i.\n", GetLastError());
		CloseHandle(hThread);
		VirtualFreeEx(hProcess, mem, strlen(path) + 1, MEM_RELEASE);
		CloseHandle(hProcess);
		return 1;
	}
	CloseHandle(hThread);
	VirtualFreeEx(hProcess, mem, strlen(path) + 1, MEM_RELEASE);	
	if (hLibrary == NULL)
	{
		hThread = CreateRemoteThread(hProcess, NULL, 0, (LPTHREAD_START_ROUTINE) GetProcAddress(GetModuleHandleA("KERNEL32.DLL"),"GetLastError"), 0, 0, NULL);
		if (hThread == INVALID_HANDLE_VALUE)
		{
			fprintf(stderr, "LoadLibraryA returned NULL and can't get last error.\n");
			CloseHandle(hProcess);
			return 1;
		}
		WaitForSingleObject(hThread, INFINITE);
		DWORD error;
		GetExitCodeThread(hThread, &error);
		CloseHandle(hThread);
		printf("LoadLibrary return NULL, GetLastError() is %i\n", error);
		CloseHandle(hProcess);
		return 1;
	}	
	return 0;
}