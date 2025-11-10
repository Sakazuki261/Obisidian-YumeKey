#include <windows.h>
#include <wchar.h>
#include <shlwapi.h>

int main() {
  wchar_t exePath[MAX_PATH];
  GetModuleFileNameW(NULL, exePath, MAX_PATH);

  if (!getenv("SYNTHV_STUDIO_EXE")) {
    PathRemoveFileSpecW(exePath);
    PathAppendW(exePath, L"synthv-studio.exe");
  } else {
    wcscpy(exePath, _wgetenv(L"SYNTHV_STUDIO_EXE"));
  }

  LPWSTR realCmdLine = GetCommandLineW();
  size_t cmdLineSize = wcslen(realCmdLine) + 1;
  LPWSTR modifiedCmdLine = (LPWSTR)malloc(cmdLineSize * sizeof(WCHAR));
  wcscpy_s(modifiedCmdLine, cmdLineSize, realCmdLine);

  STARTUPINFOW si;
  PROCESS_INFORMATION pi;

  ZeroMemory(&si, sizeof(si));
  si.cb = sizeof(si);
  ZeroMemory(&pi, sizeof(pi));

  if (CreateProcessW(exePath, modifiedCmdLine, NULL, NULL, FALSE, CREATE_SUSPENDED, NULL, NULL, &si, &pi)) {
    LPCWSTR dllPath = getenv("OBSIDIAN_DLL") ? _wgetenv(L"OBSIDIAN_DLL") : L"obsidian.dll";
    LPVOID remoteStringAddress = VirtualAllocEx(pi.hProcess, NULL, wcslen(dllPath) * sizeof(WCHAR) + 2,
      MEM_COMMIT, PAGE_READWRITE);

    WriteProcessMemory(pi.hProcess, remoteStringAddress, dllPath,
      wcslen(dllPath) * sizeof(WCHAR) + 1, NULL);

    HANDLE hThread = CreateRemoteThread(pi.hProcess, NULL, 0,
      (LPTHREAD_START_ROUTINE)LoadLibraryW, remoteStringAddress, 0, NULL);

    WaitForSingleObject(hThread, INFINITE);

    ResumeThread(pi.hThread);
    WaitForSingleObject(pi.hProcess, INFINITE);

    DWORD exitCode;
    GetExitCodeProcess(pi.hProcess, (LPDWORD)&exitCode);

    CloseHandle(hThread);
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
    VirtualFreeEx(pi.hProcess, remoteStringAddress, 0, MEM_RELEASE);

    free(modifiedCmdLine);

    return exitCode;
  }

  return 1;
}
