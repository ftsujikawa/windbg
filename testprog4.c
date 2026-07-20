#include <stdio.h>
#include <string.h>
#include <windows.h>

/* Multi-process test target for tdb: run with no arguments, it spawns a
   child copy of itself (passing "child" on the command line); the child
   just calls a small breakpointable function a few times and exits. */

int worker(int n)
{
    int result = n * 2;
    printf("worker(%d) = %d\n", n, result);
    return result;
}

int main(int argc, char *argv[])
{
    if (argc > 1 && strcmp(argv[1], "child") == 0)
    {
        printf("child pid=%lu starting\n", GetCurrentProcessId());

        for (int i = 0; i < 3; i++)
        {
            worker(i);
            Sleep(100);
        }

        printf("child pid=%lu done\n", GetCurrentProcessId());
        return 0;
    }

    char self_path[MAX_PATH];
    GetModuleFileNameA(NULL, self_path, sizeof(self_path));

    char cmdline[MAX_PATH + 16];
    snprintf(cmdline, sizeof(cmdline), "\"%s\" child", self_path);

    STARTUPINFOA si = {0};
    si.cb = sizeof(si);
    PROCESS_INFORMATION pi = {0};

    printf("parent pid=%lu launching child\n", GetCurrentProcessId());

    if (!CreateProcessA(self_path, cmdline, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi))
    {
        printf("CreateProcess failed: %lu\n", GetLastError());
        return 1;
    }

    WaitForSingleObject(pi.hProcess, INFINITE);

    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);

    printf("parent pid=%lu: child exited\n", GetCurrentProcessId());
    return 0;
}
