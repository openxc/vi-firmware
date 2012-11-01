define hook-step
    mon cortex_m3 maskisr on
end

define hookpost-step
    mon cortex_m3 maskisr off
end

define hook-next
    mon cortex_m3 maskisr on
end

define hookpost-next
    mon cortex_m3 maskisr off
end

set remote hardware-breakpoint-limit 6
set remote hardware-watchpoint-limit 4

target remote localhost:3333

echo .gdbinit for cantranslator has been executed \n
