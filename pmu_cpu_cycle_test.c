#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/smp.h>

// PMCR: Performance Monitors Control Register
#define ARMV8_PMCR_MASK 0x3f
#define ARMV8_PMCR_E (1 << 0) // E, bit [0] -> 0b1 [Enable all event counters, including PMCCNTR_EL0]
#define ARMV8_PMCR_P (1 << 1) // P, bit [1] -> 0b1 [Reset all event counters except PMCCNTR_EL0]
#define ARMV8_PMCR_C (1 << 2) // C, bit [2] -> 0b1 [Reset PMCCNTR_EL0 counter]

// PMUSERENR: Performance Monitors USER ENable Register
#define ARMV8_PMUSERENR_EN (1 << 0) // EN, bit [0] -> 0b1 [Traps access enable]
#define ARMV8_PMUSERENR_CR (1 << 2) // CR, bit [2] -> 0b1 [Cycle counter read access enable]
#define ARMV8_PMUSERENR_ER (1 << 3) // ER, bit [3] -> 0b1 [Event counter read access enable]

// PMINTENSET: Performance Monitors INTerrupt ENable SET register
#define ARMV8_PMINTENSET_DISABLE (0 << 31) // C, bit[31] -> 0b0 [cycle counter overflow interrupt disabled]

// PMCNTENSET: Performance Monitors CouNT ENable SET register
#define ARMV8_PMCNTENSET_ENABLE (1 << 31)  // C, bit[31] -> 0b1 [cycle counter enabled]
#define ARMV8_PMCNTENSET_DISABLE (0 << 31) // C, bit[31] -> 0b0 [cycle counter disabled]

#define SIZE 3

static void pmu_pmcr_write(u32 value)
{
    value &= ARMV8_PMCR_MASK;
    asm volatile("isb" : : : "memory");
    asm volatile("MSR PMCR_EL0, %0" : : "r"((u64)value));
}

static u32 pmu_pmccntr_read(void)
{
    u32 value;
    // PMCCNTR: Performance Monitors Cycle CouNT Register
    // Read the cycle counter
    asm volatile("MRS %0, PMCCNTR_EL0" : "=r"(value));
    return value;
}

static void matrix_mul(void)
{
    u8 rand_num;
    int a[SIZE][SIZE], b[SIZE][SIZE], result[SIZE][SIZE];
    u8 i, j, k;

    for (i = 0; i < SIZE; i++)
    {
        for (j = 0; j < SIZE; j++)
        {
            get_random_bytes(&rand_num, sizeof(rand_num));
            a[i][j] = rand_num % 10;
        }
    }

    for (i = 0; i < SIZE; i++)
    {
        for (j = 0; j < SIZE; j++)
        {
            get_random_bytes(&rand_num, sizeof(rand_num));
            b[i][j] = rand_num % 10;
        }
    }

    for (i = 0; i < SIZE; i++)
    {
        for (j = 0; j < SIZE; j++)
        {
            result[i][j] = 0;
        }
    }

    for (i = 0; i < SIZE; i++)
    {
        for (j = 0; j < SIZE; j++)
        {
            for (k = 0; k < SIZE; k++)
            {
                result[i][j] += a[i][k] * b[k][j];
            }
        }
    }
}

static void enable_cpu_counter(void)
{
    // Enable PMU user read access
    asm volatile("MSR PMUSERENR_EL0, %0" : : "r"((u64)ARMV8_PMUSERENR_EN | ARMV8_PMUSERENR_ER | ARMV8_PMUSERENR_CR));
    // Init & reset PMU control
    pmu_pmcr_write(ARMV8_PMCR_P | ARMV8_PMCR_C);
    // Disable PMU cycle counter overflow interrupt
    asm volatile("MSR PMINTENSET_EL1, %0" : : "r"((u64)ARMV8_PMINTENSET_DISABLE));
    // Enable PMU cycle counter
    asm volatile("MSR PMCNTENSET_el0, %0" : : "r"((u64)ARMV8_PMCNTENSET_ENABLE));
    // Enable PMU control
    pmu_pmcr_write(ARMV8_PMCR_E);

    printk(KERN_INFO "PMU access enabled.");
}

static void disable_cpu_counter(void)
{
    // Disable PMU cycle counter
    asm volatile("MSR PMCNTENSET_EL0, %0" : : "r"((u64)ARMV8_PMCNTENSET_DISABLE));
    // Disable PMU control
    pmu_pmcr_write(~ARMV8_PMCR_E); // '~'=not
    // Disable PMU user read access
    asm volatile("MSR PMUSERENR_EL0, %0" : : "r"((u64)0)); // all set 0

    printk(KERN_INFO "PMU access disabled.");
}

static void cpu_cycle_test_pmu(void)
{
    u32 cycles_before, cycles_after;

    // Save current cpu cycle
    cycles_before = pmu_pmccntr_read();

    // Simulate some work
    matrix_mul();

    // Save current cpu cycle(after run function)
    cycles_after = pmu_pmccntr_read();

    printk(KERN_INFO "PMU Test - CPU cycle count: %u\n", cycles_after - cycles_before);
}

static int __init init_pmu(void)
{
    enable_cpu_counter();

    cpu_cycle_test_pmu();

    return 0;
}

static void __exit exit_pmu(void)
{
    disable_cpu_counter();
}

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Enables PMU CPU cycle counter and test matrix multiplication function");
MODULE_VERSION("1:0.0");
MODULE_AUTHOR("MAPLELEAF3659");

module_init(init_pmu);
module_exit(exit_pmu);