#ifndef THREADS_H
#define THREADS_H

#include "debugger.h"

/* Add a thread (tid + its debug-API handle) to dbg->threads. */
void register_thread(debugger_t *dbg, DWORD tid, HANDLE handle);

/* Remove tid from dbg->threads and close its handle. */
void unregister_thread(debugger_t *dbg, DWORD tid);

/* Look up the handle registered for tid, or NULL if not found. */
HANDLE find_thread_handle(debugger_t *dbg, DWORD tid);

/* Free the thread list nodes (no CloseHandle -- used at restart, when the
   whole debuggee process is already gone). */
void free_all_threads(debugger_t *dbg);

/* Suspend/resume every registered thread except dbg->tid. Callers must pair
   every suspend with exactly one resume before suspending again. */
void suspend_other_threads(debugger_t *dbg);
void resume_other_threads(debugger_t *dbg);

/* Print all live threads, marking dbg->tid as the current one. */
void print_threads(debugger_t *dbg);

#endif
