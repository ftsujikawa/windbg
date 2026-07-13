#include <stdio.h>
#include "watchpoints.h"

/* -----------------------------------------------------------------------
 * DR7 bit layout (condensed):
 *   bits  1,3,5,7  : Local Enable for DR0,DR1,DR2,DR3
 *   bits 18-19     : condition (R/W) for DR0   (repeated per slot)
 *   bits 20-21     : size for DR0
 *   bits 22-23     : condition for DR1
 *   bits 24-25     : size for DR1
 *   ... and so on.
 *
 * Condition encoding:
 *   00 = break on instruction execution  (not used here)
 *   01 = break on data write
 *   11 = break on data read or write
 *
 * Size encoding:
 *   00 = 1-byte
 *   01 = 2-byte
 *   10 = 8-byte (x86-64 extension)
 *   11 = 4-byte
 * ----------------------------------------------------------------------- */

static DWORD64 encode_size(int size)
{
    switch (size)
    {
        case 1: return 0x0;
        case 2: return 0x1;
        case 8: return 0x2;
        case 4: return 0x3;
        default: return 0x3;
    }
}

/* Write DR0-DR3 and DR7 into the thread context to arm/disarm watchpoints. */
static void apply_watchpoints(debugger_t *dbg)
{
    CONTEXT ctx = {0};
    ctx.ContextFlags = CONTEXT_DEBUG_REGISTERS;
    GetThreadContext(dbg->thread, &ctx);

    /* Clear all local enable bits and condition/size fields */
    ctx.Dr0 = 0;
    ctx.Dr1 = 0;
    ctx.Dr2 = 0;
    ctx.Dr3 = 0;
    ctx.Dr7 = 0;

    for (int i = 0; i < dbg->watch_count; i++)
    {
        watchpoint_t *wp = &dbg->watchpoints[i];
        int slot = wp->slot;

        /* Set DRn address */
        switch (slot)
        {
            case 0: ctx.Dr0 = (DWORD64)wp->addr; break;
            case 1: ctx.Dr1 = (DWORD64)wp->addr; break;
            case 2: ctx.Dr2 = (DWORD64)wp->addr; break;
            case 3: ctx.Dr3 = (DWORD64)wp->addr; break;
        }

        /* Local enable bit: bit (slot*2 + 1) — actually bits 0,2,4,6 are local enable */
        ctx.Dr7 |= (1ULL << (slot * 2));  /* local enable for this slot */

        DWORD64 cond = (wp->type == WATCH_RW) ? 3ULL : 1ULL;
        DWORD64 sz   = encode_size(wp->size);

        /* Condition: bits 16+slot*4, 17+slot*4 */
        ctx.Dr7 |= (cond << (16 + slot * 4));
        /* Size:      bits 18+slot*4, 19+slot*4 */
        ctx.Dr7 |= (sz   << (18 + slot * 4));
    }

    SetThreadContext(dbg->thread, &ctx);
}

/* Find a free slot (0-3). Returns -1 if all used. */
static int find_free_slot(debugger_t *dbg)
{
    int used[4] = {0, 0, 0, 0};
    for (int i = 0; i < dbg->watch_count; i++)
        used[dbg->watchpoints[i].slot] = 1;
    for (int s = 0; s < 4; s++)
        if (!used[s]) return s;
    return -1;
}

int set_watchpoint(debugger_t *dbg, void *addr, int type, int size)
{
    if (dbg->watch_count >= 4)
    {
        printf("watchpoint: all 4 hardware slots are in use\n");
        return -1;
    }

    /* Reject duplicate address */
    for (int i = 0; i < dbg->watch_count; i++)
    {
        if (dbg->watchpoints[i].addr == addr)
        {
            printf("watchpoint already set at %p\n", addr);
            return -1;
        }
    }

    /* Validate size */
    if (size != 1 && size != 2 && size != 4 && size != 8)
    {
        printf("watchpoint: size must be 1, 2, 4, or 8\n");
        return -1;
    }

    int slot = find_free_slot(dbg);

    watchpoint_t *wp = &dbg->watchpoints[dbg->watch_count++];
    wp->addr = addr;
    wp->type = type;
    wp->size = size;
    wp->slot = slot;

    apply_watchpoints(dbg);

    const char *type_str = (type == WATCH_RW) ? "read/write" : "write";
    printf("watchpoint %d set at %p (%s, %d byte%s)\n",
           slot, addr, type_str, size, size > 1 ? "s" : "");
    return 0;
}

int remove_watchpoint(debugger_t *dbg, void *addr)
{
    for (int i = 0; i < dbg->watch_count; i++)
    {
        if (dbg->watchpoints[i].addr == addr)
        {
            printf("watchpoint %d removed at %p\n",
                   dbg->watchpoints[i].slot, addr);
            /* Compact the array */
            for (int j = i; j < dbg->watch_count - 1; j++)
                dbg->watchpoints[j] = dbg->watchpoints[j + 1];
            dbg->watch_count--;
            apply_watchpoints(dbg);
            return 0;
        }
    }
    printf("no watchpoint at %p\n", addr);
    return -1;
}

void remove_all_watchpoints(debugger_t *dbg)
{
    dbg->watch_count = 0;
    apply_watchpoints(dbg);
}

/* DR6 bits 0-3 indicate which watchpoint triggered. */
int watchpoint_hit_slot(debugger_t *dbg)
{
    CONTEXT ctx = {0};
    ctx.ContextFlags = CONTEXT_DEBUG_REGISTERS;
    GetThreadContext(dbg->thread, &ctx);

    for (int s = 0; s < 4; s++)
    {
        if (ctx.Dr6 & (1ULL << s))
        {
            /* Clear the status bit */
            ctx.Dr6 &= ~(1ULL << s);
            SetThreadContext(dbg->thread, &ctx);
            return s;
        }
    }
    return -1;
}

void watchpoint_report(debugger_t *dbg, int slot, DWORD64 rip)
{
    for (int i = 0; i < dbg->watch_count; i++)
    {
        if (dbg->watchpoints[i].slot == slot)
        {
            const char *type_str =
                (dbg->watchpoints[i].type == WATCH_RW) ? "read/write" : "write";
            printf("watchpoint %d hit: %s at %p (RIP=0x%llx)\n",
                   slot,
                   type_str,
                   dbg->watchpoints[i].addr,
                   (unsigned long long)rip);
            return;
        }
    }
    printf("watchpoint %d hit (RIP=0x%llx)\n", slot, (unsigned long long)rip);
}

void print_watchpoints(debugger_t *dbg)
{
    if (dbg->watch_count == 0)
    {
        printf("no watchpoints set\n");
        return;
    }
    for (int i = 0; i < dbg->watch_count; i++)
    {
        watchpoint_t *wp = &dbg->watchpoints[i];
        const char *type_str = (wp->type == WATCH_RW) ? "read/write" : "write";
        printf("  wp%d  addr=%p  %s  size=%d\n",
               wp->slot, wp->addr, type_str, wp->size);
    }
}
