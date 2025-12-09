# CS736 Testing Directory

This directory contains unit tests for each custom real-time scheduling algorithm implemented in Zephyr RTOS.

## Directory Structure

```
app/testing/
├── test_weighted_edf/    # Weighted EDF scheduler tests
├── test_rms/             # Rate Monotonic Scheduling tests
├── test_wsrt/            # Weighted Shortest Remaining Time tests
├── test_llf/             # Least Laxity First tests
└── test_pfs/             # Proportional Fair Scheduling tests
```

## Test Applications

### 1. Weighted EDF (test_weighted_edf)
Tests the weighted earliest deadline first scheduler.

**Configuration:** `CONFIG_736_MOD_EDF=y`

**Tests:**
- Test 1: Same deadline, different weights
- Test 2: Different deadlines and weights

**Expected Behavior:** Threads with lower deadline/weight ratios execute first.

**Build and Run:**
```bash
cd app/testing/test_weighted_edf
west build -b native_sim -p
west build -t run
```

### 2. Rate Monotonic Scheduling (test_rms)
Tests the RMS scheduler with static priorities based on execution time.

**Configuration:** `CONFIG_736_RMS=y`

**Tests:**
- Three threads with different execution times (10ms, 50ms, 100ms)

**Expected Behavior:** Threads with shorter execution times execute first.

**Build and Run:**
```bash
cd app/testing/test_rms
west build -b native_sim -p
west build -t run
```

### 3. Weighted Shortest Remaining Time (test_wsrt)
Tests the WSRT scheduler with dynamic priorities.

**Configuration:** `CONFIG_736_WSRT=y`

**Tests:**
- Test 1: Same time_left, different weights
- Test 2: Different time_left and weights

**Expected Behavior:** Threads with lower time_left/weight ratios execute first.

**Build and Run:**
```bash
cd app/testing/test_wsrt
west build -b native_sim -p
west build -t run
```

### 4. Least Laxity First (test_llf)
Tests the LLF scheduler based on slack time.

**Configuration:** `CONFIG_736_LLF=y`

**Tests:**
- Three threads with different laxity values (deadline - time_left)

**Expected Behavior:** Threads with lower laxity (more urgent) execute first.

**Build and Run:**
```bash
cd app/testing/test_llf
west build -b native_sim -p
west build -t run
```

### 5. Proportional Fair Scheduling (test_pfs)
Tests the PFS scheduler for fair CPU allocation.

**Configuration:** `CONFIG_736_PFS=y`

**Tests:**
- Test 1: Different runtimes, same weight
- Test 2: Same runtime, different weights (fairness)

**Expected Behavior:** Threads with lower virtual_runtime (runtime/weight) execute first.

**Build and Run:**
```bash
cd app/testing/test_pfs
west build -b native_sim -p
west build -t run
```

## Running All Tests

A convenience script is provided to run all scheduler tests:

```bash
./scripts/test_all_schedulers.sh
```

This script will:
1. Build each test application
2. Run the test
3. Display relevant output
4. Provide a summary of all tests

## Test Output

Each test application prints:
- Test description and expected behavior
- Execution order with thread parameters
- Verification that the scheduler is working correctly

Example output:
```
*** Weighted EDF Scheduler Test ***
Testing CONFIG_736_MOD_EDF
Expected order: High(300) -> Med(200) -> Low(100)
[Order 0] High Weight Thread: weight=300, deadline=1000, ratio=3
[Order 1] Med Weight Thread: weight=200, deadline=1000, ratio=5
[Order 2] Low Weight Thread: weight=100, deadline=1000, ratio=10
*** Test Complete ***
```

## Adding New Tests

To add a new test application:

1. Create a new directory in `app/testing/test_<algorithm>/`
2. Add the standard Zephyr application structure:
   - `CMakeLists.txt`
   - `prj.conf`
   - `src/main.c`
3. Configure the appropriate scheduler in `prj.conf`
4. Implement test scenarios in `main.c`
5. Update this README

## Related Documentation

- **Scheduler Algorithms:** `docs_cs736_project/SCHEDULING_ALGORITHMS.md`
- **API Reference:** See individual algorithm sections in SCHEDULING_ALGORITHMS.md
- **Iterative Development:** `docs_cs736_project/ITERATIVE_DEVELOPMENT.md`

## Troubleshooting

### Build Errors
- Ensure you're using the correct Zephyr SDK version
- Activate the virtual environment: `source .venv/bin/activate`
- Clean build: `west build -b native_sim -p`

### Test Failures
- Check that only one scheduler CONFIG is enabled at a time
- Verify thread parameters are set correctly
- Use `printk` to debug thread execution order

### Syscall Errors
- Ensure `CONFIG_736_ADD_ONS` is enabled (automatic with custom schedulers)
- Check that syscall headers are properly generated
- Verify kernel/CMakeLists.txt includes sched_rt.c

## Notes

- Only one scheduler can be active at a time (mutually exclusive Kconfig options)
- All tests use the `native_sim` board for portability
- Tests verify scheduling order, not real-time performance metrics
- For performance evaluation, see `app/workloads/` directory
