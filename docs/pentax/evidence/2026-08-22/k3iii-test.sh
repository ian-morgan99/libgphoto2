#!/usr/bin/env bash
# K-3 Mark III bounded hardware test — fail-closed, evidence-logging.
# Policy: never judge by gphoto2 exit code ("Unspecified error" may print on
# success); always judge by re-read values via pentaxconditions/widget.
# Max ONE retry per write. HARD FORBIDDEN: --capture-image, focus writes,
# astrotracer, pentaxliveviewzoom writes, live view start.

set -u

B=/home/ian/Documents/VSCodeProjects/LibGphoto2/libgphoto2/_build
export LD_LIBRARY_PATH=$B/libgphoto2:$B/libgphoto2_port/libgphoto2_port
export CAMLIBS=$B/camlibs IOLIBS=$B/libgphoto2_port/libusb1
CAMERA="Pentax:K-3 Mark III (MTP mode)"
PORT="usb:001,005"
LOG=/home/ian/Documents/VSCodeProjects/LibGphoto2/docs/pentax/evidence/2026-08-22/K3III-session-log.md

ts() { date '+%Y-%m-%d %H:%M:%S'; }

gp() {
    echo "[$(ts)] \$ gphoto2 --camera \"$CAMERA\" --port \"$PORT\" $*" >> "$LOG"
    local out rc
    out=$(timeout 45 gphoto2 --camera "$CAMERA" --port "$PORT" "$@" 2>&1)
    rc=$?
    echo "[$(ts)] exit=$rc (informational only)" >> "$LOG"
    echo "$out" >> "$LOG"
    printf '%s' "$out"
}

cond_get() { # regex -> matching field from full conditions text
    gp --get-config /main/status/pentaxconditions 2>/dev/null \
        | grep -oE "$1" | head -n1
}

widget_current() { # configpath -> Current value
    gp --get-config "$1" 2>/dev/null | awk -F': ' '/^Current/{print $2}'
}
widget_choices() {
    gp --get-config "$1" 2>/dev/null | sed -n 's/^Choice: [0-9]* //p'
}

PASS=0; FAIL=0
declare -a RESULTS=()
record() {
    if [ "$2" = PASS ]; then PASS=$((PASS+1)); else FAIL=$((FAIL+1)); fi
    RESULTS+=("$1 $2 $3")
    echo "[$(ts)] RESULT $1 $2 : $3" >> "$LOG"
}

write_with_retry() { # cfg value -> 0 if re-read matches
    local cfg="$1" val="$2" try cur
    for try in 1 2; do
        gp --set-config "$cfg=$val" >/dev/null
        sleep 1
        cur=$(widget_current "$cfg")
        if [ "$cur" = "$val" ]; then return 0; fi
        echo "[$(ts)] write attempt $try to $cfg=$val failed (read back '$cur')" >> "$LOG"
    done
    return 1
}

preflight() {
    echo "=== Ownership preflight ===" | tee -a "$LOG"
    local pids
    pids=$(fuser /dev/bus/usb/001/025 2>/dev/null)
    if [ -n "${pids// /}" ]; then
        echo "[$(ts)] killing gvfs PIDs: $pids" >> "$LOG"
        kill $pids 2>/dev/null; sleep 2
        pids=$(fuser /dev/bus/usb/001/025 2>/dev/null)
        if [ -n "${pids// /}" ]; then
            echo "[$(ts)] ABORT: still owned by $pids" >> "$LOG"; exit 2
        fi
    fi
    echo "[$(ts)] preflight OK" >> "$LOG"
}

T1_inventory() {
    echo "--- T1 read-only inventory ---" >> "$LOG"
    local ok=0 bad=0 w
    for w in pentaxconditions pentaxliveviewcontrols pentaxdirectiso \
             pentaxdirectshutter pentaxdirectaperture pentaxdirectev \
             pentaxdrivemode pentaxdirectwb; do
        if gp --get-config "/main/status/$w" >/dev/null 2>&1; then
            ok=$((ok+1))
            echo "[$(ts)] $w = $(widget_current /main/status/$w)" >> "$LOG"
        else bad=$((bad+1)); fi
    done
    [ "$bad" -eq 0 ] && record T1 PASS "inventory ok=$ok" \
                     || record T1 FAIL "ok=$ok bad=$bad"
}

