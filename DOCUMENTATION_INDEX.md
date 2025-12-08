# Documentation Index

Complete guide to the Real-Time Scheduling implementation for Zephyr RTOS.

## Documentation Map

### Getting Started (Start Here!)

| Document | Time | Purpose |
|----------|------|---------|
| **[QUICK_START.md](QUICK_START.md)** | 5 min | Get your first RT task running |
| **[RT_SCHEDULER_README.md](RT_SCHEDULER_README.md)** | 10 min | Project overview and features |
| **[RT_STATISTICS_QUICK_REF.md](RT_STATISTICS_QUICK_REF.md)** | 3 min | Statistics API reference card |

### User Documentation

| Document | Audience | Content |
|----------|----------|---------|
| **[SCHEDULER_USER_GUIDE.md](SCHEDULER_USER_GUIDE.md)** | Developers | Complete usage guide with examples |
| **[RT_STATISTICS_GUIDE.md](RT_STATISTICS_GUIDE.md)** | Developers | Comprehensive statistics documentation |
| **[QUICK_REFERENCE.md](QUICK_REFERENCE.md)** | Everyone | API and config cheat sheet |

### Technical Documentation

| Document | Audience | Content |
|----------|----------|---------|
| **[ALGORITHM_COMPARISON.md](ALGORITHM_COMPARISON.md)** | Researchers | Detailed algorithm comparison |
| **[MODULAR_DESIGN.md](MODULAR_DESIGN.md)** | Developers | Implementation architecture |
| **[NEW_ALGORITHMS.md](NEW_ALGORITHMS.md)** | Researchers | LLF and PFS deep dive |
| **[RT_STATISTICS_IMPLEMENTATION.md](RT_STATISTICS_IMPLEMENTATION.md)** | Kernel developers | Statistics implementation details |

### API Documentation

| Location | Content |
|----------|---------|
| `include/zephyr/kernel/sched_rt.h` | RT scheduler API definitions |
| `include/zephyr/kernel/rt_stats.h` | Statistics API definitions |
| `doc/kernel/services/scheduling/rt_schedulers.rst` | Sphinx documentation |

## Documentation by Use Case

### "I want to use these schedulers in my project"

1. **Start**: [QUICK_START.md](QUICK_START.md) - Get running in 5 minutes
2. **Learn**: [SCHEDULER_USER_GUIDE.md](SCHEDULER_USER_GUIDE.md) - Complete usage guide
3. **Reference**: [QUICK_REFERENCE.md](QUICK_REFERENCE.md) - Keep this handy

### "I'm doing research on RT scheduling"

1. **Overview**: [RT_SCHEDULER_README.md](RT_SCHEDULER_README.md) - Project summary
2. **Algorithms**: [ALGORITHM_COMPARISON.md](ALGORITHM_COMPARISON.md) - Detailed comparison
3. **New Algorithms**: [NEW_ALGORITHMS.md](NEW_ALGORITHMS.md) - LLF and PFS analysis
4. **Testing**: `app/simple_eval_step1/` and `app/advanced_eval/` - Evaluation framework

### "I want to extend or modify the implementation"

1. **Architecture**: [MODULAR_DESIGN.md](MODULAR_DESIGN.md) - How it's built
2. **API**: `include/zephyr/kernel/sched_rt.h` - Interface definitions
3. **Implementation**: `kernel/include/priority_q.h` - Algorithm comparison functions
4. **Configuration**: `kernel/Kconfig` - Build-time options

### "I'm learning about RT scheduling"

1. **Start**: [QUICK_START.md](QUICK_START.md) - Hands-on introduction
2. **Concepts**: [ALGORITHM_COMPARISON.md](ALGORITHM_COMPARISON.md) - Algorithm explanations
3. **Examples**: `samples/scheduler_example/` - Working code
4. **Practice**: [SCHEDULER_USER_GUIDE.md](SCHEDULER_USER_GUIDE.md) - Complete examples

## Complete File Guide

### Core Implementation

```
kernel/
├── sched.c                          # Syscall implementations (z_impl_k_thread_weight_set, etc.)
├── Kconfig                          # Scheduler selection menu
└── include/
    └── priority_q.h                # Algorithm comparison functions

include/zephyr/
├── kernel.h                         # Public API declarations
├── kernel/
│   ├── thread.h                    # Thread structure definitions
│   └── sched_rt.h                  # RT scheduling API (new)
```

### Documentation

```
Documentation/
├── RT_SCHEDULER_README.md          # Main README
├── QUICK_START.md                  # 5-minute getting started
├── SCHEDULER_USER_GUIDE.md         # Complete usage guide (40+ pages)
├── ALGORITHM_COMPARISON.md         # Algorithm analysis
├── MODULAR_DESIGN.md               # Architecture explanation
├── NEW_ALGORITHMS.md               # LLF & PFS details
├── QUICK_REFERENCE.md              # Cheat sheet
└── doc/kernel/services/scheduling/
    └── rt_schedulers.rst           # Sphinx documentation
```

### Test Applications

```
app/
├── simple_eval_step1/              # Basic evaluation (50 activations)
│   ├── src/main.c                 # Test application
│   ├── prj.conf                   # Configuration
│   ├── scripts/
│   │   ├── run_all_tests.sh      # Automated testing
│   │   └── generate_graphs.py    # Visualization
│   └── results/                   # Test output (generated)
│
├── advanced_eval/                  # Advanced evaluation (100 activations)
│   └── (similar structure)
│
└── samples/scheduler_example/      # Simple example
    ├── src/main.c
    ├── prj.conf
    └── README.md
```

