#ifndef PROCESSES_H
#define PROCESSES_H

#include "debugger.h"

/* Register a brand-new process as a backgrounded entry (does not activate
 * it -- a newly created process doesn't steal focus, same as new threads). */
void register_new_process(debugger_t *dbg, DWORD pid, HANDLE handle, const char *image_path);

/* Switch the active process to pid: saves dbg's current per-process fields
 * into a fresh backgrounded entry, then loads pid's saved fields onto dbg
 * and updates dbg->pid/dbg->process. Returns 1 on success, 0 if pid isn't
 * a tracked process. No-op (returns 1) if pid is already active. */
int switch_active_process(debugger_t *dbg, DWORD pid);

/* Handle a process's exit: prints the exit notice (and that process's leak
 * report if it had tracking on), frees its breakpoints/allocations/threads
 * sublists and closes its handle. If pid was the active process, promotes
 * another tracked process (if any) to active and clears dbg->locked_tid if
 * it belonged to a thread of the exiting process. Returns 1 if any process
 * remains tracked afterward, 0 if none do (caller should end debugger_loop). */
int unregister_process(debugger_t *dbg, DWORD pid, DWORD exit_code);

/* Look up the handle registered for pid (active or backgrounded), or NULL. */
HANDLE find_process_handle(debugger_t *dbg, DWORD pid);

/* Look up the backgrounded (non-active) process_entry_t for pid, or NULL if
 * pid is the active process or isn't tracked. Used by CREATE_THREAD_DEBUG_EVENT/
 * EXIT_THREAD_DEBUG_EVENT to target a non-active process's own thread list
 * directly. */
process_entry_t *find_background_process(debugger_t *dbg, DWORD pid);

/* Print all tracked processes, marking dbg->pid as current. */
void print_processes(debugger_t *dbg);

/* Free the backgrounded process list nodes (and their threads/breakpoints/
 * allocations sublists), no CloseHandle -- used at restart, when the whole
 * debuggee process tree is already gone. */
void free_all_processes(debugger_t *dbg);

#endif