T2_iso() {
    local orig tgt after
    orig=$(cond_get 'ISO=[0-9]+' | cut -d= -f2)
    [ -z "$orig" ] && { record T2 FAIL "no initial ISO"; return; }
    tgt=$([ "$orig" = "3200" ] && echo 1600 || echo 3200)
    echo "--- T2 ISO $orig->$tgt->$orig ---" >> "$LOG"
    write_with_retry /main/status/pentaxdirectiso "$tgt" || { record T2 FAIL "write $tgt"; return; }
    after=$(cond_get 'ISO=[0-9]+' | cut -d= -f2)
    [ "$after" != "$tgt" ] && { record T2 FAIL "verify: got '$after' want '$tgt'"; return; }
    write_with_retry /main/status/pentaxdirectiso "$orig" || { record T2 FAIL "restore"; return; }
    after=$(cond_get 'ISO=[0-9]+' | cut -d= -f2)
    [ "$after" = "$orig" ] && record T2 PASS "ISO $orig->$tgt->$orig" \
                           || record T2 FAIL "restore verify: '$after'"
}

T3_aperture() {
    local orig tgt after
    orig=$(widget_current /main/status/pentaxdirectaperture)
    [ -z "$orig" ] && { record T3 FAIL "no aperture read"; return; }
    tgt=$([ "$orig" = "f/3.5" ] && echo "f/4" || echo "f/3.5")
    echo "--- T3 aperture $orig->$tgt->$orig ---" >> "$LOG"
    write_with_retry /main/status/pentaxdirectaperture "$tgt" || { record T3 FAIL "write $tgt"; return; }
    after=$(cond_get 'aperture=[0-9]+/[0-9]+')
    echo "[$(ts)] conditions after write: $after" >> "$LOG"
    write_with_retry /main/status/pentaxdirectaperture "$orig" || { record T3 FAIL "restore"; return; }
    after=$(widget_current /main/status/pentaxdirectaperture)
    [ "$after" = "$orig" ] && record T3 PASS "aperture $orig->$tgt->$orig" \
                           || record T3 FAIL "restore verify: '$after'"
}

T4_ev() {
    local orig tgt after choices
    orig=$(widget_current /main/status/pentaxdirectev)
    choices=$(widget_choices /main/status/pentaxdirectev)
    [ -z "$orig" ] && { record T4 FAIL "no EV read"; return; }
    tgt=$(printf '%s\n' "$choices" | grep -Fx "+0.3")
    [ -z "$tgt" ] && tgt=$(printf '%s\n' "$choices" | grep -Fvx "$orig" | head -n1)
    [ -z "$tgt" ] && { record T4 FAIL "no alternative EV choice"; return; }
    echo "--- T4 EV '$orig'->'$tgt'->'$orig' ---" >> "$LOG"
    write_with_retry /main/status/pentaxdirectev "$tgt" || { record T4 FAIL "write $tgt"; return; }
    after=$(cond_get 'exposure-comp=[-0-9]+/[0-9]+')
    echo "[$(ts)] conditions after write: $after" >> "$LOG"
    write_with_retry /main/status/pentaxdirectev "$orig" || { record T4 FAIL "restore"; return; }
    after=$(widget_current /main/status/pentaxdirectev)
    [ "$after" = "$orig" ] && record T4 PASS "EV '$orig'->'$tgt'->'$orig'" \
                           || record T4 FAIL "restore verify: '$after'"
}