## 🔍 Quick Lookups

### "How do I configure scheduler X?"

See: [QUICK_REFERENCE.md](QUICK_REFERENCE.md) → Configuration Cheat Sheet

### "What's the API for setting weights?"

See: `include/zephyr/kernel/sched_rt.h` or [SCHEDULER_USER_GUIDE.md](SCHEDULER_USER_GUIDE.md) → API Overview

### "How do the algorithms compare?"

See: [ALGORITHM_COMPARISON.md](ALGORITHM_COMPARISON.md) → Algorithm Summary Table

### "How do I run the tests?"

See: [SCHEDULER_USER_GUIDE.md](SCHEDULER_USER_GUIDE.md) → Testing and Evaluation

### "How is this implemented?"

See: [MODULAR_DESIGN.md](MODULAR_DESIGN.md) → Implementation Architecture

### "Why choose LLF over EDF?"

See: [NEW_ALGORITHMS.md](NEW_ALGORITHMS.md) → Least Laxity First

### "What workloads are available?"

See: `app/simple_eval_step1/include/workloads.h`

## Documentation Statistics

| Document | Lines | Pages | Level |
|----------|-------|-------|-------|
| QUICK_START.md | ~200 | 4 | Beginner |
| RT_SCHEDULER_README.md | ~400 | 8 | Beginner |
| QUICK_REFERENCE.md | ~300 | 6 | All Levels |
| SCHEDULER_USER_GUIDE.md | ~2000 | 40+ | Intermediate |
| ALGORITHM_COMPARISON.md | ~600 | 12 | Advanced |
| MODULAR_DESIGN.md | ~400 | 8 | Advanced |
| NEW_ALGORITHMS.md | ~500 | 10 | Advanced |

**Total**: ~4,400 lines of documentation

## 🎓 Learning Path

### Beginner Path (1-2 hours)

1. Read [RT_SCHEDULER_README.md](RT_SCHEDULER_README.md) (10 min)
2. Follow [QUICK_START.md](QUICK_START.md) (30 min)
3. Try `samples/scheduler_example/` (30 min)
4. Experiment with different schedulers (30 min)

### Intermediate Path (4-6 hours)

1. Complete Beginner Path
2. Read [SCHEDULER_USER_GUIDE.md](SCHEDULER_USER_GUIDE.md) sections 1-6 (2 hours)
3. Build and run `app/simple_eval_step1/` (1 hour)
4. Analyze test results (1 hour)
5. Create your own test workload (1-2 hours)

### Advanced Path (8-12 hours)

1. Complete Intermediate Path
2. Study [ALGORITHM_COMPARISON.md](ALGORITHM_COMPARISON.md) (2 hours)
3. Read [MODULAR_DESIGN.md](MODULAR_DESIGN.md) and [NEW_ALGORITHMS.md](NEW_ALGORITHMS.md) (2 hours)
4. Run full evaluation suite (2 hours)
5. Analyze all algorithms across all workloads (2-4 hours)
6. Implement a custom scheduler (2-4 hours)

### Research Path (20+ hours)

1. Complete Advanced Path
2. Deep dive into all documentation
3. Run extensive benchmarks
4. Analyze results statistically
5. Write research paper
6. Implement novel algorithms

## 🛠️ Maintenance

### Updating Documentation

When adding new features:

1. Update relevant user guide sections
2. Add examples if needed
3. Update quick reference
4. Update this index if new files added

### Documentation Standards

- **Code examples**: Always tested and working
- **API references**: Match actual implementation
- **Screenshots**: Update when UI changes
- **Links**: Check for broken links

## Getting Help

### Documentation Issues

If documentation is unclear:

1. Check [SCHEDULER_USER_GUIDE.md](SCHEDULER_USER_GUIDE.md) → Troubleshooting
2. Review examples in `samples/`
3. Check test applications in `app/`

### Common Questions

| Question | Answer Location |
|----------|----------------|
| How do I choose a scheduler? | [QUICK_REFERENCE.md](QUICK_REFERENCE.md) → Algorithm Decision Tree |
| What's the syntax for...? | `include/zephyr/kernel/sched_rt.h` |
| Why is my task not running? | [SCHEDULER_USER_GUIDE.md](SCHEDULER_USER_GUIDE.md) → Troubleshooting |
| How do I test schedulers? | [SCHEDULER_USER_GUIDE.md](SCHEDULER_USER_GUIDE.md) → Testing and Evaluation |
| What's the performance overhead? | [ALGORITHM_COMPARISON.md](ALGORITHM_COMPARISON.md) → Complexity Analysis |

## Document Status

| Document | Status | Last Updated |
|----------|--------|--------------|  
| All documentation | Complete | Dec 2025 |
| Code examples | Tested | Dec 2025 |
| Test framework | Working | Dec 2025 |## Navigation Tips

- **New users**: Start with bold links above
- **Quick lookup**: Use Ctrl+F in this file to find topics
- **API reference**: Always in header files (`*.h`)
- **Examples**: Always in `samples/` or `app/` directories
- **Testing**: Scripts in `app/*/scripts/`

---

**Not sure where to start?** → [QUICK_START.md](QUICK_START.md)

**Want comprehensive guide?** → [SCHEDULER_USER_GUIDE.md](SCHEDULER_USER_GUIDE.md)

**Need algorithm details?** → [ALGORITHM_COMPARISON.md](ALGORITHM_COMPARISON.md)

**Just need quick reference?** → [QUICK_REFERENCE.md](QUICK_REFERENCE.md)
