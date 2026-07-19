#include <stdio.h>
#include <windows.h>

/* Multithread test target for tdb: spawns a few worker threads that all
   call the same function and touch the same global, so a breakpoint or
   watchpoint set once should fire from each of them. */

volatile LONG g_shared = 0;

DWORD WINAPI worker(LPVOID param)
{
    int id = (int)(intptr_t)param;

    for (int i = 0; i < 5; i++)
    {
        InterlockedIncrement(&g_shared);
        printf("thread %d: g_shared=%ld\n", id, g_shared);
        Sleep(100);
    }

    return 0;
}

int main(void)
{
    HANDLE threads[3];

    for (int i = 0; i < 3; i++)
        threads[i] = CreateThread(NULL, 0, worker, (LPVOID)(intptr_t)(i + 1), 0, NULL);

    WaitForMultipleObjects(3, threads, TRUE, INFINITE);

    for (int i = 0; i < 3; i++)
        CloseHandle(threads[i]);

    printf("done, g_shared=%ld\n", g_shared);
    return 0;
}
