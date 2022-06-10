#include <arch/x86/boot/multiboot.h>
#include <lunaix/common.h>
#include <lunaix/lunistd.h>
#include <lunaix/mm/vmm.h>
#include <lunaix/peripheral/ps2kbd.h>
#include <lunaix/spike.h>
#include <lunaix/syscall.h>
#include <lunaix/syslog.h>
#include <stddef.h>

#include <hal/acpi/acpi.h>
#include <hal/apic.h>
#include <hal/ioapic.h>

LOG_MODULE("PROC0")

extern void
_lxinit_main(); /* lxinit.c */

void
init_platform();

void
lock_reserved_memory();

/**
 * @brief LunaixOS的零号进程，该进程永远为可执行。
 *
 * 这主要是为了保证调度器在没有进程可调度时依然有事可做。
 *
 * 同时，该进程也负责fork出我们的init进程。
 *
 */
void
__proc0()
{
    init_platform();
    if (!fork()) {
        asm("jmp _lxinit_main");
    }

    asm("1: jmp 1b");
}

extern uint8_t __kernel_start;            /* link/linker.ld */
extern uint8_t __kernel_end;              /* link/linker.ld */
extern uint8_t __init_hhk_end;            /* link/linker.ld */
extern multiboot_info_t* _k_init_mb_info; /* k_init.c */

void
init_platform()
{
    assert_msg(kalloc_init(), "Fail to initialize heap");

    size_t hhk_init_pg_count = ((uintptr_t)(&__init_hhk_end)) >> PG_SIZE_BITS;
    kprintf(KINFO "[MM] Releaseing %d pages from 0x0.\n", hhk_init_pg_count);

    // Fuck it, I will no longer bother this little 1MiB
    // I just release 4 pages for my APIC & IOAPIC remappings
    for (size_t i = 0; i < 3; i++) {
        vmm_unmap_page(KERNEL_PID, (void*)(i << PG_SIZE_BITS));
    }

    // 锁定所有系统预留页（内存映射IO，ACPI之类的），并且进行1:1映射
    lock_reserved_memory();

    acpi_init(_k_init_mb_info);
    uintptr_t ioapic_addr = acpi_get_context()->madt.ioapic->ioapic_addr;
    pmm_mark_page_occupied(
      KERNEL_PID, FLOOR(__APIC_BASE_PADDR, PG_SIZE_BITS), 0);
    pmm_mark_page_occupied(KERNEL_PID, FLOOR(ioapic_addr, PG_SIZE_BITS), 0);

    vmm_set_mapping(KERNEL_PID, APIC_BASE_VADDR, __APIC_BASE_PADDR, PG_PREM_RW);
    vmm_set_mapping(KERNEL_PID, IOAPIC_BASE_VADDR, ioapic_addr, PG_PREM_RW);

    apic_init();
    ioapic_init();
    timer_init(SYS_TIMER_FREQUENCY_HZ);
    clock_init();
    ps2_kbd_init();

    syscall_install();

    for (size_t i = 256; i < hhk_init_pg_count; i++) {
        vmm_unmap_page(KERNEL_PID, (void*)(i << PG_SIZE_BITS));
    }
}

void
lock_reserved_memory()
{
    multiboot_memory_map_t* mmaps = _k_init_mb_info->mmap_addr;
    size_t map_size =
      _k_init_mb_info->mmap_length / sizeof(multiboot_memory_map_t);
    for (unsigned int i = 0; i < map_size; i++) {
        multiboot_memory_map_t mmap = mmaps[i];
        if (mmap.type == MULTIBOOT_MEMORY_AVAILABLE) {
            continue;
        }
        uint8_t* pa = PG_ALIGN(mmap.addr_low);
        size_t pg_num = CEIL(mmap.len_low, PG_SIZE_BITS);
        for (size_t j = 0; j < pg_num; j++) {
            vmm_set_mapping(KERNEL_PID,
                            (pa + (j << PG_SIZE_BITS)),
                            (pa + (j << PG_SIZE_BITS)),
                            PG_PREM_R);
        }
    }
}