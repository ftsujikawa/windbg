#ifndef THREADS_H
#define THREADS_H

#include "debugger.h"

/* Add a thread (tid + its debug-API handle) to *list. Takes the owning
 * process's thread-list head directly (&dbg->threads for the active
 * process, or &entry->threads for a backgrounded process_entry_t) so a
 * thread belonging to a non-active process can be registered without
 * disturbing the active process's state. */
void register_thread(thread_entry_t **list, DWORD tid, HANDLE handle);

/* Remove tid from *list and close its handle. */
void unregister_thread(thread_entry_t **list, DWORD tid);

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