T5_shutter() {
    local orig tgt after choices
    orig=$(widget_current /main/status/pentaxdirectshutter)
    choices=$(widget_choices /main/status/pentaxdirectshutter)
    [ -z "$orig" ] && { record T5 FAIL "no shutter read"; return; }
    tgt=$(printf '%s\n' "$choices" | grep -Fvx "$orig" | tail -n1)
    [ -z "$tgt" ] && { record T5 FAIL "no neighbor choice"; return; }
    echo "--- T5 shutter '$orig'->'$tgt'->'$orig' ---" >> "$LOG"
    write_with_retry /main/status/pentaxdirectshutter "$tgt" || { record T5 FAIL "write '$tgt'"; return; }
    after=$(cond_get 'bulb-seconds=[0-9/]+')
    echo "[$(ts)] bulb-seconds after write: '${after:-none}'" >> "$LOG"
    write_with_retry /main/status/pentaxdirectshutter "$orig" || { record T5 FAIL "restore"; return; }
    after=$(widget_current /main/status/pentaxdirectshutter)
    [ "$after" = "$orig" ] && record T5 PASS "shutter '$orig'->'$tgt'->'$orig'" \
                           || record T5 FAIL "restore verify: '$after'"
}

T6_wb() {
    local orig tgt after
    orig=$(widget_current /main/status/pentaxdirectwb)
    [ -z "$orig" ] && { record T6 FAIL "no WB read"; return; }
    tgt=$([ "$orig" = "daylight" ] && echo "cloudy" || echo "daylight")
    echo "--- T6 WB '$orig'->'$tgt'->'$orig' ---" >> "$LOG"
    write_with_retry /main/status/pentaxdirectwb "$tgt" || { record T6 FAIL "write $tgt"; return; }
    after=$(widget_current /main/status/pentaxdirectwb)
    [ "$after" != "$tgt" ] && { record T6 FAIL "verify: got '$after'"; return; }
    write_with_retry /main/status/pentaxdirectwb "$orig" || { record T6 FAIL "restore"; return; }
    after=$(widget_current /main/status/pentaxdirectwb)
    [ "$after" = "$orig" ] && record T6 PASS "WB '$orig'->'$tgt'->'$orig'" \
                           || record T6 FAIL "restore verify: '$after'"
}

T7_drive() {
    local orig tgt after
    orig=$(widget_current /main/status/pentaxdrivemode)
    [ -z "$orig" ] && { record T7 FAIL "no drive read"; return; }
    tgt=$([ "$orig" = "single" ] && echo "continuous-lo" || echo "single")
    echo "--- T7 drive '$orig'->'$tgt'->'$orig' ---" >> "$LOG"
    write_with_retry /main/status/pentaxdrivemode "$tgt" || { record T7 FAIL "write $tgt"; return; }
    after=$(cond_get 'drive-mode-raw=[0-9]+')
    echo "[$(ts)] drive raw after write: ${after:-none}" >> "$LOG"
    write_with_retry /main/status/pentaxdrivemode "$orig" || { record T7 FAIL "restore"; return; }
    after=$(widget_current /main/status/pentaxdrivemode)
    [ "$after" = "$orig" ] && record T7 PASS "drive '$orig'->'$tgt'->'$orig'" \
                           || record T7 FAIL "restore verify: '$after'"
}

summary() {
    echo "=== Final conditions ===" >> "$LOG"
    gp --get-config /main/status/pentaxconditions >> "$LOG" 2>&1
    {
        echo ""; echo "================ SUMMARY ($(ts)) ================"
        printf '%s\n' "${RESULTS[@]}"
        echo "PASS=$PASS FAIL=$FAIL"
    } >> "$LOG"
    echo "================ SUMMARY ================"
    printf '%s\n' "${RESULTS[@]}"
    echo "PASS=$PASS FAIL=$FAIL"
}

trap 'echo "Session log: $LOG"' EXIT

mkdir -p "$(dirname "$LOG")"
{
    echo ""
    echo "============================================================"
    echo "K-3 III test session started: $(ts)  Port: $PORT"
    echo "============================================================"
} >> "$LOG"
preflight
T1_inventory
T2_iso
T3_aperture
T4_ev
T5_shutter
T6_wb
T7_drive
summary
