# H1.17 — K-1 Mark II ten warm reconnects

Camera: PENTAX K-1 Mark II firmware 1.02 in PC-P mode, physical dial `M`.
Serial omitted.

Ten independent sessions ran one second apart without powering off the camera.
Every session enabled Pentax vendor mode with response `0x2001` and function
flags 3, issued one read-only `GetAllConditions`, returned a complete parsed
conditions widget, disabled vendor mode during exit, and released USB.

All ten condition values were identical: operation state 0, exposure mode raw 8
(IT2 Manual), drive raw 4, ISO 200, exposure step 1, `openAvNum` 14,
Tv-changeable, not shooting, not processing, not task-changing, Bulb timer off,
shutter 1/500, Astro/GPS flags inactive.

Status: **WARM RECONNECT PASS 10/10**. This does not satisfy the separate ten
operator-observed cold-start cycles or the later 50-cycle gate.
