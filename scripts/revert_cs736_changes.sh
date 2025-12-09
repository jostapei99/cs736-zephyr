#!/bin/bash
# 
# Revert all CS736 custom scheduler changes
# This comments out all modifications to return to clean Zephyr
#

ZEPHYR_BASE="/home/jack/cs736-project/zephyr"

echo "Reverting CS736 custom scheduler changes..."

# Backup originals
echo "Creating backups..."
cp "${ZEPHYR_BASE}/kernel/Kconfig" "${ZEPHYR_BASE}/kernel/Kconfig.cs736_backup"
cp "${ZEPHYR_BASE}/kernel/sched.c" "${ZEPHYR_BASE}/kernel/sched.c.cs736_backup"
cp "${ZEPHYR_BASE}/kernel/include/priority_q.h" "${ZEPHYR_BASE}/kernel/include/priority_q.h.cs736_backup"

echo "Step 1: Rename custom header files (so they won't be included)"
if [ -f "${ZEPHYR_BASE}/include/zephyr/kernel/sched_rt.h" ]; then
    mv "${ZEPHYR_BASE}/include/zephyr/kernel/sched_rt.h" \
       "${ZEPHYR_BASE}/include/zephyr/kernel/sched_rt.h.disabled"
    echo "  - Disabled sched_rt.h"
fi

if [ -f "${ZEPHYR_BASE}/include/zephyr/kernel/rt_stats.h" ]; then
    mv "${ZEPHYR_BASE}/include/zephyr/kernel/rt_stats.h" \
       "${ZEPHYR_BASE}/include/zephyr/kernel/rt_stats.h.disabled"
    echo "  - Disabled rt_stats.h"
fi

if [ -f "${ZEPHYR_BASE}/kernel/sched_rt.c" ]; then
    mv "${ZEPHYR_BASE}/kernel/sched_rt.c" \
       "${ZEPHYR_BASE}/kernel/sched_rt.c.disabled"
    echo "  - Disabled sched_rt.c"
fi

echo ""
echo "Step 2: Comment out Kconfig entries"
echo "  - Edit kernel/Kconfig manually to comment out lines 120-268"
echo "  - Or use the provided kernel/Kconfig.clean if available"

echo ""
echo "Step 3: Clean build directories"
find "${ZEPHYR_BASE}/app" -name "build" -type d -exec rm -rf {} + 2>/dev/null || true
echo "  - Cleaned all build directories"

echo ""
echo "================================"
echo "Manual steps required:"
echo "================================"
echo "1. Edit kernel/Kconfig - comment out lines 120-268 (custom scheduler configs)"
echo "2. Edit kernel/sched.c - comment out all #ifdef CONFIG_736_RT_STATS blocks"
echo "3. Edit kernel/include/priority_q.h - comment out custom scheduler comparison functions"
echo "4. Edit kernel/CMakeLists.txt - comment out: zephyr_library_sources_ifdef(CONFIG_736 sched_rt.c)"
echo ""
echo "Backups created with .cs736_backup extension"
echo "Custom headers disabled with .disabled extension"
echo ""
echo "To re-enable: rename .disabled files back to .h and uncomment code"
