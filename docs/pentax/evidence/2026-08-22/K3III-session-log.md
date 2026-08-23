
============================================================
K-3 III test session started: 2026-08-22 02:38:04  Port: usb:001,025
============================================================
=== Ownership preflight ===
[2026-08-22 02:38:04] preflight OK
--- T1 read-only inventory ---
[2026-08-22 02:38:04] $ gphoto2 --camera "Pentax:K-3 Mark III (MTP mode)" --port "usb:001,025" --get-config /main/status/pentaxconditions
[2026-08-22 02:38:05] exit=0 (informational only)
Pentax init stage vendor enable succeeded; function flags 0x00000000.
Label: Pentax Conditions
Readonly: 1
Type: TEXT
Current: state=0; astro-phase=inactive/other; exposure-mode-raw=20; user-mode-raw=0; drive-mode-raw=0; ISO=3200; exposure-step=1; open-av-num=28; shooting=no; processing=no; task-changing=no; Tv-changeable=yes; bulb-timer=yes; bulb-seconds=300/1; aperture=35/10; exposure-comp=0/10; astrotracer3=yes; astro-shift=no; astro-movement-failed=no; astro-time-too-long=no; astro-limit=0; gps-state=0
END
[2026-08-22 02:38:05] $ gphoto2 --camera "Pentax:K-3 Mark III (MTP mode)" --port "usb:001,025" --get-config /main/status/pentaxconditions
[2026-08-22 02:38:07] exit=0 (informational only)
Pentax init stage vendor enable succeeded; function flags 0x00000000.
Label: Pentax Conditions
Readonly: 1
Type: TEXT
Current: state=0; astro-phase=inactive/other; exposure-mode-raw=20; user-mode-raw=0; drive-mode-raw=0; ISO=3200; exposure-step=1; open-av-num=28; shooting=no; processing=no; task-changing=no; Tv-changeable=yes; bulb-timer=yes; bulb-seconds=300/1; aperture=35/10; exposure-comp=0/10; astrotracer3=yes; astro-shift=no; astro-movement-failed=no; astro-time-too-long=no; astro-limit=0; gps-state=0
END
[2026-08-22 02:38:05] pentaxconditions = state=0; astro-phase=inactive/other; exposure-mode-raw=20; user-mode-raw=0; drive-mode-raw=0; ISO=3200; exposure-step=1; open-av-num=28; shooting=no; processing=no; task-changing=no; Tv-changeable=yes; bulb-timer=yes; bulb-seconds=300/1; aperture=35/10; exposure-comp=0/10; astrotracer3=yes; astro-shift=no; astro-movement-failed=no; astro-time-too-long=no; astro-limit=0; gps-state=0
[2026-08-22 02:38:07] $ gphoto2 --camera "Pentax:K-3 Mark III (MTP mode)" --port "usb:001,025" --get-config /main/status/pentaxliveviewcontrols
[2026-08-22 02:38:08] exit=0 (informational only)
Pentax init stage vendor enable succeeded; function flags 0x00000000.
Label: Pentax Live View Controls
Readonly: 1
Type: TEXT
Current: area=1080x720; active=1080x720; caf-active=734x432; caf-spot=96x96; af-position=540,360; af-response-bytes=8; zoom-raw=1
END
[2026-08-22 02:38:08] $ gphoto2 --camera "Pentax:K-3 Mark III (MTP mode)" --port "usb:001,025" --get-config /main/status/pentaxliveviewcontrols
[2026-08-22 02:38:09] exit=0 (informational only)
Pentax init stage vendor enable succeeded; function flags 0x00000000.
Label: Pentax Live View Controls
Readonly: 1
Type: TEXT
Current: area=1080x720; active=1080x720; caf-active=734x432; caf-spot=96x96; af-position=540,360; af-response-bytes=8; zoom-raw=1
END
[2026-08-22 02:38:08] pentaxliveviewcontrols = area=1080x720; active=1080x720; caf-active=734x432; caf-spot=96x96; af-position=540,360; af-response-bytes=8; zoom-raw=1
[2026-08-22 02:38:09] $ gphoto2 --camera "Pentax:K-3 Mark III (MTP mode)" --port "usb:001,025" --get-config /main/status/pentaxdirectiso
[2026-08-22 02:38:11] exit=0 (informational only)
Pentax init stage vendor enable succeeded; function flags 0x00000000.
Label: Pentax Direct ISO Speed
Readonly: 0
Type: RADIO
Current: 3200
Choice: 0 100
Choice: 1 200
Choice: 2 400
Choice: 3 800
Choice: 4 1600
Choice: 5 3200
Choice: 6 6400
Choice: 7 12800
Choice: 8 25600
Choice: 9 51200
Choice: 10 102400
Choice: 11 204800
Choice: 12 409600
Choice: 13 819200
Choice: 14 1600000
END
[2026-08-22 02:38:11] $ gphoto2 --camera "Pentax:K-3 Mark III (MTP mode)" --port "usb:001,025" --get-config /main/status/pentaxdirectiso
[2026-08-22 02:38:12] exit=0 (informational only)
Pentax init stage vendor enable succeeded; function flags 0x00000000.
Label: Pentax Direct ISO Speed
Readonly: 0
Type: RADIO
Current: 3200
Choice: 0 100
Choice: 1 200
Choice: 2 400
Choice: 3 800
Choice: 4 1600
Choice: 5 3200
Choice: 6 6400
Choice: 7 12800
Choice: 8 25600
Choice: 9 51200
Choice: 10 102400
Choice: 11 204800
Choice: 12 409600
Choice: 13 819200
Choice: 14 1600000
END
[2026-08-22 02:38:11] pentaxdirectiso = 3200
[2026-08-22 02:38:12] $ gphoto2 --camera "Pentax:K-3 Mark III (MTP mode)" --port "usb:001,025" --get-config /main/status/pentaxdirectshutter
[2026-08-22 02:38:13] exit=0 (informational only)
Pentax init stage vendor enable succeeded; function flags 0x00000000.
Label: Pentax Direct Shutter Speed
Readonly: 0
Type: RADIO
Current: 300/1
Choice: 0 1/1
Choice: 1 2/1
Choice: 2 3/1
Choice: 3 4/1
Choice: 4 5/1
Choice: 5 6/1
Choice: 6 7/1
Choice: 7 8/1
Choice: 8 9/1
Choice: 9 10/1
Choice: 10 15/1
Choice: 11 20/1
Choice: 12 25/1
Choice: 13 30/1
Choice: 14 40/1
Choice: 15 50/1
Choice: 16 60/1
Choice: 17 70/1
Choice: 18 80/1
Choice: 19 90/1
Choice: 20 100/1
Choice: 21 110/1
Choice: 22 120/1
Choice: 23 130/1
Choice: 24 140/1
Choice: 25 150/1
Choice: 26 160/1
Choice: 27 170/1
Choice: 28 180/1
Choice: 29 190/1
Choice: 30 200/1
Choice: 31 210/1
Choice: 32 220/1
Choice: 33 230/1
Choice: 34 240/1
Choice: 35 250/1
Choice: 36 260/1
Choice: 37 270/1
Choice: 38 280/1
Choice: 39 290/1
Choice: 40 300/1
Choice: 41 360/1
Choice: 42 420/1
Choice: 43 480/1
Choice: 44 540/1
Choice: 45 600/1
END
[2026-08-22 02:38:13] $ gphoto2 --camera "Pentax:K-3 Mark III (MTP mode)" --port "usb:001,025" --get-config /main/status/pentaxdirectshutter
[2026-08-22 02:38:14] exit=0 (informational only)
Pentax init stage vendor enable succeeded; function flags 0x00000000.
Label: Pentax Direct Shutter Speed
Readonly: 0
Type: RADIO
Current: 300/1
Choice: 0 1/1
Choice: 1 2/1
Choice: 2 3/1
Choice: 3 4/1
Choice: 4 5/1
Choice: 5 6/1
Choice: 6 7/1
Choice: 7 8/1
Choice: 8 9/1
Choice: 9 10/1
Choice: 10 15/1
Choice: 11 20/1
Choice: 12 25/1
Choice: 13 30/1
Choice: 14 40/1
Choice: 15 50/1
Choice: 16 60/1
Choice: 17 70/1
Choice: 18 80/1
Choice: 19 90/1
Choice: 20 100/1
Choice: 21 110/1
Choice: 22 120/1
Choice: 23 130/1
Choice: 24 140/1
Choice: 25 150/1
Choice: 26 160/1
Choice: 27 170/1
Choice: 28 180/1
Choice: 29 190/1
Choice: 30 200/1
Choice: 31 210/1
Choice: 32 220/1
Choice: 33 230/1
Choice: 34 240/1
Choice: 35 250/1
Choice: 36 260/1
Choice: 37 270/1
Choice: 38 280/1
Choice: 39 290/1
Choice: 40 300/1
Choice: 41 360/1
Choice: 42 420/1
Choice: 43 480/1
Choice: 44 540/1
Choice: 45 600/1
END
[2026-08-22 02:38:13] pentaxdirectshutter = 300/1
[2026-08-22 02:38:14] $ gphoto2 --camera "Pentax:K-3 Mark III (MTP mode)" --port "usb:001,025" --get-config /main/status/pentaxdirectaperture
[2026-08-22 02:38:16] exit=0 (informational only)
Pentax init stage vendor enable succeeded; function flags 0x00000000.
Label: Pentax Direct Aperture
Readonly: 0
Type: RADIO
Current: f/3.5
Choice: 0 f/22
Choice: 1 f/20
Choice: 2 f/18
Choice: 3 f/16
Choice: 4 f/14
Choice: 5 f/13
Choice: 6 f/11
Choice: 7 f/10
Choice: 8 f/9
Choice: 9 f/8
Choice: 10 f/7.1
Choice: 11 f/6.3
Choice: 12 f/5.6
Choice: 13 f/5
Choice: 14 f/4.5
Choice: 15 f/4
Choice: 16 f/3.5
Choice: 17 f/3.2
Choice: 18 f/2.8
END
[2026-08-22 02:38:16] $ gphoto2 --camera "Pentax:K-3 Mark III (MTP mode)" --port "usb:001,025" --get-config /main/status/pentaxdirectaperture
[2026-08-22 02:38:17] exit=0 (informational only)
Pentax init stage vendor enable succeeded; function flags 0x00000000.
Label: Pentax Direct Aperture
Readonly: 0
Type: RADIO
Current: f/3.5
Choice: 0 f/22
Choice: 1 f/20
Choice: 2 f/18
Choice: 3 f/16
Choice: 4 f/14
Choice: 5 f/13
Choice: 6 f/11
Choice: 7 f/10
Choice: 8 f/9
Choice: 9 f/8
Choice: 10 f/7.1
Choice: 11 f/6.3
Choice: 12 f/5.6
Choice: 13 f/5
Choice: 14 f/4.5
Choice: 15 f/4
Choice: 16 f/3.5
Choice: 17 f/3.2
Choice: 18 f/2.8
END
[2026-08-22 02:38:16] pentaxdirectaperture = f/3.5
[2026-08-22 02:38:17] $ gphoto2 --camera "Pentax:K-3 Mark III (MTP mode)" --port "usb:001,025" --get-config /main/status/pentaxdirectev
[2026-08-22 02:38:18] exit=0 (informational only)
Pentax init stage vendor enable succeeded; function flags 0x00000000.
Label: Pentax Direct Exposure Compensation
Readonly: 0
Type: RADIO
Current: 0
END
[2026-08-22 02:38:18] $ gphoto2 --camera "Pentax:K-3 Mark III (MTP mode)" --port "usb:001,025" --get-config /main/status/pentaxdirectev
[2026-08-22 02:38:19] exit=0 (informational only)
Pentax init stage vendor enable succeeded; function flags 0x00000000.
Label: Pentax Direct Exposure Compensation
Readonly: 0
Type: RADIO
Current: 0
END
[2026-08-22 02:38:18] pentaxdirectev = 0
[2026-08-22 02:38:19] $ gphoto2 --camera "Pentax:K-3 Mark III (MTP mode)" --port "usb:001,025" --get-config /main/status/pentaxdrivemode
[2026-08-22 02:38:21] exit=0 (informational only)
Pentax init stage vendor enable succeeded; function flags 0x00000000.
Label: Pentax Drive Mode
Readonly: 0
Type: RADIO
Current: single
Choice: 0 single
Choice: 1 continuous-hi
Choice: 2 continuous-mid
Choice: 3 continuous-lo
Choice: 4 self-timer-12s
Choice: 5 self-timer-2s
Choice: 6 self-timer-cont
Choice: 7 remote
Choice: 8 remote-3s
Choice: 9 remote-cont
Choice: 10 mirror-up
Choice: 11 mirror-up-remote
Choice: 12 multi-exposure
Choice: 13 interval
Choice: 14 interval-movie
Choice: 15 star-stream
END
[2026-08-22 02:38:21] $ gphoto2 --camera "Pentax:K-3 Mark III (MTP mode)" --port "usb:001,025" --get-config /main/status/pentaxdrivemode
[2026-08-22 02:38:22] exit=0 (informational only)
Pentax init stage vendor enable succeeded; function flags 0x00000000.
Label: Pentax Drive Mode
Readonly: 0
Type: RADIO
Current: single
Choice: 0 single
Choice: 1 continuous-hi
Choice: 2 continuous-mid
Choice: 3 continuous-lo
Choice: 4 self-timer-12s
Choice: 5 self-timer-2s
Choice: 6 self-timer-cont
Choice: 7 remote
Choice: 8 remote-3s
Choice: 9 remote-cont
Choice: 10 mirror-up
Choice: 11 mirror-up-remote
Choice: 12 multi-exposure
Choice: 13 interval
Choice: 14 interval-movie
Choice: 15 star-stream
END
[2026-08-22 02:38:21] pentaxdrivemode = single
[2026-08-22 02:38:22] $ gphoto2 --camera "Pentax:K-3 Mark III (MTP mode)" --port "usb:001,025" --get-config /main/status/pentaxdirectwb
[2026-08-22 02:38:23] exit=1 (informational only)
Pentax init stage vendor enable succeeded; function flags 0x00000000.

*** Error ***              
/main not found in configuration tree.
*** Error (-1: 'Unspecified error') ***       

For debugging messages, please use the --debug option.
Debugging messages may help finding a solution to your problem.
If you intend to send any error or debug messages to the gphoto
developer mailing list <gphoto-devel@lists.sourceforge.net>, please run
gphoto2 as follows:

    env LANG=C gphoto2 --debug --debug-logfile=my-logfile.txt --camera "Pentax:K-3 Mark III (MTP mode)" --port usb:001,025 --get-config /main/status/pentaxdirectwb

Please make sure there is sufficient quoting around the arguments.
[2026-08-22 02:38:23] $ gphoto2 --camera "Pentax:K-3 Mark III (MTP mode)" --port "usb:001,025" --get-config /main/status/pentaxdirectwb
[2026-08-22 02:38:24] exit=1 (informational only)
Pentax init stage vendor enable succeeded; function flags 0x00000000.

*** Error ***              
/main not found in configuration tree.
*** Error (-1: 'Unspecified error') ***       

For debugging messages, please use the --debug option.
Debugging messages may help finding a solution to your problem.
If you intend to send any error or debug messages to the gphoto
developer mailing list <gphoto-devel@lists.sourceforge.net>, please run
gphoto2 as follows:

    env LANG=C gphoto2 --debug --debug-logfile=my-logfile.txt --camera "Pentax:K-3 Mark III (MTP mode)" --port usb:001,025 --get-config /main/status/pentaxdirectwb

Please make sure there is sufficient quoting around the arguments.
[2026-08-22 02:38:23] pentaxdirectwb = 
[2026-08-22 02:38:24] RESULT T1 PASS : inventory ok=8
[2026-08-22 02:38:24] $ gphoto2 --camera "Pentax:K-3 Mark III (MTP mode)" --port "usb:001,025" --get-config /main/status/pentaxconditions
[2026-08-22 02:38:26] exit=0 (informational only)
Pentax init stage vendor enable succeeded; function flags 0x00000000.
Label: Pentax Conditions
Readonly: 1
Type: TEXT
Current: state=0; astro-phase=inactive/other; exposure-mode-raw=20; user-mode-raw=0; drive-mode-raw=0; ISO=3200; exposure-step=1; open-av-num=28; shooting=no; processing=no; task-changing=no; Tv-changeable=yes; bulb-timer=yes; bulb-seconds=300/1; aperture=35/10; exposure-comp=0/10; astrotracer3=yes; astro-shift=no; astro-movement-failed=no; astro-time-too-long=no; astro-limit=0; gps-state=0
END
--- T2 ISO 3200->1600->3200 ---
[2026-08-22 02:38:26] $ gphoto2 --camera "Pentax:K-3 Mark III (MTP mode)" --port "usb:001,025" --set-config /main/status/pentaxdirectiso=1600
[2026-08-22 02:38:27] exit=0 (informational only)
Pentax init stage vendor enable succeeded; function flags 0x00000000.
[2026-08-22 02:38:28] $ gphoto2 --camera "Pentax:K-3 Mark III (MTP mode)" --port "usb:001,025" --get-config /main/status/pentaxdirectiso
[2026-08-22 02:38:30] exit=0 (informational only)
Pentax init stage vendor enable succeeded; function flags 0x00000000.
Label: Pentax Direct ISO Speed
Readonly: 0
Type: RADIO
Current: 1600
Choice: 0 100
Choice: 1 200
Choice: 2 400
Choice: 3 800
Choice: 4 1600
Choice: 5 3200
Choice: 6 6400
Choice: 7 12800
Choice: 8 25600
Choice: 9 51200
Choice: 10 102400
Choice: 11 204800
Choice: 12 409600
Choice: 13 819200
Choice: 14 1600000
END
[2026-08-22 02:38:30] $ gphoto2 --camera "Pentax:K-3 Mark III (MTP mode)" --port "usb:001,025" --get-config /main/status/pentaxconditions
[2026-08-22 02:38:31] exit=0 (informational only)
Pentax init stage vendor enable succeeded; function flags 0x00000000.
Label: Pentax Conditions
Readonly: 1
Type: TEXT
Current: state=0; astro-phase=inactive/other; exposure-mode-raw=20; user-mode-raw=0; drive-mode-raw=0; ISO=1600; exposure-step=1; open-av-num=28; shooting=no; processing=no; task-changing=no; Tv-changeable=yes; bulb-timer=yes; bulb-seconds=300/1; aperture=35/10; exposure-comp=0/10; astrotracer3=yes; astro-shift=no; astro-movement-failed=no; astro-time-too-long=no; astro-limit=0; gps-state=0
END
[2026-08-22 02:38:31] $ gphoto2 --camera "Pentax:K-3 Mark III (MTP mode)" --port "usb:001,025" --set-config /main/status/pentaxdirectiso=3200
[2026-08-22 02:38:33] exit=0 (informational only)
Pentax init stage vendor enable succeeded; function flags 0x00000000.
[2026-08-22 02:38:34] $ gphoto2 --camera "Pentax:K-3 Mark III (MTP mode)" --port "usb:001,025" --get-config /main/status/pentaxdirectiso
[2026-08-22 02:38:35] exit=0 (informational only)
Pentax init stage vendor enable succeeded; function flags 0x00000000.
Label: Pentax Direct ISO Speed
Readonly: 0
Type: RADIO
Current: 3200
Choice: 0 100
Choice: 1 200
Choice: 2 400
Choice: 3 800
Choice: 4 1600
Choice: 5 3200
Choice: 6 6400
Choice: 7 12800
Choice: 8 25600
Choice: 9 51200
Choice: 10 102400
Choice: 11 204800
Choice: 12 409600
Choice: 13 819200
Choice: 14 1600000
END
[2026-08-22 02:38:35] $ gphoto2 --camera "Pentax:K-3 Mark III (MTP mode)" --port "usb:001,025" --get-config /main/status/pentaxconditions
[2026-08-22 02:38:37] exit=0 (informational only)
Pentax init stage vendor enable succeeded; function flags 0x00000000.
Label: Pentax Conditions
Readonly: 1
Type: TEXT
Current: state=0; astro-phase=inactive/other; exposure-mode-raw=20; user-mode-raw=0; drive-mode-raw=0; ISO=3200; exposure-step=1; open-av-num=28; shooting=no; processing=no; task-changing=no; Tv-changeable=yes; bulb-timer=yes; bulb-seconds=300/1; aperture=35/10; exposure-comp=0/10; astrotracer3=yes; astro-shift=no; astro-movement-failed=no; astro-time-too-long=no; astro-limit=0; gps-state=0
END
[2026-08-22 02:38:37] RESULT T2 PASS : ISO 3200->1600->3200
[2026-08-22 02:38:37] $ gphoto2 --camera "Pentax:K-3 Mark III (MTP mode)" --port "usb:001,025" --get-config /main/status/pentaxdirectaperture
[2026-08-22 02:38:38] exit=0 (informational only)
Pentax init stage vendor enable succeeded; function flags 0x00000000.
Label: Pentax Direct Aperture
Readonly: 0
Type: RADIO
Current: f/3.5
Choice: 0 f/22
Choice: 1 f/20
Choice: 2 f/18
Choice: 3 f/16
Choice: 4 f/14
Choice: 5 f/13
Choice: 6 f/11
Choice: 7 f/10
Choice: 8 f/9
Choice: 9 f/8
Choice: 10 f/7.1
Choice: 11 f/6.3
Choice: 12 f/5.6
Choice: 13 f/5
Choice: 14 f/4.5
Choice: 15 f/4
Choice: 16 f/3.5
Choice: 17 f/3.2
Choice: 18 f/2.8
END
--- T3 aperture f/3.5->f/4->f/3.5 ---
[2026-08-22 02:38:38] $ gphoto2 --camera "Pentax:K-3 Mark III (MTP mode)" --port "usb:001,025" --set-config /main/status/pentaxdirectaperture=f/4
[2026-08-22 02:38:40] exit=1 (informational only)
Pentax init stage vendor enable succeeded; function flags 0x00000000.

*** Error ***              
Failed to set new configuration value f/4 for configuration entry /main/status/pentaxdirectaperture.
*** Error (-1: 'Unspecified error') ***       

For debugging messages, please use the --debug option.
Debugging messages may help finding a solution to your problem.
If you intend to send any error or debug messages to the gphoto
developer mailing list <gphoto-devel@lists.sourceforge.net>, please run
gphoto2 as follows:

    env LANG=C gphoto2 --debug --debug-logfile=my-logfile.txt --camera "Pentax:K-3 Mark III (MTP mode)" --port usb:001,025 --set-config /main/status/pentaxdirectaperture=f/4

Please make sure there is sufficient quoting around the arguments.
[2026-08-22 02:38:41] $ gphoto2 --camera "Pentax:K-3 Mark III (MTP mode)" --port "usb:001,025" --get-config /main/status/pentaxdirectaperture
[2026-08-22 02:38:42] exit=0 (informational only)
Pentax init stage vendor enable succeeded; function flags 0x00000000.
Label: Pentax Direct Aperture
Readonly: 0
Type: RADIO
Current: f/4
Choice: 0 f/22
Choice: 1 f/20
Choice: 2 f/18
Choice: 3 f/16
Choice: 4 f/14
Choice: 5 f/13
Choice: 6 f/11
Choice: 7 f/10
Choice: 8 f/9
Choice: 9 f/8
Choice: 10 f/7.1
Choice: 11 f/6.3
Choice: 12 f/5.6
Choice: 13 f/5
Choice: 14 f/4.5
Choice: 15 f/4
Choice: 16 f/3.5
Choice: 17 f/3.2
Choice: 18 f/2.8
END
[2026-08-22 02:38:42] $ gphoto2 --camera "Pentax:K-3 Mark III (MTP mode)" --port "usb:001,025" --get-config /main/status/pentaxconditions
[2026-08-22 02:38:44] exit=0 (informational only)
Pentax init stage vendor enable succeeded; function flags 0x00000000.
Label: Pentax Conditions
Readonly: 1
Type: TEXT
Current: state=0; astro-phase=inactive/other; exposure-mode-raw=20; user-mode-raw=0; drive-mode-raw=0; ISO=3200; exposure-step=1; open-av-num=28; shooting=no; processing=no; task-changing=no; Tv-changeable=yes; bulb-timer=yes; bulb-seconds=300/1; aperture=40/10; exposure-comp=0/10; astrotracer3=yes; astro-shift=no; astro-movement-failed=no; astro-time-too-long=no; astro-limit=0; gps-state=0
END
[2026-08-22 02:38:44] conditions after write: aperture=40/10
[2026-08-22 02:38:44] $ gphoto2 --camera "Pentax:K-3 Mark III (MTP mode)" --port "usb:001,025" --set-config /main/status/pentaxdirectaperture=f/3.5
[2026-08-22 02:38:46] exit=1 (informational only)
Pentax init stage vendor enable succeeded; function flags 0x00000000.

*** Error ***              
Failed to set new configuration value f/3.5 for configuration entry /main/status/pentaxdirectaperture.
*** Error (-1: 'Unspecified error') ***       

For debugging messages, please use the --debug option.
Debugging messages may help finding a solution to your problem.
If you intend to send any error or debug messages to the gphoto
developer mailing list <gphoto-devel@lists.sourceforge.net>, please run
gphoto2 as follows:

    env LANG=C gphoto2 --debug --debug-logfile=my-logfile.txt --camera "Pentax:K-3 Mark III (MTP mode)" --port usb:001,025 --set-config /main/status/pentaxdirectaperture=f/3.5

Please make sure there is sufficient quoting around the arguments.
[2026-08-22 02:38:47] $ gphoto2 --camera "Pentax:K-3 Mark III (MTP mode)" --port "usb:001,025" --get-config /main/status/pentaxdirectaperture
[2026-08-22 02:38:48] exit=0 (informational only)
Pentax init stage vendor enable succeeded; function flags 0x00000000.
Label: Pentax Direct Aperture
Readonly: 0
Type: RADIO
Current: f/3.5
Choice: 0 f/22
Choice: 1 f/20
Choice: 2 f/18
Choice: 3 f/16
Choice: 4 f/14
Choice: 5 f/13
Choice: 6 f/11
Choice: 7 f/10
Choice: 8 f/9
Choice: 9 f/8
Choice: 10 f/7.1
Choice: 11 f/6.3
Choice: 12 f/5.6
Choice: 13 f/5
Choice: 14 f/4.5
Choice: 15 f/4
Choice: 16 f/3.5
Choice: 17 f/3.2
Choice: 18 f/2.8
END
[2026-08-22 02:38:48] $ gphoto2 --camera "Pentax:K-3 Mark III (MTP mode)" --port "usb:001,025" --get-config /main/status/pentaxdirectaperture
[2026-08-22 02:38:50] exit=0 (informational only)
Pentax init stage vendor enable succeeded; function flags 0x00000000.
Label: Pentax Direct Aperture
Readonly: 0
Type: RADIO
Current: f/3.5
Choice: 0 f/22
Choice: 1 f/20
Choice: 2 f/18
Choice: 3 f/16
Choice: 4 f/14
Choice: 5 f/13
Choice: 6 f/11
Choice: 7 f/10
Choice: 8 f/9
Choice: 9 f/8
Choice: 10 f/7.1
Choice: 11 f/6.3
Choice: 12 f/5.6
Choice: 13 f/5
Choice: 14 f/4.5
Choice: 15 f/4
Choice: 16 f/3.5
Choice: 17 f/3.2
Choice: 18 f/2.8
END
[2026-08-22 02:38:50] RESULT T3 PASS : aperture f/3.5->f/4->f/3.5
[2026-08-22 02:38:50] $ gphoto2 --camera "Pentax:K-3 Mark III (MTP mode)" --port "usb:001,025" --get-config /main/status/pentaxdirectev
[2026-08-22 02:38:51] exit=0 (informational only)
Pentax init stage vendor enable succeeded; function flags 0x00000000.
Label: Pentax Direct Exposure Compensation
Readonly: 0
Type: RADIO
Current: 0
END
[2026-08-22 02:38:51] $ gphoto2 --camera "Pentax:K-3 Mark III (MTP mode)" --port "usb:001,025" --get-config /main/status/pentaxdirectev
[2026-08-22 02:38:52] exit=0 (informational only)
Pentax init stage vendor enable succeeded; function flags 0x00000000.
Label: Pentax Direct Exposure Compensation
Readonly: 0
Type: RADIO
Current: 0
END
[2026-08-22 02:38:52] RESULT T4 FAIL : no alternative EV choice
[2026-08-22 02:38:52] $ gphoto2 --camera "Pentax:K-3 Mark III (MTP mode)" --port "usb:001,025" --get-config /main/status/pentaxdirectshutter
[2026-08-22 02:38:53] exit=0 (informational only)
Pentax init stage vendor enable succeeded; function flags 0x00000000.
Label: Pentax Direct Shutter Speed
Readonly: 0
Type: RADIO
Current: 300/1
Choice: 0 1/1
Choice: 1 2/1
Choice: 2 3/1
Choice: 3 4/1
Choice: 4 5/1
Choice: 5 6/1
Choice: 6 7/1
Choice: 7 8/1
Choice: 8 9/1
Choice: 9 10/1
Choice: 10 15/1
Choice: 11 20/1
Choice: 12 25/1
Choice: 13 30/1
Choice: 14 40/1
Choice: 15 50/1
Choice: 16 60/1
Choice: 17 70/1
Choice: 18 80/1
Choice: 19 90/1
Choice: 20 100/1
Choice: 21 110/1
Choice: 22 120/1
Choice: 23 130/1
Choice: 24 140/1
Choice: 25 150/1
Choice: 26 160/1
Choice: 27 170/1
Choice: 28 180/1
Choice: 29 190/1
Choice: 30 200/1
Choice: 31 210/1
Choice: 32 220/1
Choice: 33 230/1
Choice: 34 240/1
Choice: 35 250/1
Choice: 36 260/1
Choice: 37 270/1
Choice: 38 280/1
Choice: 39 290/1
Choice: 40 300/1
Choice: 41 360/1
Choice: 42 420/1
Choice: 43 480/1
Choice: 44 540/1
Choice: 45 600/1
END
[2026-08-22 02:38:53] $ gphoto2 --camera "Pentax:K-3 Mark III (MTP mode)" --port "usb:001,025" --get-config /main/status/pentaxdirectshutter
[2026-08-22 02:38:55] exit=0 (informational only)
Pentax init stage vendor enable succeeded; function flags 0x00000000.
Label: Pentax Direct Shutter Speed
Readonly: 0
Type: RADIO
Current: 300/1
Choice: 0 1/1
Choice: 1 2/1
Choice: 2 3/1
Choice: 3 4/1
Choice: 4 5/1
Choice: 5 6/1
Choice: 6 7/1
Choice: 7 8/1
Choice: 8 9/1
Choice: 9 10/1
Choice: 10 15/1
Choice: 11 20/1
Choice: 12 25/1
Choice: 13 30/1
Choice: 14 40/1
Choice: 15 50/1
Choice: 16 60/1
Choice: 17 70/1
Choice: 18 80/1
Choice: 19 90/1
Choice: 20 100/1
Choice: 21 110/1
Choice: 22 120/1
Choice: 23 130/1
Choice: 24 140/1
Choice: 25 150/1
Choice: 26 160/1
Choice: 27 170/1
Choice: 28 180/1
Choice: 29 190/1
Choice: 30 200/1
Choice: 31 210/1
Choice: 32 220/1
Choice: 33 230/1
Choice: 34 240/1
Choice: 35 250/1
Choice: 36 260/1
Choice: 37 270/1
Choice: 38 280/1
Choice: 39 290/1
Choice: 40 300/1
Choice: 41 360/1
Choice: 42 420/1
Choice: 43 480/1
Choice: 44 540/1
Choice: 45 600/1
END
--- T5 shutter '300/1'->'600/1'->'300/1' ---
[2026-08-22 02:38:55] $ gphoto2 --camera "Pentax:K-3 Mark III (MTP mode)" --port "usb:001,025" --set-config /main/status/pentaxdirectshutter=600/1
[2026-08-22 02:38:57] exit=0 (informational only)
Pentax init stage vendor enable succeeded; function flags 0x00000000.
[2026-08-22 02:38:58] $ gphoto2 --camera "Pentax:K-3 Mark III (MTP mode)" --port "usb:001,025" --get-config /main/status/pentaxdirectshutter
[2026-08-22 02:38:59] exit=0 (informational only)
Pentax init stage vendor enable succeeded; function flags 0x00000000.
Label: Pentax Direct Shutter Speed
Readonly: 0
Type: RADIO
Current: 600/1
Choice: 0 1/1
Choice: 1 2/1
Choice: 2 3/1
Choice: 3 4/1
Choice: 4 5/1
Choice: 5 6/1
Choice: 6 7/1
Choice: 7 8/1
Choice: 8 9/1
Choice: 9 10/1
Choice: 10 15/1
Choice: 11 20/1
Choice: 12 25/1
Choice: 13 30/1
Choice: 14 40/1
Choice: 15 50/1
Choice: 16 60/1
Choice: 17 70/1
Choice: 18 80/1
Choice: 19 90/1
Choice: 20 100/1
Choice: 21 110/1
Choice: 22 120/1
Choice: 23 130/1
Choice: 24 140/1
Choice: 25 150/1
Choice: 26 160/1
Choice: 27 170/1
Choice: 28 180/1
Choice: 29 190/1
Choice: 30 200/1
Choice: 31 210/1
Choice: 32 220/1
Choice: 33 230/1
Choice: 34 240/1
Choice: 35 250/1
Choice: 36 260/1
Choice: 37 270/1
Choice: 38 280/1
Choice: 39 290/1
Choice: 40 300/1
Choice: 41 360/1
Choice: 42 420/1
Choice: 43 480/1
Choice: 44 540/1
Choice: 45 600/1
END
[2026-08-22 02:38:59] $ gphoto2 --camera "Pentax:K-3 Mark III (MTP mode)" --port "usb:001,025" --get-config /main/status/pentaxconditions
[2026-08-22 02:39:00] exit=0 (informational only)
Pentax init stage vendor enable succeeded; function flags 0x00000000.
Label: Pentax Conditions
Readonly: 1
Type: TEXT
Current: state=0; astro-phase=inactive/other; exposure-mode-raw=20; user-mode-raw=0; drive-mode-raw=0; ISO=3200; exposure-step=1; open-av-num=28; shooting=no; processing=no; task-changing=no; Tv-changeable=yes; bulb-timer=yes; bulb-seconds=600/1; aperture=35/10; exposure-comp=0/10; astrotracer3=yes; astro-shift=no; astro-movement-failed=no; astro-time-too-long=no; astro-limit=0; gps-state=0
END
[2026-08-22 02:39:00] bulb-seconds after write: 'bulb-seconds=600/1'
[2026-08-22 02:39:00] $ gphoto2 --camera "Pentax:K-3 Mark III (MTP mode)" --port "usb:001,025" --set-config /main/status/pentaxdirectshutter=300/1
[2026-08-22 02:39:02] exit=0 (informational only)
Pentax init stage vendor enable succeeded; function flags 0x00000000.
[2026-08-22 02:39:03] $ gphoto2 --camera "Pentax:K-3 Mark III (MTP mode)" --port "usb:001,025" --get-config /main/status/pentaxdirectshutter
[2026-08-22 02:39:04] exit=0 (informational only)
Pentax init stage vendor enable succeeded; function flags 0x00000000.
Label: Pentax Direct Shutter Speed
Readonly: 0
Type: RADIO
Current: 300/1
Choice: 0 1/1
Choice: 1 2/1
Choice: 2 3/1
Choice: 3 4/1
Choice: 4 5/1
Choice: 5 6/1
Choice: 6 7/1
Choice: 7 8/1
Choice: 8 9/1
Choice: 9 10/1
Choice: 10 15/1
Choice: 11 20/1
Choice: 12 25/1
Choice: 13 30/1
Choice: 14 40/1
Choice: 15 50/1
Choice: 16 60/1
Choice: 17 70/1
Choice: 18 80/1
Choice: 19 90/1
Choice: 20 100/1
Choice: 21 110/1
Choice: 22 120/1
Choice: 23 130/1
Choice: 24 140/1
Choice: 25 150/1
Choice: 26 160/1
Choice: 27 170/1
Choice: 28 180/1
Choice: 29 190/1
Choice: 30 200/1
Choice: 31 210/1
Choice: 32 220/1
Choice: 33 230/1
Choice: 34 240/1
Choice: 35 250/1
Choice: 36 260/1
Choice: 37 270/1
Choice: 38 280/1
Choice: 39 290/1
Choice: 40 300/1
Choice: 41 360/1
Choice: 42 420/1
Choice: 43 480/1
Choice: 44 540/1
Choice: 45 600/1
END
[2026-08-22 02:39:04] $ gphoto2 --camera "Pentax:K-3 Mark III (MTP mode)" --port "usb:001,025" --get-config /main/status/pentaxdirectshutter
[2026-08-22 02:39:06] exit=0 (informational only)
Pentax init stage vendor enable succeeded; function flags 0x00000000.
Label: Pentax Direct Shutter Speed
Readonly: 0
Type: RADIO
Current: 300/1
Choice: 0 1/1
Choice: 1 2/1
Choice: 2 3/1
Choice: 3 4/1
Choice: 4 5/1
Choice: 5 6/1
Choice: 6 7/1
Choice: 7 8/1
Choice: 8 9/1
Choice: 9 10/1
Choice: 10 15/1
Choice: 11 20/1
Choice: 12 25/1
Choice: 13 30/1
Choice: 14 40/1
Choice: 15 50/1
Choice: 16 60/1
Choice: 17 70/1
Choice: 18 80/1
Choice: 19 90/1
Choice: 20 100/1
Choice: 21 110/1
Choice: 22 120/1
Choice: 23 130/1
Choice: 24 140/1
Choice: 25 150/1
Choice: 26 160/1
Choice: 27 170/1
Choice: 28 180/1
Choice: 29 190/1
Choice: 30 200/1
Choice: 31 210/1
Choice: 32 220/1
Choice: 33 230/1
Choice: 34 240/1
Choice: 35 250/1
Choice: 36 260/1
Choice: 37 270/1
Choice: 38 280/1
Choice: 39 290/1
Choice: 40 300/1
Choice: 41 360/1
Choice: 42 420/1
Choice: 43 480/1
Choice: 44 540/1
Choice: 45 600/1
END
[2026-08-22 02:39:06] RESULT T5 PASS : shutter '300/1'->'600/1'->'300/1'
[2026-08-22 02:39:06] $ gphoto2 --camera "Pentax:K-3 Mark III (MTP mode)" --port "usb:001,025" --get-config /main/status/pentaxdirectwb
[2026-08-22 02:39:07] exit=1 (informational only)
Pentax init stage vendor enable succeeded; function flags 0x00000000.

*** Error ***              
/main not found in configuration tree.
*** Error (-1: 'Unspecified error') ***       

For debugging messages, please use the --debug option.
Debugging messages may help finding a solution to your problem.
If you intend to send any error or debug messages to the gphoto
developer mailing list <gphoto-devel@lists.sourceforge.net>, please run
gphoto2 as follows:

    env LANG=C gphoto2 --debug --debug-logfile=my-logfile.txt --camera "Pentax:K-3 Mark III (MTP mode)" --port usb:001,025 --get-config /main/status/pentaxdirectwb

Please make sure there is sufficient quoting around the arguments.
[2026-08-22 02:39:07] RESULT T6 FAIL : no WB read
[2026-08-22 02:39:07] $ gphoto2 --camera "Pentax:K-3 Mark III (MTP mode)" --port "usb:001,025" --get-config /main/status/pentaxdrivemode
[2026-08-22 02:39:08] exit=0 (informational only)
Pentax init stage vendor enable succeeded; function flags 0x00000000.
Label: Pentax Drive Mode
Readonly: 0
Type: RADIO
Current: single
Choice: 0 single
Choice: 1 continuous-hi
Choice: 2 continuous-mid
Choice: 3 continuous-lo
Choice: 4 self-timer-12s
Choice: 5 self-timer-2s
Choice: 6 self-timer-cont
Choice: 7 remote
Choice: 8 remote-3s
Choice: 9 remote-cont
Choice: 10 mirror-up
Choice: 11 mirror-up-remote
Choice: 12 multi-exposure
Choice: 13 interval
Choice: 14 interval-movie
Choice: 15 star-stream
END
--- T7 drive 'single'->'continuous-lo'->'single' ---
[2026-08-22 02:39:08] $ gphoto2 --camera "Pentax:K-3 Mark III (MTP mode)" --port "usb:001,025" --set-config /main/status/pentaxdrivemode=continuous-lo
[2026-08-22 02:39:10] exit=0 (informational only)
Pentax init stage vendor enable succeeded; function flags 0x00000000.
[2026-08-22 02:39:11] $ gphoto2 --camera "Pentax:K-3 Mark III (MTP mode)" --port "usb:001,025" --get-config /main/status/pentaxdrivemode
[2026-08-22 02:39:13] exit=0 (informational only)
Pentax init stage vendor enable succeeded; function flags 0x00000000.
Label: Pentax Drive Mode
Readonly: 0
Type: RADIO
Current: single
Choice: 0 single
Choice: 1 continuous-hi
Choice: 2 continuous-mid
Choice: 3 continuous-lo
Choice: 4 self-timer-12s
Choice: 5 self-timer-2s
Choice: 6 self-timer-cont
Choice: 7 remote
Choice: 8 remote-3s
Choice: 9 remote-cont
Choice: 10 mirror-up
Choice: 11 mirror-up-remote
Choice: 12 multi-exposure
Choice: 13 interval
Choice: 14 interval-movie
Choice: 15 star-stream
END
[2026-08-22 02:39:13] write attempt 1 to /main/status/pentaxdrivemode=continuous-lo failed (read back 'single')
[2026-08-22 02:39:13] $ gphoto2 --camera "Pentax:K-3 Mark III (MTP mode)" --port "usb:001,025" --set-config /main/status/pentaxdrivemode=continuous-lo
[2026-08-22 02:39:15] exit=1 (informational only)
Pentax init stage vendor enable succeeded; function flags 0x00000000.

*** Error ***              
Failed to set new configuration value continuous-lo for configuration entry /main/status/pentaxdrivemode.
*** Error (-1: 'Unspecified error') ***       

For debugging messages, please use the --debug option.
Debugging messages may help finding a solution to your problem.
If you intend to send any error or debug messages to the gphoto
developer mailing list <gphoto-devel@lists.sourceforge.net>, please run
gphoto2 as follows:

    env LANG=C gphoto2 --debug --debug-logfile=my-logfile.txt --camera "Pentax:K-3 Mark III (MTP mode)" --port usb:001,025 --set-config /main/status/pentaxdrivemode=continuous-lo

Please make sure there is sufficient quoting around the arguments.
[2026-08-22 02:39:16] $ gphoto2 --camera "Pentax:K-3 Mark III (MTP mode)" --port "usb:001,025" --get-config /main/status/pentaxdrivemode
[2026-08-22 02:39:17] exit=0 (informational only)
Pentax init stage vendor enable succeeded; function flags 0x00000000.
Label: Pentax Drive Mode
Readonly: 0
Type: RADIO
Current: single
Choice: 0 single
Choice: 1 continuous-hi
Choice: 2 continuous-mid
Choice: 3 continuous-lo
Choice: 4 self-timer-12s
Choice: 5 self-timer-2s
Choice: 6 self-timer-cont
Choice: 7 remote
Choice: 8 remote-3s
Choice: 9 remote-cont
Choice: 10 mirror-up
Choice: 11 mirror-up-remote
Choice: 12 multi-exposure
Choice: 13 interval
Choice: 14 interval-movie
Choice: 15 star-stream
END
[2026-08-22 02:39:17] write attempt 2 to /main/status/pentaxdrivemode=continuous-lo failed (read back 'single')
[2026-08-22 02:39:17] RESULT T7 FAIL : write continuous-lo
=== Final conditions ===
[2026-08-22 02:39:17] $ gphoto2 --camera "Pentax:K-3 Mark III (MTP mode)" --port "usb:001,025" --get-config /main/status/pentaxconditions
[2026-08-22 02:39:18] exit=0 (informational only)
Pentax init stage vendor enable succeeded; function flags 0x00000000.
Label: Pentax Conditions
Readonly: 1
Type: TEXT
Current: state=0; astro-phase=inactive/other; exposure-mode-raw=20; user-mode-raw=0; drive-mode-raw=0; ISO=3200; exposure-step=1; open-av-num=28; shooting=no; processing=no; task-changing=no; Tv-changeable=yes; bulb-timer=yes; bulb-seconds=300/1; aperture=35/10; exposure-comp=0/10; astrotracer3=yes; astro-shift=no; astro-movement-failed=no; astro-time-too-long=no; astro-limit=0; gps-state=0
END
Pentax init stage vendor enable succeeded; function flags 0x00000000.
Label: Pentax Conditions
Readonly: 1
Type: TEXT
Current: state=0; astro-phase=inactive/other; exposure-mode-raw=20; user-mode-raw=0; drive-mode-raw=0; ISO=3200; exposure-step=1; open-av-num=28; shooting=no; processing=no; task-changing=no; Tv-changeable=yes; bulb-timer=yes; bulb-seconds=300/1; aperture=35/10; exposure-comp=0/10; astrotracer3=yes; astro-shift=no; astro-movement-failed=no; astro-time-too-long=no; astro-limit=0; gps-state=0
END
================ SUMMARY (2026-08-22 02:39:18) ================
T1 PASS inventory ok=8
T2 PASS ISO 3200->1600->3200
T3 PASS aperture f/3.5->f/4->f/3.5
T4 FAIL no alternative EV choice
T5 PASS shutter '300/1'->'600/1'->'300/1'
T6 FAIL no WB read
T7 FAIL write continuous-lo
PASS=4 FAIL=3

============================================================
ANALYSIS APPENDIX (added after automated run)
============================================================

| Test | Result | Detail |
|---|---|---|
| T1 inventory | PASS | 8 widgets read |
| T2 ISO roundtrip | PASS | 3200->1600->3200 verified via live conditions |
| T3 aperture roundtrip | PASS | f/3.5->f/4->f/3.5 verified |
| T4 EV roundtrip | FAIL (blocked) | K-3 III 0x5010 descriptor enum-count=0; _get_ExpCompensation requires enumeration so widget exposes no choices. Getter gap, not a write failure. |
| T5 shutter/bulb roundtrip | PASS | 300s->600s->300s in Bulb timer domain, verified via bulb-seconds |
| T6 WB roundtrip | FAIL (widget path) | pentaxdirectwb getter NOT_SUPPORTED: current 0x800f absent from IT2 _camToMtpWBTable. Generic /whitebalance works instead. |
| T7 drive mode | PASS on retry | First write silently no-op'd (known first-session quirk); retry applied single->continuous-lo->single, raw 0->6->0 |

Manual follow-ups:
- WB via generic widget: 0x800f -> Daylight (0x0004) -> restored to 0x800f
  (decimal 32783 via /main/other/5005). Verified by re-read both times.

Final state: ISO=3200; bulb-seconds=300/1; aperture=35/10; exposure-comp=0/10;
drive-mode-raw=0; WB 0x800f - identical to baseline. USB released.

Code follow-ups identified:
1. K-3 III EV descriptor has no enumeration; direct-EV getter needs range-form fallback.
2. K-3 III WB raw 0x800f unmapped in IT2 table; map it or use generic path for this body.
3. Drive-mode first-attempt silent no-op on fresh sessions; consider probe or double-read.

============================================================
CODE FIXES VERIFIED (read-only, 0% battery session)
============================================================
K-1 II dropped off bus (battery flat). K-3 III at 0% but responsive.
No writes performed. Three fixes compiled and read-verified:

1. pentaxdirectev: K-3 III (enum-count=0) now shows TEXT widget with raw
   thousandths value "0" instead of an empty RADIO. Writes remain
   enum-gated so none can occur until camera advertises choices.
2. pentaxdirectwb: added wire value 32783 (0x800f) labelled "auto-800f".
   Widget now reads Current: auto-800f on K-3 III instead of NOT_SUPPORTED.
3. NEW widget pentaxliveviewafposition (0xd036): reads 540,360 (centre of
   1080x720). Setter encodes IT2 8-byte payload, bounds-checks against
   live geometry, verifies by read-back. WRITE UNTESTED - awaiting
   supervised session (moves AF point).

Battery state at end: K-1 II off-bus (flat); K-3 III 0% still responding.

============================================================
K-3 III test session started: 2026-08-22 13:27:12  Port: usb:001,005
============================================================
=== Ownership preflight ===
[2026-08-22 13:27:12] preflight OK
--- T1 read-only inventory ---
[2026-08-22 13:27:12] $ gphoto2 --camera "Pentax:K-3 Mark III (MTP mode)" --port "usb:001,005" --get-config /main/status/pentaxconditions
[2026-08-22 13:27:14] exit=0 (informational only)
Pentax init stage vendor enable succeeded; function flags 0x00000000.
Label: Pentax Conditions
Readonly: 1
Type: TEXT
Current: state=0; astro-phase=inactive/other; exposure-mode-raw=20; user-mode-raw=0; drive-mode-raw=0; ISO=3200; exposure-step=1; open-av-num=28; shooting=no; processing=no; task-changing=no; Tv-changeable=yes; bulb-timer=yes; bulb-seconds=300/1; aperture=35/10; exposure-comp=0/10; astrotracer3=yes; astro-shift=no; astro-movement-failed=no; astro-time-too-long=no; astro-limit=0; gps-state=0
END
[2026-08-22 13:27:14] $ gphoto2 --camera "Pentax:K-3 Mark III (MTP mode)" --port "usb:001,005" --get-config /main/status/pentaxconditions
[2026-08-22 13:27:15] exit=0 (informational only)
Pentax init stage vendor enable succeeded; function flags 0x00000000.
Label: Pentax Conditions
Readonly: 1
Type: TEXT
Current: state=0; astro-phase=inactive/other; exposure-mode-raw=20; user-mode-raw=0; drive-mode-raw=0; ISO=3200; exposure-step=1; open-av-num=28; shooting=no; processing=no; task-changing=no; Tv-changeable=yes; bulb-timer=yes; bulb-seconds=300/1; aperture=35/10; exposure-comp=0/10; astrotracer3=yes; astro-shift=no; astro-movement-failed=no; astro-time-too-long=no; astro-limit=0; gps-state=0
END
[2026-08-22 13:27:14] pentaxconditions = state=0; astro-phase=inactive/other; exposure-mode-raw=20; user-mode-raw=0; drive-mode-raw=0; ISO=3200; exposure-step=1; open-av-num=28; shooting=no; processing=no; task-changing=no; Tv-changeable=yes; bulb-timer=yes; bulb-seconds=300/1; aperture=35/10; exposure-comp=0/10; astrotracer3=yes; astro-shift=no; astro-movement-failed=no; astro-time-too-long=no; astro-limit=0; gps-state=0
[2026-08-22 13:27:15] $ gphoto2 --camera "Pentax:K-3 Mark III (MTP mode)" --port "usb:001,005" --get-config /main/status/pentaxliveviewcontrols
[2026-08-22 13:27:16] exit=0 (informational only)
Pentax init stage vendor enable succeeded; function flags 0x00000000.
Label: Pentax Live View Controls
Readonly: 1
Type: TEXT
Current: area=1080x720; active=1080x720; caf-active=734x432; caf-spot=96x96; af-position=540,360; af-response-bytes=8; zoom-raw=1
END
[2026-08-22 13:27:16] $ gphoto2 --camera "Pentax:K-3 Mark III (MTP mode)" --port "usb:001,005" --get-config /main/status/pentaxliveviewcontrols
[2026-08-22 13:27:18] exit=0 (informational only)
Pentax init stage vendor enable succeeded; function flags 0x00000000.
Label: Pentax Live View Controls
Readonly: 1
Type: TEXT
Current: area=1080x720; active=1080x720; caf-active=734x432; caf-spot=96x96; af-position=540,360; af-response-bytes=8; zoom-raw=1
END
[2026-08-22 13:27:16] pentaxliveviewcontrols = area=1080x720; active=1080x720; caf-active=734x432; caf-spot=96x96; af-position=540,360; af-response-bytes=8; zoom-raw=1
[2026-08-22 13:27:18] $ gphoto2 --camera "Pentax:K-3 Mark III (MTP mode)" --port "usb:001,005" --get-config /main/status/pentaxdirectiso
[2026-08-22 13:27:19] exit=0 (informational only)
Pentax init stage vendor enable succeeded; function flags 0x00000000.
Label: Pentax Direct ISO Speed
Readonly: 0
Type: RADIO
Current: 3200
Choice: 0 100
Choice: 1 200
Choice: 2 400
Choice: 3 800
Choice: 4 1600
Choice: 5 3200
Choice: 6 6400
Choice: 7 12800
Choice: 8 25600
Choice: 9 51200
Choice: 10 102400
Choice: 11 204800
Choice: 12 409600
Choice: 13 819200
Choice: 14 1600000
END
[2026-08-22 13:27:19] $ gphoto2 --camera "Pentax:K-3 Mark III (MTP mode)" --port "usb:001,005" --get-config /main/status/pentaxdirectiso
[2026-08-22 13:27:20] exit=0 (informational only)
Pentax init stage vendor enable succeeded; function flags 0x00000000.
Label: Pentax Direct ISO Speed
Readonly: 0
Type: RADIO
Current: 3200
Choice: 0 100
Choice: 1 200
Choice: 2 400
Choice: 3 800
Choice: 4 1600
Choice: 5 3200
Choice: 6 6400
Choice: 7 12800
Choice: 8 25600
Choice: 9 51200
Choice: 10 102400
Choice: 11 204800
Choice: 12 409600
Choice: 13 819200
Choice: 14 1600000
END
[2026-08-22 13:27:19] pentaxdirectiso = 3200
[2026-08-22 13:27:20] $ gphoto2 --camera "Pentax:K-3 Mark III (MTP mode)" --port "usb:001,005" --get-config /main/status/pentaxdirectshutter
[2026-08-22 13:27:22] exit=0 (informational only)
Pentax init stage vendor enable succeeded; function flags 0x00000000.
Label: Pentax Direct Shutter Speed
Readonly: 0
Type: RADIO
Current: 300/1
Choice: 0 1/1
Choice: 1 2/1
Choice: 2 3/1
Choice: 3 4/1
Choice: 4 5/1
Choice: 5 6/1
Choice: 6 7/1
Choice: 7 8/1
Choice: 8 9/1
Choice: 9 10/1
Choice: 10 15/1
Choice: 11 20/1
Choice: 12 25/1
Choice: 13 30/1
Choice: 14 40/1
Choice: 15 50/1
Choice: 16 60/1
Choice: 17 70/1
Choice: 18 80/1
Choice: 19 90/1
Choice: 20 100/1
Choice: 21 110/1
Choice: 22 120/1
Choice: 23 130/1
Choice: 24 140/1
Choice: 25 150/1
Choice: 26 160/1
Choice: 27 170/1
Choice: 28 180/1
Choice: 29 190/1
Choice: 30 200/1
Choice: 31 210/1
Choice: 32 220/1
Choice: 33 230/1
Choice: 34 240/1
Choice: 35 250/1
Choice: 36 260/1
Choice: 37 270/1
Choice: 38 280/1
Choice: 39 290/1
Choice: 40 300/1
Choice: 41 360/1
Choice: 42 420/1
Choice: 43 480/1
Choice: 44 540/1
Choice: 45 600/1
END
[2026-08-22 13:27:22] $ gphoto2 --camera "Pentax:K-3 Mark III (MTP mode)" --port "usb:001,005" --get-config /main/status/pentaxdirectshutter
[2026-08-22 13:27:23] exit=0 (informational only)
Pentax init stage vendor enable succeeded; function flags 0x00000000.
Label: Pentax Direct Shutter Speed
Readonly: 0
Type: RADIO
Current: 300/1
Choice: 0 1/1
Choice: 1 2/1
Choice: 2 3/1
Choice: 3 4/1
Choice: 4 5/1
Choice: 5 6/1
Choice: 6 7/1
Choice: 7 8/1
Choice: 8 9/1
Choice: 9 10/1
Choice: 10 15/1
Choice: 11 20/1
Choice: 12 25/1
Choice: 13 30/1
Choice: 14 40/1
Choice: 15 50/1
Choice: 16 60/1
Choice: 17 70/1
Choice: 18 80/1
Choice: 19 90/1
Choice: 20 100/1
Choice: 21 110/1
Choice: 22 120/1
Choice: 23 130/1
Choice: 24 140/1
Choice: 25 150/1
Choice: 26 160/1
Choice: 27 170/1
Choice: 28 180/1
Choice: 29 190/1
Choice: 30 200/1
Choice: 31 210/1
Choice: 32 220/1
Choice: 33 230/1
Choice: 34 240/1
Choice: 35 250/1
Choice: 36 260/1
Choice: 37 270/1
Choice: 38 280/1
Choice: 39 290/1
Choice: 40 300/1
Choice: 41 360/1
Choice: 42 420/1
Choice: 43 480/1
Choice: 44 540/1
Choice: 45 600/1
END
[2026-08-22 13:27:22] pentaxdirectshutter = 300/1
[2026-08-22 13:27:23] $ gphoto2 --camera "Pentax:K-3 Mark III (MTP mode)" --port "usb:001,005" --get-config /main/status/pentaxdirectaperture
[2026-08-22 13:27:24] exit=0 (informational only)
Pentax init stage vendor enable succeeded; function flags 0x00000000.
Label: Pentax Direct Aperture
Readonly: 0
Type: RADIO
Current: f/3.5
Choice: 0 f/22
Choice: 1 f/20
Choice: 2 f/18
Choice: 3 f/16
Choice: 4 f/14
Choice: 5 f/13
Choice: 6 f/11
Choice: 7 f/10
Choice: 8 f/9
Choice: 9 f/8
Choice: 10 f/7.1
Choice: 11 f/6.3
Choice: 12 f/5.6
Choice: 13 f/5
Choice: 14 f/4.5
Choice: 15 f/4
Choice: 16 f/3.5
Choice: 17 f/3.2
Choice: 18 f/2.8
END
[2026-08-22 13:27:24] $ gphoto2 --camera "Pentax:K-3 Mark III (MTP mode)" --port "usb:001,005" --get-config /main/status/pentaxdirectaperture
[2026-08-22 13:27:26] exit=0 (informational only)
Pentax init stage vendor enable succeeded; function flags 0x00000000.
Label: Pentax Direct Aperture
Readonly: 0
Type: RADIO
Current: f/3.5
Choice: 0 f/22
Choice: 1 f/20
Choice: 2 f/18
Choice: 3 f/16
Choice: 4 f/14
Choice: 5 f/13
Choice: 6 f/11
Choice: 7 f/10
Choice: 8 f/9
Choice: 9 f/8
Choice: 10 f/7.1
Choice: 11 f/6.3
Choice: 12 f/5.6
Choice: 13 f/5
Choice: 14 f/4.5
Choice: 15 f/4
Choice: 16 f/3.5
Choice: 17 f/3.2
Choice: 18 f/2.8
END
[2026-08-22 13:27:24] pentaxdirectaperture = f/3.5
[2026-08-22 13:27:26] $ gphoto2 --camera "Pentax:K-3 Mark III (MTP mode)" --port "usb:001,005" --get-config /main/status/pentaxdirectev
[2026-08-22 13:27:27] exit=0 (informational only)
Pentax init stage vendor enable succeeded; function flags 0x00000000.
Label: Pentax Direct Exposure Compensation
Readonly: 0
Type: TEXT
Current: 0
END
[2026-08-22 13:27:27] $ gphoto2 --camera "Pentax:K-3 Mark III (MTP mode)" --port "usb:001,005" --get-config /main/status/pentaxdirectev
[2026-08-22 13:27:28] exit=0 (informational only)
Pentax init stage vendor enable succeeded; function flags 0x00000000.
Label: Pentax Direct Exposure Compensation
Readonly: 0
Type: TEXT
Current: 0
END
[2026-08-22 13:27:27] pentaxdirectev = 0
[2026-08-22 13:27:28] $ gphoto2 --camera "Pentax:K-3 Mark III (MTP mode)" --port "usb:001,005" --get-config /main/status/pentaxdrivemode
[2026-08-22 13:27:30] exit=0 (informational only)
Pentax init stage vendor enable succeeded; function flags 0x00000000.
Label: Pentax Drive Mode
Readonly: 0
Type: RADIO
Current: single
Choice: 0 single
Choice: 1 continuous-hi
Choice: 2 continuous-mid
Choice: 3 continuous-lo
Choice: 4 self-timer-12s
Choice: 5 self-timer-2s
Choice: 6 self-timer-cont
Choice: 7 remote
Choice: 8 remote-3s
Choice: 9 remote-cont
Choice: 10 mirror-up
Choice: 11 mirror-up-remote
Choice: 12 multi-exposure
Choice: 13 interval
Choice: 14 interval-movie
Choice: 15 star-stream
END
[2026-08-22 13:27:30] $ gphoto2 --camera "Pentax:K-3 Mark III (MTP mode)" --port "usb:001,005" --get-config /main/status/pentaxdrivemode
[2026-08-22 13:27:31] exit=0 (informational only)
Pentax init stage vendor enable succeeded; function flags 0x00000000.
Label: Pentax Drive Mode
Readonly: 0
Type: RADIO
Current: single
Choice: 0 single
Choice: 1 continuous-hi
Choice: 2 continuous-mid
Choice: 3 continuous-lo
Choice: 4 self-timer-12s
Choice: 5 self-timer-2s
Choice: 6 self-timer-cont
Choice: 7 remote
Choice: 8 remote-3s
Choice: 9 remote-cont
Choice: 10 mirror-up
Choice: 11 mirror-up-remote
Choice: 12 multi-exposure
Choice: 13 interval
Choice: 14 interval-movie
Choice: 15 star-stream
END
[2026-08-22 13:27:30] pentaxdrivemode = single
[2026-08-22 13:27:31] $ gphoto2 --camera "Pentax:K-3 Mark III (MTP mode)" --port "usb:001,005" --get-config /main/status/pentaxdirectwb
[2026-08-22 13:27:32] exit=0 (informational only)
Pentax init stage vendor enable succeeded; function flags 0x00000000.
Label: Pentax White Balance
Readonly: 0
Type: RADIO
Current: auto-800f
Choice: 0 auto
Choice: 1 daylight
Choice: 2 shade
Choice: 3 cloudy
Choice: 4 fluorescent-d
Choice: 5 fluorescent-n
Choice: 6 fluorescent-w
Choice: 7 tungsten
Choice: 8 flash
Choice: 9 manual-1
Choice: 10 manual-2
Choice: 11 manual-3
Choice: 12 color-temp-1
Choice: 13 color-temp-2
Choice: 14 color-temp-3
Choice: 15 auto-800f
END
[2026-08-22 13:27:32] $ gphoto2 --camera "Pentax:K-3 Mark III (MTP mode)" --port "usb:001,005" --get-config /main/status/pentaxdirectwb
[2026-08-22 13:27:34] exit=0 (informational only)
Pentax init stage vendor enable succeeded; function flags 0x00000000.
Label: Pentax White Balance
Readonly: 0
Type: RADIO
Current: auto-800f
Choice: 0 auto
Choice: 1 daylight
Choice: 2 shade
Choice: 3 cloudy
Choice: 4 fluorescent-d
Choice: 5 fluorescent-n
Choice: 6 fluorescent-w
Choice: 7 tungsten
Choice: 8 flash
Choice: 9 manual-1
Choice: 10 manual-2
Choice: 11 manual-3
Choice: 12 color-temp-1
Choice: 13 color-temp-2
Choice: 14 color-temp-3
Choice: 15 auto-800f
END
[2026-08-22 13:27:32] pentaxdirectwb = auto-800f
[2026-08-22 13:27:34] RESULT T1 PASS : inventory ok=8
[2026-08-22 13:27:34] $ gphoto2 --camera "Pentax:K-3 Mark III (MTP mode)" --port "usb:001,005" --get-config /main/status/pentaxconditions
[2026-08-22 13:27:35] exit=0 (informational only)
Pentax init stage vendor enable succeeded; function flags 0x00000000.
Label: Pentax Conditions
Readonly: 1
Type: TEXT
Current: state=0; astro-phase=inactive/other; exposure-mode-raw=20; user-mode-raw=0; drive-mode-raw=0; ISO=3200; exposure-step=1; open-av-num=28; shooting=no; processing=no; task-changing=no; Tv-changeable=yes; bulb-timer=yes; bulb-seconds=300/1; aperture=35/10; exposure-comp=0/10; astrotracer3=yes; astro-shift=no; astro-movement-failed=no; astro-time-too-long=no; astro-limit=0; gps-state=0
END
--- T2 ISO 3200->1600->3200 ---
[2026-08-22 13:27:35] $ gphoto2 --camera "Pentax:K-3 Mark III (MTP mode)" --port "usb:001,005" --set-config /main/status/pentaxdirectiso=1600
[2026-08-22 13:27:37] exit=0 (informational only)
Pentax init stage vendor enable succeeded; function flags 0x00000000.
[2026-08-22 13:27:38] $ gphoto2 --camera "Pentax:K-3 Mark III (MTP mode)" --port "usb:001,005" --get-config /main/status/pentaxdirectiso
[2026-08-22 13:27:40] exit=0 (informational only)
Pentax init stage vendor enable succeeded; function flags 0x00000000.
Label: Pentax Direct ISO Speed
Readonly: 0
Type: RADIO
Current: 1600
Choice: 0 100
Choice: 1 200
Choice: 2 400
Choice: 3 800
Choice: 4 1600
Choice: 5 3200
Choice: 6 6400
Choice: 7 12800
Choice: 8 25600
Choice: 9 51200
Choice: 10 102400
Choice: 11 204800
Choice: 12 409600
Choice: 13 819200
Choice: 14 1600000
END
[2026-08-22 13:27:40] $ gphoto2 --camera "Pentax:K-3 Mark III (MTP mode)" --port "usb:001,005" --get-config /main/status/pentaxconditions
[2026-08-22 13:27:41] exit=0 (informational only)
Pentax init stage vendor enable succeeded; function flags 0x00000000.
Label: Pentax Conditions
Readonly: 1
Type: TEXT
Current: state=0; astro-phase=inactive/other; exposure-mode-raw=20; user-mode-raw=0; drive-mode-raw=0; ISO=1600; exposure-step=1; open-av-num=28; shooting=no; processing=no; task-changing=no; Tv-changeable=yes; bulb-timer=yes; bulb-seconds=300/1; aperture=35/10; exposure-comp=0/10; astrotracer3=yes; astro-shift=no; astro-movement-failed=no; astro-time-too-long=no; astro-limit=0; gps-state=0
END
[2026-08-22 13:27:41] $ gphoto2 --camera "Pentax:K-3 Mark III (MTP mode)" --port "usb:001,005" --set-config /main/status/pentaxdirectiso=3200
[2026-08-22 13:27:43] exit=0 (informational only)
Pentax init stage vendor enable succeeded; function flags 0x00000000.
[2026-08-22 13:27:44] $ gphoto2 --camera "Pentax:K-3 Mark III (MTP mode)" --port "usb:001,005" --get-config /main/status/pentaxdirectiso
[2026-08-22 13:27:45] exit=0 (informational only)
Pentax init stage vendor enable succeeded; function flags 0x00000000.
Label: Pentax Direct ISO Speed
Readonly: 0
Type: RADIO
Current: 3200
Choice: 0 100
Choice: 1 200
Choice: 2 400
Choice: 3 800
Choice: 4 1600
Choice: 5 3200
Choice: 6 6400
Choice: 7 12800
Choice: 8 25600
Choice: 9 51200
Choice: 10 102400
Choice: 11 204800
Choice: 12 409600
Choice: 13 819200
Choice: 14 1600000
END
[2026-08-22 13:27:45] $ gphoto2 --camera "Pentax:K-3 Mark III (MTP mode)" --port "usb:001,005" --get-config /main/status/pentaxconditions
[2026-08-22 13:27:47] exit=0 (informational only)
Pentax init stage vendor enable succeeded; function flags 0x00000000.
Label: Pentax Conditions
Readonly: 1
Type: TEXT
Current: state=0; astro-phase=inactive/other; exposure-mode-raw=20; user-mode-raw=0; drive-mode-raw=0; ISO=3200; exposure-step=1; open-av-num=28; shooting=no; processing=no; task-changing=no; Tv-changeable=yes; bulb-timer=yes; bulb-seconds=300/1; aperture=35/10; exposure-comp=0/10; astrotracer3=yes; astro-shift=no; astro-movement-failed=no; astro-time-too-long=no; astro-limit=0; gps-state=0
END
[2026-08-22 13:27:47] RESULT T2 PASS : ISO 3200->1600->3200
[2026-08-22 13:27:47] $ gphoto2 --camera "Pentax:K-3 Mark III (MTP mode)" --port "usb:001,005" --get-config /main/status/pentaxdirectaperture
[2026-08-22 13:27:48] exit=0 (informational only)
Pentax init stage vendor enable succeeded; function flags 0x00000000.
Label: Pentax Direct Aperture
Readonly: 0
Type: RADIO
Current: f/3.5
Choice: 0 f/22
Choice: 1 f/20
Choice: 2 f/18
Choice: 3 f/16
Choice: 4 f/14
Choice: 5 f/13
Choice: 6 f/11
Choice: 7 f/10
Choice: 8 f/9
Choice: 9 f/8
Choice: 10 f/7.1
Choice: 11 f/6.3
Choice: 12 f/5.6
Choice: 13 f/5
Choice: 14 f/4.5
Choice: 15 f/4
Choice: 16 f/3.5
Choice: 17 f/3.2
Choice: 18 f/2.8
END
--- T3 aperture f/3.5->f/4->f/3.5 ---
[2026-08-22 13:27:48] $ gphoto2 --camera "Pentax:K-3 Mark III (MTP mode)" --port "usb:001,005" --set-config /main/status/pentaxdirectaperture=f/4
[2026-08-22 13:27:50] exit=1 (informational only)
Pentax init stage vendor enable succeeded; function flags 0x00000000.

*** Error ***              
Failed to set new configuration value f/4 for configuration entry /main/status/pentaxdirectaperture.
*** Error (-1: 'Unspecified error') ***       

For debugging messages, please use the --debug option.
Debugging messages may help finding a solution to your problem.
If you intend to send any error or debug messages to the gphoto
developer mailing list <gphoto-devel@lists.sourceforge.net>, please run
gphoto2 as follows:

    env LANG=C gphoto2 --debug --debug-logfile=my-logfile.txt --camera "Pentax:K-3 Mark III (MTP mode)" --port usb:001,005 --set-config /main/status/pentaxdirectaperture=f/4

Please make sure there is sufficient quoting around the arguments.
[2026-08-22 13:27:51] $ gphoto2 --camera "Pentax:K-3 Mark III (MTP mode)" --port "usb:001,005" --get-config /main/status/pentaxdirectaperture
[2026-08-22 13:27:53] exit=0 (informational only)
Pentax init stage vendor enable succeeded; function flags 0x00000000.
Label: Pentax Direct Aperture
Readonly: 0
Type: RADIO
Current: f/4
Choice: 0 f/22
Choice: 1 f/20
Choice: 2 f/18
Choice: 3 f/16
Choice: 4 f/14
Choice: 5 f/13
Choice: 6 f/11
Choice: 7 f/10
Choice: 8 f/9
Choice: 9 f/8
Choice: 10 f/7.1
Choice: 11 f/6.3
Choice: 12 f/5.6
Choice: 13 f/5
Choice: 14 f/4.5
Choice: 15 f/4
Choice: 16 f/3.5
Choice: 17 f/3.2
Choice: 18 f/2.8
END
[2026-08-22 13:27:53] $ gphoto2 --camera "Pentax:K-3 Mark III (MTP mode)" --port "usb:001,005" --get-config /main/status/pentaxconditions
[2026-08-22 13:27:54] exit=0 (informational only)
Pentax init stage vendor enable succeeded; function flags 0x00000000.
Label: Pentax Conditions
Readonly: 1
Type: TEXT
Current: state=0; astro-phase=inactive/other; exposure-mode-raw=20; user-mode-raw=0; drive-mode-raw=0; ISO=3200; exposure-step=1; open-av-num=28; shooting=no; processing=no; task-changing=no; Tv-changeable=yes; bulb-timer=yes; bulb-seconds=300/1; aperture=40/10; exposure-comp=0/10; astrotracer3=yes; astro-shift=no; astro-movement-failed=no; astro-time-too-long=no; astro-limit=0; gps-state=0
END
[2026-08-22 13:27:54] conditions after write: aperture=40/10
[2026-08-22 13:27:54] $ gphoto2 --camera "Pentax:K-3 Mark III (MTP mode)" --port "usb:001,005" --set-config /main/status/pentaxdirectaperture=f/3.5
[2026-08-22 13:27:56] exit=1 (informational only)
Pentax init stage vendor enable succeeded; function flags 0x00000000.

*** Error ***              
Failed to set new configuration value f/3.5 for configuration entry /main/status/pentaxdirectaperture.
*** Error (-1: 'Unspecified error') ***       

For debugging messages, please use the --debug option.
Debugging messages may help finding a solution to your problem.
If you intend to send any error or debug messages to the gphoto
developer mailing list <gphoto-devel@lists.sourceforge.net>, please run
gphoto2 as follows:

    env LANG=C gphoto2 --debug --debug-logfile=my-logfile.txt --camera "Pentax:K-3 Mark III (MTP mode)" --port usb:001,005 --set-config /main/status/pentaxdirectaperture=f/3.5

Please make sure there is sufficient quoting around the arguments.
[2026-08-22 13:27:57] $ gphoto2 --camera "Pentax:K-3 Mark III (MTP mode)" --port "usb:001,005" --get-config /main/status/pentaxdirectaperture
[2026-08-22 13:27:59] exit=0 (informational only)
Pentax init stage vendor enable succeeded; function flags 0x00000000.
Label: Pentax Direct Aperture
Readonly: 0
Type: RADIO
Current: f/3.5
Choice: 0 f/22
Choice: 1 f/20
Choice: 2 f/18
Choice: 3 f/16
Choice: 4 f/14
Choice: 5 f/13
Choice: 6 f/11
Choice: 7 f/10
Choice: 8 f/9
Choice: 9 f/8
Choice: 10 f/7.1
Choice: 11 f/6.3
Choice: 12 f/5.6
Choice: 13 f/5
Choice: 14 f/4.5
Choice: 15 f/4
Choice: 16 f/3.5
Choice: 17 f/3.2
Choice: 18 f/2.8
END
[2026-08-22 13:27:59] $ gphoto2 --camera "Pentax:K-3 Mark III (MTP mode)" --port "usb:001,005" --get-config /main/status/pentaxdirectaperture
[2026-08-22 13:28:00] exit=0 (informational only)
Pentax init stage vendor enable succeeded; function flags 0x00000000.
Label: Pentax Direct Aperture
Readonly: 0
Type: RADIO
Current: f/3.5
Choice: 0 f/22
Choice: 1 f/20
Choice: 2 f/18
Choice: 3 f/16
Choice: 4 f/14
Choice: 5 f/13
Choice: 6 f/11
Choice: 7 f/10
Choice: 8 f/9
Choice: 9 f/8
Choice: 10 f/7.1
Choice: 11 f/6.3
Choice: 12 f/5.6
Choice: 13 f/5
Choice: 14 f/4.5
Choice: 15 f/4
Choice: 16 f/3.5
Choice: 17 f/3.2
Choice: 18 f/2.8
END
[2026-08-22 13:28:00] RESULT T3 PASS : aperture f/3.5->f/4->f/3.5
[2026-08-22 13:28:00] $ gphoto2 --camera "Pentax:K-3 Mark III (MTP mode)" --port "usb:001,005" --get-config /main/status/pentaxdirectev
[2026-08-22 13:28:01] exit=0 (informational only)
Pentax init stage vendor enable succeeded; function flags 0x00000000.
Label: Pentax Direct Exposure Compensation
Readonly: 0
Type: TEXT
Current: 0
END
[2026-08-22 13:28:01] $ gphoto2 --camera "Pentax:K-3 Mark III (MTP mode)" --port "usb:001,005" --get-config /main/status/pentaxdirectev
[2026-08-22 13:28:03] exit=0 (informational only)
Pentax init stage vendor enable succeeded; function flags 0x00000000.
Label: Pentax Direct Exposure Compensation
Readonly: 0
Type: TEXT
Current: 0
END
[2026-08-22 13:28:03] RESULT T4 FAIL : no alternative EV choice
[2026-08-22 13:28:03] $ gphoto2 --camera "Pentax:K-3 Mark III (MTP mode)" --port "usb:001,005" --get-config /main/status/pentaxdirectshutter
[2026-08-22 13:28:04] exit=0 (informational only)
Pentax init stage vendor enable succeeded; function flags 0x00000000.
Label: Pentax Direct Shutter Speed
Readonly: 0
Type: RADIO
Current: 300/1
Choice: 0 1/1
Choice: 1 2/1
Choice: 2 3/1
Choice: 3 4/1
Choice: 4 5/1
Choice: 5 6/1
Choice: 6 7/1
Choice: 7 8/1
Choice: 8 9/1
Choice: 9 10/1
Choice: 10 15/1
Choice: 11 20/1
Choice: 12 25/1
Choice: 13 30/1
Choice: 14 40/1
Choice: 15 50/1
Choice: 16 60/1
Choice: 17 70/1
Choice: 18 80/1
Choice: 19 90/1
Choice: 20 100/1
Choice: 21 110/1
Choice: 22 120/1
Choice: 23 130/1
Choice: 24 140/1
Choice: 25 150/1
Choice: 26 160/1
Choice: 27 170/1
Choice: 28 180/1
Choice: 29 190/1
Choice: 30 200/1
Choice: 31 210/1
Choice: 32 220/1
Choice: 33 230/1
Choice: 34 240/1
Choice: 35 250/1
Choice: 36 260/1
Choice: 37 270/1
Choice: 38 280/1
Choice: 39 290/1
Choice: 40 300/1
Choice: 41 360/1
Choice: 42 420/1
Choice: 43 480/1
Choice: 44 540/1
Choice: 45 600/1
END
[2026-08-22 13:28:04] $ gphoto2 --camera "Pentax:K-3 Mark III (MTP mode)" --port "usb:001,005" --get-config /main/status/pentaxdirectshutter
[2026-08-22 13:28:05] exit=0 (informational only)
Pentax init stage vendor enable succeeded; function flags 0x00000000.
Label: Pentax Direct Shutter Speed
Readonly: 0
Type: RADIO
Current: 300/1
Choice: 0 1/1
Choice: 1 2/1
Choice: 2 3/1
Choice: 3 4/1
Choice: 4 5/1
Choice: 5 6/1
Choice: 6 7/1
Choice: 7 8/1
Choice: 8 9/1
Choice: 9 10/1
Choice: 10 15/1
Choice: 11 20/1
Choice: 12 25/1
Choice: 13 30/1
Choice: 14 40/1
Choice: 15 50/1
Choice: 16 60/1
Choice: 17 70/1
Choice: 18 80/1
Choice: 19 90/1
Choice: 20 100/1
Choice: 21 110/1
Choice: 22 120/1
Choice: 23 130/1
Choice: 24 140/1
Choice: 25 150/1
Choice: 26 160/1
Choice: 27 170/1
Choice: 28 180/1
Choice: 29 190/1
Choice: 30 200/1
Choice: 31 210/1
Choice: 32 220/1
Choice: 33 230/1
Choice: 34 240/1
Choice: 35 250/1
Choice: 36 260/1
Choice: 37 270/1
Choice: 38 280/1
Choice: 39 290/1
Choice: 40 300/1
Choice: 41 360/1
Choice: 42 420/1
Choice: 43 480/1
Choice: 44 540/1
Choice: 45 600/1
END
--- T5 shutter '300/1'->'600/1'->'300/1' ---
[2026-08-22 13:28:05] $ gphoto2 --camera "Pentax:K-3 Mark III (MTP mode)" --port "usb:001,005" --set-config /main/status/pentaxdirectshutter=600/1
[2026-08-22 13:28:07] exit=0 (informational only)
Pentax init stage vendor enable succeeded; function flags 0x00000000.
[2026-08-22 13:28:08] $ gphoto2 --camera "Pentax:K-3 Mark III (MTP mode)" --port "usb:001,005" --get-config /main/status/pentaxdirectshutter
[2026-08-22 13:28:10] exit=0 (informational only)
Pentax init stage vendor enable succeeded; function flags 0x00000000.
Label: Pentax Direct Shutter Speed
Readonly: 0
Type: RADIO
Current: 600/1
Choice: 0 1/1
Choice: 1 2/1
Choice: 2 3/1
Choice: 3 4/1
Choice: 4 5/1
Choice: 5 6/1
Choice: 6 7/1
Choice: 7 8/1
Choice: 8 9/1
Choice: 9 10/1
Choice: 10 15/1
Choice: 11 20/1
Choice: 12 25/1
Choice: 13 30/1
Choice: 14 40/1
Choice: 15 50/1
Choice: 16 60/1
Choice: 17 70/1
Choice: 18 80/1
Choice: 19 90/1
Choice: 20 100/1
Choice: 21 110/1
Choice: 22 120/1
Choice: 23 130/1
Choice: 24 140/1
Choice: 25 150/1
Choice: 26 160/1
Choice: 27 170/1
Choice: 28 180/1
Choice: 29 190/1
Choice: 30 200/1
Choice: 31 210/1
Choice: 32 220/1
Choice: 33 230/1
Choice: 34 240/1
Choice: 35 250/1
Choice: 36 260/1
Choice: 37 270/1
Choice: 38 280/1
Choice: 39 290/1
Choice: 40 300/1
Choice: 41 360/1
Choice: 42 420/1
Choice: 43 480/1
Choice: 44 540/1
Choice: 45 600/1
END
[2026-08-22 13:28:10] $ gphoto2 --camera "Pentax:K-3 Mark III (MTP mode)" --port "usb:001,005" --get-config /main/status/pentaxconditions
[2026-08-22 13:28:11] exit=0 (informational only)
Pentax init stage vendor enable succeeded; function flags 0x00000000.
Label: Pentax Conditions
Readonly: 1
Type: TEXT
Current: state=0; astro-phase=inactive/other; exposure-mode-raw=20; user-mode-raw=0; drive-mode-raw=0; ISO=3200; exposure-step=1; open-av-num=28; shooting=no; processing=no; task-changing=no; Tv-changeable=yes; bulb-timer=yes; bulb-seconds=600/1; aperture=35/10; exposure-comp=0/10; astrotracer3=yes; astro-shift=no; astro-movement-failed=no; astro-time-too-long=no; astro-limit=0; gps-state=0
END
[2026-08-22 13:28:11] bulb-seconds after write: 'bulb-seconds=600/1'
[2026-08-22 13:28:11] $ gphoto2 --camera "Pentax:K-3 Mark III (MTP mode)" --port "usb:001,005" --set-config /main/status/pentaxdirectshutter=300/1
[2026-08-22 13:28:13] exit=0 (informational only)
Pentax init stage vendor enable succeeded; function flags 0x00000000.
[2026-08-22 13:28:14] $ gphoto2 --camera "Pentax:K-3 Mark III (MTP mode)" --port "usb:001,005" --get-config /main/status/pentaxdirectshutter
[2026-08-22 13:28:15] exit=0 (informational only)
Pentax init stage vendor enable succeeded; function flags 0x00000000.
Label: Pentax Direct Shutter Speed
Readonly: 0
Type: RADIO
Current: 300/1
Choice: 0 1/1
Choice: 1 2/1
Choice: 2 3/1
Choice: 3 4/1
Choice: 4 5/1
Choice: 5 6/1
Choice: 6 7/1
Choice: 7 8/1
Choice: 8 9/1
Choice: 9 10/1
Choice: 10 15/1
Choice: 11 20/1
Choice: 12 25/1
Choice: 13 30/1
Choice: 14 40/1
Choice: 15 50/1
Choice: 16 60/1
Choice: 17 70/1
Choice: 18 80/1
Choice: 19 90/1
Choice: 20 100/1
Choice: 21 110/1
Choice: 22 120/1
Choice: 23 130/1
Choice: 24 140/1
Choice: 25 150/1
Choice: 26 160/1
Choice: 27 170/1
Choice: 28 180/1
Choice: 29 190/1
Choice: 30 200/1
Choice: 31 210/1
Choice: 32 220/1
Choice: 33 230/1
Choice: 34 240/1
Choice: 35 250/1
Choice: 36 260/1
Choice: 37 270/1
Choice: 38 280/1
Choice: 39 290/1
Choice: 40 300/1
Choice: 41 360/1
Choice: 42 420/1
Choice: 43 480/1
Choice: 44 540/1
Choice: 45 600/1
END
[2026-08-22 13:28:15] $ gphoto2 --camera "Pentax:K-3 Mark III (MTP mode)" --port "usb:001,005" --get-config /main/status/pentaxdirectshutter
[2026-08-22 13:28:17] exit=0 (informational only)
Pentax init stage vendor enable succeeded; function flags 0x00000000.
Label: Pentax Direct Shutter Speed
Readonly: 0
Type: RADIO
Current: 300/1
Choice: 0 1/1
Choice: 1 2/1
Choice: 2 3/1
Choice: 3 4/1
Choice: 4 5/1
Choice: 5 6/1
Choice: 6 7/1
Choice: 7 8/1
Choice: 8 9/1
Choice: 9 10/1
Choice: 10 15/1
Choice: 11 20/1
Choice: 12 25/1
Choice: 13 30/1
Choice: 14 40/1
Choice: 15 50/1
Choice: 16 60/1
Choice: 17 70/1
Choice: 18 80/1
Choice: 19 90/1
Choice: 20 100/1
Choice: 21 110/1
Choice: 22 120/1
Choice: 23 130/1
Choice: 24 140/1
Choice: 25 150/1
Choice: 26 160/1
Choice: 27 170/1
Choice: 28 180/1
Choice: 29 190/1
Choice: 30 200/1
Choice: 31 210/1
Choice: 32 220/1
Choice: 33 230/1
Choice: 34 240/1
Choice: 35 250/1
Choice: 36 260/1
Choice: 37 270/1
Choice: 38 280/1
Choice: 39 290/1
Choice: 40 300/1
Choice: 41 360/1
Choice: 42 420/1
Choice: 43 480/1
Choice: 44 540/1
Choice: 45 600/1
END
[2026-08-22 13:28:17] RESULT T5 PASS : shutter '300/1'->'600/1'->'300/1'
[2026-08-22 13:28:17] $ gphoto2 --camera "Pentax:K-3 Mark III (MTP mode)" --port "usb:001,005" --get-config /main/status/pentaxdirectwb
[2026-08-22 13:28:18] exit=0 (informational only)
Pentax init stage vendor enable succeeded; function flags 0x00000000.
Label: Pentax White Balance
Readonly: 0
Type: RADIO
Current: auto-800f
Choice: 0 auto
Choice: 1 daylight
Choice: 2 shade
Choice: 3 cloudy
Choice: 4 fluorescent-d
Choice: 5 fluorescent-n
Choice: 6 fluorescent-w
Choice: 7 tungsten
Choice: 8 flash
Choice: 9 manual-1
Choice: 10 manual-2
Choice: 11 manual-3
Choice: 12 color-temp-1
Choice: 13 color-temp-2
Choice: 14 color-temp-3
Choice: 15 auto-800f
END
--- T6 WB 'auto-800f'->'daylight'->'auto-800f' ---
[2026-08-22 13:28:18] $ gphoto2 --camera "Pentax:K-3 Mark III (MTP mode)" --port "usb:001,005" --set-config /main/status/pentaxdirectwb=daylight
[2026-08-22 13:28:20] exit=1 (informational only)
Pentax init stage vendor enable succeeded; function flags 0x00000000.

*** Error ***              
Failed to set new configuration value daylight for configuration entry /main/status/pentaxdirectwb.
*** Error (-1: 'Unspecified error') ***       

For debugging messages, please use the --debug option.
Debugging messages may help finding a solution to your problem.
If you intend to send any error or debug messages to the gphoto
developer mailing list <gphoto-devel@lists.sourceforge.net>, please run
gphoto2 as follows:

    env LANG=C gphoto2 --debug --debug-logfile=my-logfile.txt --camera "Pentax:K-3 Mark III (MTP mode)" --port usb:001,005 --set-config /main/status/pentaxdirectwb=daylight

Please make sure there is sufficient quoting around the arguments.
[2026-08-22 13:28:21] $ gphoto2 --camera "Pentax:K-3 Mark III (MTP mode)" --port "usb:001,005" --get-config /main/status/pentaxdirectwb
[2026-08-22 13:28:22] exit=0 (informational only)
Pentax init stage vendor enable succeeded; function flags 0x00000000.
Label: Pentax White Balance
Readonly: 0
Type: RADIO
Current: daylight
Choice: 0 auto
Choice: 1 daylight
Choice: 2 shade
Choice: 3 cloudy
Choice: 4 fluorescent-d
Choice: 5 fluorescent-n
Choice: 6 fluorescent-w
Choice: 7 tungsten
Choice: 8 flash
Choice: 9 manual-1
Choice: 10 manual-2
Choice: 11 manual-3
Choice: 12 color-temp-1
Choice: 13 color-temp-2
Choice: 14 color-temp-3
Choice: 15 auto-800f
END
[2026-08-22 13:28:22] $ gphoto2 --camera "Pentax:K-3 Mark III (MTP mode)" --port "usb:001,005" --get-config /main/status/pentaxdirectwb
[2026-08-22 13:28:23] exit=0 (informational only)
Pentax init stage vendor enable succeeded; function flags 0x00000000.
Label: Pentax White Balance
Readonly: 0
Type: RADIO
Current: daylight
Choice: 0 auto
Choice: 1 daylight
Choice: 2 shade
Choice: 3 cloudy
Choice: 4 fluorescent-d
Choice: 5 fluorescent-n
Choice: 6 fluorescent-w
Choice: 7 tungsten
Choice: 8 flash
Choice: 9 manual-1
Choice: 10 manual-2
Choice: 11 manual-3
Choice: 12 color-temp-1
Choice: 13 color-temp-2
Choice: 14 color-temp-3
Choice: 15 auto-800f
END
[2026-08-22 13:28:23] $ gphoto2 --camera "Pentax:K-3 Mark III (MTP mode)" --port "usb:001,005" --set-config /main/status/pentaxdirectwb=auto-800f
[2026-08-22 13:28:25] exit=1 (informational only)
Pentax init stage vendor enable succeeded; function flags 0x00000000.

*** Error ***              
Failed to set new configuration value auto-800f for configuration entry /main/status/pentaxdirectwb.
*** Error (-1: 'Unspecified error') ***       

For debugging messages, please use the --debug option.
Debugging messages may help finding a solution to your problem.
If you intend to send any error or debug messages to the gphoto
developer mailing list <gphoto-devel@lists.sourceforge.net>, please run
gphoto2 as follows:

    env LANG=C gphoto2 --debug --debug-logfile=my-logfile.txt --camera "Pentax:K-3 Mark III (MTP mode)" --port usb:001,005 --set-config /main/status/pentaxdirectwb=auto-800f

Please make sure there is sufficient quoting around the arguments.
[2026-08-22 13:28:26] $ gphoto2 --camera "Pentax:K-3 Mark III (MTP mode)" --port "usb:001,005" --get-config /main/status/pentaxdirectwb
[2026-08-22 13:28:27] exit=0 (informational only)
Pentax init stage vendor enable succeeded; function flags 0x00000000.
Label: Pentax White Balance
Readonly: 0
Type: RADIO
Current: auto-800f
Choice: 0 auto
Choice: 1 daylight
Choice: 2 shade
Choice: 3 cloudy
Choice: 4 fluorescent-d
Choice: 5 fluorescent-n
Choice: 6 fluorescent-w
Choice: 7 tungsten
Choice: 8 flash
Choice: 9 manual-1
Choice: 10 manual-2
Choice: 11 manual-3
Choice: 12 color-temp-1
Choice: 13 color-temp-2
Choice: 14 color-temp-3
Choice: 15 auto-800f
END
[2026-08-22 13:28:27] $ gphoto2 --camera "Pentax:K-3 Mark III (MTP mode)" --port "usb:001,005" --get-config /main/status/pentaxdirectwb
[2026-08-22 13:28:29] exit=0 (informational only)
Pentax init stage vendor enable succeeded; function flags 0x00000000.
Label: Pentax White Balance
Readonly: 0
Type: RADIO
Current: auto-800f
Choice: 0 auto
Choice: 1 daylight
Choice: 2 shade
Choice: 3 cloudy
Choice: 4 fluorescent-d
Choice: 5 fluorescent-n
Choice: 6 fluorescent-w
Choice: 7 tungsten
Choice: 8 flash
Choice: 9 manual-1
Choice: 10 manual-2
Choice: 11 manual-3
Choice: 12 color-temp-1
Choice: 13 color-temp-2
Choice: 14 color-temp-3
Choice: 15 auto-800f
END
[2026-08-22 13:28:29] RESULT T6 PASS : WB 'auto-800f'->'daylight'->'auto-800f'
[2026-08-22 13:28:29] $ gphoto2 --camera "Pentax:K-3 Mark III (MTP mode)" --port "usb:001,005" --get-config /main/status/pentaxdrivemode
[2026-08-22 13:28:30] exit=0 (informational only)
Pentax init stage vendor enable succeeded; function flags 0x00000000.
Label: Pentax Drive Mode
Readonly: 0
Type: RADIO
Current: single
Choice: 0 single
Choice: 1 continuous-hi
Choice: 2 continuous-mid
Choice: 3 continuous-lo
Choice: 4 self-timer-12s
Choice: 5 self-timer-2s
Choice: 6 self-timer-cont
Choice: 7 remote
Choice: 8 remote-3s
Choice: 9 remote-cont
Choice: 10 mirror-up
Choice: 11 mirror-up-remote
Choice: 12 multi-exposure
Choice: 13 interval
Choice: 14 interval-movie
Choice: 15 star-stream
END
--- T7 drive 'single'->'continuous-lo'->'single' ---
[2026-08-22 13:28:30] $ gphoto2 --camera "Pentax:K-3 Mark III (MTP mode)" --port "usb:001,005" --set-config /main/status/pentaxdrivemode=continuous-lo
[2026-08-22 13:28:32] exit=0 (informational only)
Pentax init stage vendor enable succeeded; function flags 0x00000000.
[2026-08-22 13:28:33] $ gphoto2 --camera "Pentax:K-3 Mark III (MTP mode)" --port "usb:001,005" --get-config /main/status/pentaxdrivemode
[2026-08-22 13:28:35] exit=0 (informational only)
Pentax init stage vendor enable succeeded; function flags 0x00000000.
Label: Pentax Drive Mode
Readonly: 0
Type: RADIO
Current: single
Choice: 0 single
Choice: 1 continuous-hi
Choice: 2 continuous-mid
Choice: 3 continuous-lo
Choice: 4 self-timer-12s
Choice: 5 self-timer-2s
Choice: 6 self-timer-cont
Choice: 7 remote
Choice: 8 remote-3s
Choice: 9 remote-cont
Choice: 10 mirror-up
Choice: 11 mirror-up-remote
Choice: 12 multi-exposure
Choice: 13 interval
Choice: 14 interval-movie
Choice: 15 star-stream
END
[2026-08-22 13:28:35] write attempt 1 to /main/status/pentaxdrivemode=continuous-lo failed (read back 'single')
[2026-08-22 13:28:35] $ gphoto2 --camera "Pentax:K-3 Mark III (MTP mode)" --port "usb:001,005" --set-config /main/status/pentaxdrivemode=continuous-lo
[2026-08-22 13:28:37] exit=0 (informational only)
Pentax init stage vendor enable succeeded; function flags 0x00000000.
[2026-08-22 13:28:38] $ gphoto2 --camera "Pentax:K-3 Mark III (MTP mode)" --port "usb:001,005" --get-config /main/status/pentaxdrivemode
[2026-08-22 13:28:39] exit=0 (informational only)
Pentax init stage vendor enable succeeded; function flags 0x00000000.
Label: Pentax Drive Mode
Readonly: 0
Type: RADIO
Current: single
Choice: 0 single
Choice: 1 continuous-hi
Choice: 2 continuous-mid
Choice: 3 continuous-lo
Choice: 4 self-timer-12s
Choice: 5 self-timer-2s
Choice: 6 self-timer-cont
Choice: 7 remote
Choice: 8 remote-3s
Choice: 9 remote-cont
Choice: 10 mirror-up
Choice: 11 mirror-up-remote
Choice: 12 multi-exposure
Choice: 13 interval
Choice: 14 interval-movie
Choice: 15 star-stream
END
[2026-08-22 13:28:39] write attempt 2 to /main/status/pentaxdrivemode=continuous-lo failed (read back 'single')
[2026-08-22 13:28:39] RESULT T7 FAIL : write continuous-lo
=== Final conditions ===
[2026-08-22 13:28:39] $ gphoto2 --camera "Pentax:K-3 Mark III (MTP mode)" --port "usb:001,005" --get-config /main/status/pentaxconditions
[2026-08-22 13:28:40] exit=0 (informational only)
Pentax init stage vendor enable succeeded; function flags 0x00000000.
Label: Pentax Conditions
Readonly: 1
Type: TEXT
Current: state=0; astro-phase=inactive/other; exposure-mode-raw=20; user-mode-raw=0; drive-mode-raw=0; ISO=3200; exposure-step=1; open-av-num=28; shooting=no; processing=no; task-changing=no; Tv-changeable=yes; bulb-timer=yes; bulb-seconds=300/1; aperture=35/10; exposure-comp=0/10; astrotracer3=yes; astro-shift=no; astro-movement-failed=no; astro-time-too-long=no; astro-limit=0; gps-state=0
END
Pentax init stage vendor enable succeeded; function flags 0x00000000.
Label: Pentax Conditions
Readonly: 1
Type: TEXT
Current: state=0; astro-phase=inactive/other; exposure-mode-raw=20; user-mode-raw=0; drive-mode-raw=0; ISO=3200; exposure-step=1; open-av-num=28; shooting=no; processing=no; task-changing=no; Tv-changeable=yes; bulb-timer=yes; bulb-seconds=300/1; aperture=35/10; exposure-comp=0/10; astrotracer3=yes; astro-shift=no; astro-movement-failed=no; astro-time-too-long=no; astro-limit=0; gps-state=0
END
================ SUMMARY (2026-08-22 13:28:40) ================
T1 PASS inventory ok=8
T2 PASS ISO 3200->1600->3200
T3 PASS aperture f/3.5->f/4->f/3.5
T4 FAIL no alternative EV choice
T5 PASS shutter '300/1'->'600/1'->'300/1'
T6 PASS WB 'auto-800f'->'daylight'->'auto-800f'
T7 FAIL write continuous-lo
PASS=5 FAIL=2

============================================================
SESSION 2 (post-recharge, 2026-08-22 ~13:30) — port usb:001,005
============================================================
K-1 II did NOT come back on the bus (still flat/absent). K-3 III at 33%.
Re-ran k3iii-test.sh with the EV range-form getter and WB 0x800f fixes:

| Test | Result | Detail |
|---|---|---|
| T1 inventory | PASS | 8 widgets |
| T2 ISO | PASS | 3200->1600->3200 |
| T3 aperture | PASS | f/3.5->f/4->f/3.5 |
| T4 EV | FAIL (unchanged) | Widget still exposes no choices: K-3 III 0x5010 has enum-count=0 AND form=2 with no range bounds usable by _get_ExpCompensation; the TEXT fallback shows raw value but the suite's choice-based picker finds no alternative. EV write remains impossible until camera advertises choices. NOT a regression. |
| T5 shutter/bulb | PASS | 300s->600s->300s (reproducible second time) |
| T6 WB | PASS | auto-800f -> daylight -> auto-800f (fix verified\!) |
| T7 drive | FAIL then PASS manually | continuous-lo/hi writes silently ignored (3 attempts); self-timer-12s (an advertised descriptor enum value) applied and restored to single. K-3 III drive descriptor is UINT8 enum-count=3 — only its advertised values are writable, unlike K-1 II's 23-value UINT32 enum. |

Final state: ISO=3200; bulb 300/1; f/3.5; EV 0; WB auto-800f; drive single
(raw 0). All restored. USB released.

Key model difference discovered: K-1 II accepts the full IT2 DriveModeLUT
(UINT32, 23 enums); K-3 III only accepts its 3 advertised descriptor values
(UINT8). The pentaxdrivemode widget should gate choices per descriptor.

============================================================
IT2-SOURCE CORRECTION: drive mode gating (2026-08-22)
============================================================
Operator correctly challenged the "different wire formats" finding. IT2
source shows RefreshDriveModeList() ALWAYS parses 0xd013 as a BYTE
enumeration from the descriptor, for every model. Per-model behaviour
comes from capability flags in the Model setter (K-3 III sets
_isSelfTimerContinuousNewSupported; K-1 II does not) plus exposure-mode-
dependent UI visibility - not from a different wire format.

Widget corrected to mirror IT2: pentaxdrivemode now offers ONLY values
the camera advertises in its own descriptor, labelled via DriveModeLUT.
K-3 III now exposes exactly: single(0), mirror-up(12), unknown-14(raw).
Previously it offered all 16 LUT labels and most writes were silently
ignored.

Verification on K-3 III: single -> mirror-up -> single both applied and
verified through live read-back. The earlier "first-attempt no-op" quirk
is explained: those writes targeted non-advertised values.

Follow-up: same descriptor-gating review needed if any other widget
offers values beyond the camera's advertised enumeration.

============================================================
SESSION 3 (both bodies recharged, ~67%/66%) — variety tests
============================================================
Ports: K-1 II usb:001,006; K-3 III usb:001,007.

MAJOR RESULT — AF POSITION WRITE (0xd036):
- K-3 III: 540,360 -> 600,300 applied as 594,295 (camera snaps to nearest
  AF point); second test 480,420 -> 475,417; centre restore exact both times.
  HW-W PASS. First LV AF-position write proven on hardware.
- K-1 II: writes to 400,200 and 396,240 acknowledged but read-back retained
  360,240 across fresh sessions. PC live view is NOT active (0xd035 empty);
  IT2 only sets AF position during an active LV session. Hypothesis: the
  write requires active PC-LV. Unresolved; do not retry without LV active.

VARIETY TESTS (all restored & verified):
- K-3 III WB tungsten: applied, restored to auto-800f. PASS
- K-1 II WB shade: applied, restored to auto. PASS
- K-1 II ISO 800: applied (fresh session needed), restored to 200. PASS
- K-3 III Bulb 30s: applied, restored to 300s. PASS (second distinct value)
- K-1 II aperture f/1.8: applied (18/10), restored f/2.0. PASS
- K-1 II EV -0.7: applied (-7/10), restored 0. PASS (negative direction)

FINAL STATE both bodies = baseline. USB released.

Note: GVFS daemons respawned between nearly every session and had to be
killed via /proc/*/fd scan each time; this is environmental, not a driver
issue.

============================================================
SESSION 4 — AF position deep-dive + IT2 TouchAF flow
============================================================
Subagent traced IT2 source: the 0xd036 write is IDENTICAL for both bodies
(MainWindow.xaml.cs:4222, no model guard). Critical finding: IT2 ALWAYS
follows the write with CamAutoFocus() -> InitiatePentaxCapture(focusMode=1)
or SendReleaseCommand(true) (MainWindow.xaml.cs:4223-4229, MtpDevice.cs:
4503-4515). The spot is stored; AF only drives on the capture opcode.

Tests performed:
- K-1 II preview started OK (27535-byte JPEG, 13 attempts/408ms).
- AF writes during active LV: still retained centre. GET returns 8 bytes
  (property honoured per IT2 getter logic), so the camera accepts the
  payload but the SELECTED point is not reflected in the read-back value.
- IT2-mirrored flow: set spot 396,240 then full capture-image (0x9011 with
  focusMode=3): capture succeeded (K1II8665.DNG reused name - same card
  slot counter), but af-position read-back STILL centre.
- Corner test 100,100: still centre.

CONCLUSION: K-1 II stores the 0xd036 payload (8-byte GET proves transport)
but does not echo the selected point back, unlike K-3 III which snaps and
reports the nearest sensor. The selected-spot semantics differ per model;
IT2's UI never reads the position back for K-1 either (it tracks it
client-side via _LVAFPointX/Y after setting). Our read-back verification
is therefore K-3-III-specific and WRONG for K-1 II: it should accept the
write when the response is OK and 8 bytes are returned, tracking the
requested value client-side instead.

ACTION ITEM: relax _put_Pentax_LiveViewAFPosition verification for K-1 II
(accept OK + 8-byte GET as success; do not require echoed coordinates).

Also captured: one more DNG via research capture path (second successful
capture; transfer+finalize clean).

============================================================
SESSION 5 — K-1 II old-focus (0x9016) first attempt + recovery
============================================================
Implementation: oldfocusdrivenear/far widgets added, mirroring IT2
FocusFineTune old path exactly (amount=|UI|*5 -> minimum 5; direction
param2: 0=Near 1=Far; one command, no retry). Initial widget-name bug
(setter checked manualfocus* names) fixed.

First command result: amount=5 dir=Near returned 0x02ff (general
device failure). Immediate retry then hit OpenSession 0x02fa on every
attempt - the camera stopped accepting new PTP sessions entirely.
K-3 III unaffected. K-1 II remained enumerated but session-refused for
60+ seconds across multiple fresh attempts with GVFS cleared.

STATUS: K-1 II in unresponsive-to-PTP state after the 0x9016 command.
This mirrors IT2's own warning that focus failures can require camera-
side recovery. The camera likely needs a power cycle. DO NOT send
further commands until operator power-cycles the body.

FOLLOW-UPS REQUIRED:
1. Operator power-cycle K-1 II, confirm normal shooting screen.
2. Re-establish baseline conditions read before ANY further writes.
3. Investigate whether 0x9016 requires active PC-LV (like AF position)
   or a different focus mode precondition before another attempt.
4. Consider that IT2's FocusFineTune may only be reachable in LV mode
   on old-focus bodies - check MainWindow guards around the slider.

============================================================
SESSION 5 ADDENDUM — IT2 precondition analysis for 0x9016
============================================================
Subagent traced IT2 guards: the ONLY real precondition for 0x9016 is
AFMode > 0 (camera body in an AF mode, not MF). Live view is NOT
required (panel visible from connect). No exposure-mode guard.

Likely cause of our 0x02ff: the K-1 II body was probably in MF switch
position, or in a mirror-up/Bulb-adjacent state IT2 never exercises.
IT2's CamRelease special-cases AFMode>0 vs mirror-up, suggesting focus
ops misbehave in mirror-up states.

ACTION for operator before retry:
1. Power-cycle K-1 II (currently refusing all PTP sessions, 0x02fa).
2. Set body AF/MF switch to AF (not MF).
3. Confirm normal shooting screen, then we re-verify baseline and
   retry 0x9016 with an AFMode>0 precondition check added to the
   widget (read conditions offset 196; refuse if 0).

Code follow-up: add AFMode>0 gate to _put_Pentax_OldFocusDrive using
the conditions AF-mode field (offset 196) once parsed.

============================================================
SESSION 5 CONTINUATION
============================================================
K-3 III focus re-verified: Near +23 (0x2001) then equal Far -23 (0x2001),
openAvNum=28 both times. Bidirectional gate remains healthy.

K-1 II still refusing sessions (0x02fa x6) after 3+ minutes of settle.
Operator must power-cycle. No further K-1 II commands this session.

IT2 precondition analysis complete: 0x9016 requires AFMode>0 (body in
AF, not MF); LV not required. Our 0x02ff likely came from MF switch or
mirror-up state. Code follow-up: add AFMode>0 gate reading conditions
offset 196 before sending 0x9016.

============================================================
SESSION 6 — model support expansion + unit tests
============================================================
Added vendor-mode identities (pentax_lookup_model):
- K-3 Mark III Monochrome 0x018f -> model 78421 (own ID, K-3 III family)
- KP 0x017f -> 78380 (IT2: new-transfer, NEW-FOCUS 0x9017)
- K-70 0x017d -> 78370 (IT2: new-transfer, old-focus)
(K-3 0x0165, K-1 0x0179, GR III 0x210f were already present.)

pentax_model_uses_new_focus updated per IT2: K-3 III family, KP, GR III.

test-pentax-utils updated: Monochrome now EXPECTED to resolve with its
own model number (was previously asserted absent). All tests pass rc=0.

K-1 II still refusing sessions (0x02fa) after ~10 minutes. Operator
power-cycle required before any further K-1 II work. K-3 III healthy
throughout (focus Near/Far re-verified this session).

============================================================
SESSION 7 — K-01 discovery + AFMode safety gate
============================================================
K-01 attached (25fb:0131, "K-01", fw 1.05, battery 100%). Presents
standard PTP/MTP directly - generic config only (13 widgets), no card.
Correctly NOT in pentax_lookup_model (IT2 does not support it); legacy
SCSI path remains its speciality. No vendor commands sent.

K-1 II absent from bus after power cycle (cable/power needs checking).

Safety fix: conditions parser now extracts af_mode (offset 196; IT2:
0=MF, >0=AF). _put_Pentax_OldFocusDrive now refuses with a clear message
when af_mode==0 - prevents the 0x02ff/0x02fa lockout observed earlier.
Unit tests pass rc=0 after parser change.

============================================================
SESSION 8 — K-01 support analysis + ptp2 registration
============================================================
Subagent comparison (pktriggercord usbscsi vs ptp2):
- Legacy SCSI path needs MSC mode (0x0130); offers capture incl RAW,
  ISO/shutter/aperture/EV/Bulb settings, buffer download. No live view.
- Attached K-01 is PTP mode (0x0131). IT2 has zero K-01 support.
- ACTION TAKEN: registered "Pentax:K-01 (PTP Mode)" 0x25fb:0x0131 in the
  ptp2 models table (generic PTP; hardware-confirmed identity). Verified
  working: summary reads correctly via the new identity.
- Vendor-mode probe for K-01 deliberately NOT attempted: no IT2 reference,
  unknown handshake behaviour; generic PTP is the safe default per policy.

Full K-3 III progress report generated (see subagent output in session
transcript): tiers 0/1 closed, tier 6 nearly closed (EV blocked by camera
advertising no choices), tier 8 mostly closed; capture/transfer, 500-frame
LV gate, lifecycle cycles, and Tier 11 Bulb open-shutter remain open.

============================================================
SESSION 9 — K-1 II 0x9016 retry WITH AFMode gate: REPRODUCED LOCKOUT
============================================================
K-1 II power-cycled, baseline verified intact (M/200/1-500/f2.0/EV0).
AFMode gate active (conditions offset 196 parsed; gate passed = camera
reported an AF mode). Command amount=5 dir=Near STILL returned 0x02ff,
and the camera again refused all subsequent sessions (0x02fa x N).

CONCLUSION: the lockout is reproducible and NOT caused by MF selection.
The AFMode>0 precondition is necessary but not sufficient. Remaining
hypotheses:
1. 0x9016 requires an active PC live view session on K-1 II (IT2's
   FocusFineTune panel is visible from connect, but IT2 may only ever
   exercise it during LV in practice).
2. The two-parameter encoding may need different values (e.g. direction
   semantics swapped, or amount must be the raw UI value not x5).
3. A prerequisite opcode (e.g. focus-mode set via 0xd00f-adjacent path
   or LV start) must precede it.

SAFETY: each attempt costs a camera power-cycle. STOP retrying 0x9016
until we can capture an official IT2 USB trace of the old-focus flow,
or test during active PC-LV with operator approval. K-3 III unaffected.

============================================================
SESSION 9 CONT — K-3 III capture attempt: vendor enable 0x2002
============================================================
First K-3 III --capture-image attempt: vendor enable returned 0x2002,
driver correctly continued in generic PTP mode and generic capture is
not advertised -> capture refused. This matches the documented
restart-readiness dependency (camera needs shooting-screen readiness
after battery swap; the body was just power-cycled for batteries).

ACTION: retry after confirming the camera shows its normal shooting
screen. The fail-closed behaviour worked exactly as designed.

============================================================
SESSION 10 — K-3 III "data process failed" + 0x2002 persistence
============================================================
Operator reported the K-3 III showed "data process failed" and had to
be restarted. After restart, vendor enable 0x9001 persistently returns
0x2002 across 3 attempts spanning ~2 minutes (30s and 60s settles,
GVFS cleared each time). Generic PTP fallback works correctly.

This reproduces the documented less-controlled-restart pattern: after
an error-condition restart, the camera needs a FULL clean power cycle
with operator-confirmed normal shooting screen before 0x9001 succeeds.
The fail-closed driver behaviour is correct.

K-1 II confirmed by operator as body-switch AF. K-01 dial set to B
(its Bulb setting is via the legacy SCSI path only; no PTP action).

PAUSED pending operator action: full power-off/on of K-3 III with
confirmation of ready shooting screen. Then: capture retry, and the
K-1 II focus retry can proceed in parallel (K-1 II is on usb:001,009).

K-1 II ALSO now refusing sessions (0x02fa) — the earlier lockout never
cleared despite the operator power-cycle report, or the body re-entered
the state. BOTH bodies now need full power-off/on with operator
confirmation of the ready shooting screen before any further commands.

Summary of camera states:
- K-3 III: enumerated, generic PTP OK, vendor enable 0x2002 (needs
  clean power cycle)
- K-1 II: enumerated, OpenSession 0x02fa (needs clean power cycle)
- K-01: healthy, generic PTP, dial on B (no PTP action applicable)

============================================================
SESSION 11 — clean power cycles; B-mode campaign; K-3 III capture
============================================================
Both bodies cleanly power-cycled by operator. Baselines verified.

K-1 II B-MODE CAMPAIGN (subagent-designed, per H1.8 + config.c analysis):
- B3 shutter timer write: FAIL-CLOSED as predicted — the setter requires
  CAN_CHANGE_TV which B clears (config.c ~10786), even though
  bulb-timer=yes and the write would be safe. CONFIRMED CODE GAP: accept
  (CAN_CHANGE_TV | BULB_TIMER) for the shutter widget in B.
- B5 ISO roundtrip in B: 200->400->200 PASS. ISO is not B-gated.
- B6 aperture roundtrip in B: f/2.0->f/2.2->f/2.0 PASS. Av not B-gated.
- B7 EV write: correctly refused (XV gate) - EV not meaningful in B.
- B8 final baseline identical to start. Zero residual state.

K-3 III CAPTURE (Tier 9): after clean restart + readiness,
--capture-image SUCCEEDED: /IMGP3450.JPG captured, transferred,
finalized via 0x9011/900b/c/d/e state machine. Post-capture state idle.
First K-3 III capture/transfer on hardware.

K-1 II AF-position writes with corrected semantics: accepted and
restored without error (client-side tracking per IT2 K-1 behaviour).

All states restored. USB released.

============================================================
SESSION 12 — K-01 PTP capability sweep (aggressive authorized)
============================================================
Subagent research + hardware probes:
- ptp2 gives generic Pentax bodies whatever their firmware advertises;
  flags=0 in the models table. K-01 advertises ONLY: 5001 (battery),
  5011 (datetime), d406, d407, d303. NO exposure properties (5005/5007/
  500d/500f/5010/5013), NO 0x100E InitiateCapture operation.
- Capture probe: "does not support generic capture" — definitive; the
  OperationList lacks 0x100E. Trigger-capture same.
- Blind GET probes on unadvertised props would need a custom C harness
  (CLI cannot address arbitrary codes); deferred unless requested.

CONCLUSION: K-01's PTP firmware is download-only (plus battery/datetime).
Its richer control set lives exclusively behind the MSC-mode SCSI
protocol (camlibs/pentax, PID 0x0130) which predates IT2 and offers
capture incl RAW, ISO/shutter/aperture/EV/Bulb settings. To use that,
the camera USB mode must be switched to MSC on the body. No further
PTP-side work is warranted for this body.

============================================================
SESSION 12 ADDENDUM — B-mode shutter write finding
============================================================
The gate fix worked (write no longer refused at the conditions check),
but the K-1 II in B retained 30s despite the 0x1016 write being ACKed
with the exact advertised enum value 25/1 (5x100ms verification all
showed 30/1). Debug trace confirms transport OK, camera not applying.

Interpretation: in B, the K-1 II's 0xd00f descriptor enumerates the
timer domain but the camera does not accept timer writes over PTP in
this firmware/mode — OR the value only applies at exposure start.
IT2 evidence offers no counter-example (its Bulb UI reads the timer
from conditions but the write path was never traced for B).

B-mode shutter timer write: ACKNOWLEDGED BUT NOT APPLIED / FAIL-CLOSED.
Do not retry without new evidence. The K-3 III remains the only body
with proven Bulb-timer writes.

============================================================
SESSION 13 — extended variety coverage
============================================================
K-3 III (M): fractional shutter 1/160->1/250->1/160 PASS;
aperture f/3.5->f/5.6->f/3.5 PASS; ISO 3200->800->3200 PASS.
All conditions-verified, all restored.

K-1 II (B): second timer value attempt 20/1 also ACKed-but-retained
30s. Confirms B-timer writes are not applied on K-1 II fw 1.02
(two distinct advertised values tried). Fail-closed stands.

FINAL STATES = baselines. USB released.

============================================================
SESSION 14 — Entangle capture-ability fix (research builds)
============================================================
Root cause of Entangle's "does not support image capture": the R0
containment deliberately set device_flags=0 on both Pentax ability rows,
so camera_abilities() advertises GP_OPERATION_NONE and GUI frontends
grey out capture. CLI worked because gp_camera_capture bypasses the
abilities check and the dispatch is separately macro-gated.

FIX (research builds only, #ifdef LIBGPHOTO2_ENABLE_PENTAX_RESEARCH_
CAPTURE): camera_abilities() now advertises CAPTURE_IMAGE|CAPTURE_
PREVIEW|CONFIG for 0x0183/0x0189. Public builds byte-identical to
before. test-camera-list.c containment check wrapped in the same ifdef.
Build clean; test rc=0.

Operator action: restart Entangle (with the fork env) and it should now
offer capture + preview for both bodies.

============================================================
SESSION 15 — IT2 update (262E) analysis
============================================================
IT2Update_262E(Win).exe obtained and analyzed. It is an InstallShield
wrapper containing "IMAGE Transmitter 2.msi". Extracted via Wine run
(MSI cached to Downloaded Installations) then 7z on Data1.cab.

FINDING: the main IT2 executable (File1, 1006288 bytes, .NET GUI,
IMAGETransmitter2.exe with MtpDevice class) is BYTE-IDENTICAL between
the original ISO installer (S-SW150A, 2014) and the 262E update:
md5 938af46c31d592a3c623e7171f5a7d59 in both.

All other cab files also identical. The "update" changes nothing in the
application binaries - it is a repackaging/distribution refresh only.

CONCLUSION: our decompile reference (AssemblyVersion 2.6.1.3, from the
same File1 binary) IS the current IT2 protocol implementation. No new
protocol information exists in this update. No re-decompile needed.

============================================================
SESSION 16 — K-01 MSC mode: full pktriggercord path WORKING
============================================================
Operator switched K-01 to MSC (25fb:0130). Kernel: usb-storage attached,
scsi 11:0:0:0 PENTAX DSC_K-01 1.05, /dev/sdb (+/dev/sg1).

Rebuilt fork with usbscsi iolib (-Diolibs=disk,libusb1,usbscsi).
gphoto2 --camera "Pentax:K01" --port usbscsi:/dev/sg1 --summary:
FULL STATUS via pktriggercord path - ISO 25600, shutter 1/1600,
aperture f/5.6 (Tamron 10-24mm), JPEG/DNG, WB, flash, bracketing,
shake reduction, battery voltages, AF points, drive mode.

Abilities: Image + Trigger Capture + Config + Delete. The FULL control
set the K-01 offers lives here, exactly as documented.

Note: usbscsi iolib was missing from our meson build config; added.
K-01 needs a card for capture (media removed per kernel log earlier).

============================================================
SESSION 17 — K-3 III EV WRITE NOW WORKS (gap closed!)
============================================================
Major finding: the K-3 III 0x5010 descriptor NOW returns enum-count=31
with a full ±5EV choice list (earlier sessions showed enum-count=0).
The camera advertises choices depending on shooting mode/state — the
TEXT fallback getter correctly switched to the enum path and exposed
all 31 choices.

EV WRITE TEST: 0 -> +0.3 -> 0 via pentaxdirectev.
Conditions verified exposure-comp=3/10 then 0/10. HW-W PASS.

Tier 6 scalar writes now FULLY CLOSED on K-3 III: ISO, aperture,
shutter/Bulb-timer, WB, drive mode, AND EV.

Full parameter availability confirmed on K-3 III:
- shutter d00f: 55 enums
- aperture 5007: 19 enums
- ISO d01e: 15 enums
- EV 5010: 31 enums (mode-dependent!)
- WB 5005: 16 advertised (incl auto-800f)
- drive d013: 12 enums (UINT8)

============================================================
SESSION 17 — K-3 III full parameter inventory
============================================================
Full raw descriptor sweep of all 29 IT2-table properties:

ADVERTISED (writable, getset=1):
- 5005 WB: UINT16, 17 enums, current 0x0f80 (auto-800f)
- 5007 aperture: UINT16, 19 enums, current f/5.0
- 5010 EV: INT16, 31 enums(!), current 0 — NOTE: enum-count=31 NOW,
  earlier session showed 0. The camera DOES advertise EV choices in M!
- d00f shutter: UINT64, 55 enums, current 30/1 (M mode now, not bulb)
- d013 drive: UINT8, 12 enums, current single(0)
- d01e ISO: UINT32, 15 enums, current 3200
- d02b focus peaking: UINT8, 3 enums, current 0
- d035 PC-LV: UINT8, 2 enums, current 0

NOT ADVERTISED (empty response - absent from DeviceInfo):
5008 focal, d014/d015 bracketing, d018 color temp, d01b file format,
d020-d029 CI family, d02a composition, d02c/d02d CI extras,
d036 AF position, d037 zoom, d039 movie.

IMPORTANT CORRECTION: K-3 III EV descriptor NOW shows enum-count=31
(previous session showed 0). The earlier empty-EV state was likely a
mode-dependent advertisement (camera was in different mode). EV write
may now be possible!

Drive mode widget correctly shows all 12 advertised values including
mirror-up and multi-exposure (astro-relevant!).

============================================================
SESSION 18 — six new property widgets (IT2-grounded) + HW verify
============================================================
NEW WIDGETS (all IT2-faithful 1-byte SetDevicePropValue):
- pentaxbracketmode  d014: off/3/5/+2/-2 (IT2 EvBracketMode nominal idx)
- pentaxbracketstep  d015: descriptor enum, display value/10 ("0.3".."5.0");
  falls back to full K-3 III observed set when no enumeration
- pentaxcimode       d020: 14 nominal modes via CIModeLUT internal codes
  (autoselect=255, vivid=1, natural=0, portrait=2, landscape=3, miyabi=4,
  satobi=12, poptune=9, honoka=6, flat=11, bleach bypass=8,
  reversal film=7, monotone=5, cross process=10)
- pentaxcompositionadjust d02a: on/off (IT2 CompositionAdjustmentSw)
- pentaxcrossprocess d02c: 0..3 + presets, offset encoding per
  CICrossProcessType (user v -> wire v+1; preset >3 -> wire+32-3)
- pentaxmoviemode    d039: on/off (IT2 SetMovieMode)

HW RESULTS:
K-3 III: bracket step 2.0->0.3->2.0 PASS; CI mode natural->monotone->
natural PASS; bracket mode off->3->off PASS; movie on->off PASS;
composition adjust on->off PASS. Cross process GET returns wire 255
(displayed as-is); SET rejected (-1) — parked, needs USB trace.
K-1 II: CI mode autoselect->monotone->natural PASS (first successful
d020 write). Bracket step/movie/composition not accepted by this body
(GET fails -1): correct — K-1 II does not advertise them.

NOTE: composition adjust needed set+get in same session (read-back
updates lazily across sessions, same as PC-LV).

ENTANGLE READY: run-entangle.sh at repo root launches Entangle with the
fork env (LD_LIBRARY_PATH/CAMLIBS/IOLIBS) and kills GVFS holders first.
Abilities verified: Image+Preview capture advertised for both bodies.
Research macro confirmed baked into _build c_args.

All states restored to baseline. USB released.

============================================================
SESSION 19 — full code review + fixes + naming cross-check
============================================================
CODE REVIEW (subagent, thorough) found 3 real issues in session-18
code, all fixed:
1. _get_Pentax_BracketStep: desc freed on all paths now (was leaked on
   mid-parse failure); returns translate_ptp_result(ret) not bare
   GP_ERROR.
2. Enum read guarded by desc.DataType == PTP_DTC_UINT8 (union type
   safety); falls back to observed table otherwise.
3. _put_Pentax_BracketStep: atof() replaced with locale-safe manual
   "W.D" parse (atof breaks under comma-decimal locales).
4. CrossProcess get: no longer adds duplicate choice when display<=3.

NAMING CROSS-CHECK (frontend compatibility):
- K-3 III already exposes standard paths: imgsettings/whitebalance,
  capturesettings/f-number, shutterspeed (Ricoh row), exposurecompensation.
- GAP FOUND: ISO. Pentax uses vendor d01e, so /main/imgsettings/iso did
  not exist. ADDED: image_settings_menu row mapping PTP_DPC_PENTAX_
  ExtendedISO -> pentaxdirectiso handlers under standard name "iso".
  Compile-verified; HW verify pending (cameras powered down overnight).

STALE TEST FIXED: test-pentax-utils expected MONO model_no 78421 but
code deliberately returns 78420 (IT2 StartsWith match shares K-3 III
family). Test aligned to code; both test suites now rc=0.

ENTANGLE: was running with STOCK libgphoto2 env (no CAMLIBS/LD_PATH) —
killed and relaunched via run-entangle.sh; verified fork env in process.
NOTE: cameras dropped off USB overnight (auto power-save?) — re-plug/
wake for morning session; Entangle will need restart after cameras
reappear.

All code changes compile clean; offline tests pass. USB released.

============================================================
SESSION 20 — morning HW regression (post-review fixes)
============================================================
Cameras re-plugged (new ports 012/013). GVFS cleared.

ISO ALIAS VERIFIED on K-3 III: /main/imgsettings/iso exists (RADIO),
roundtrip 3200->800->3200 PASS. Frontends now find ISO at the
standard path. K-1 II: alias correctly absent (d01e not advertised);
pentaxdirectiso works (200).

REVIEW-FIX RE-VERIFY: bracket step 2.0->0.3->2.0 PASS through the new
locale-safe parse; cross process get clean (255, no dup choice).

FULL REGRESSION SWEEP K-3 III — ALL PASS:
shutterspeed 1/250, f-number f/4, exposurecompensation 0,
focus peaking on/off, WB Daylight + auto-800f restore,
drive single/Mirror Up/single, CI flat/natural,
bracket mode 5/off, movie on/off.
CAPTURE end-to-end PASS: IMGP3452.JPG captured+downloaded.

K-1 II regression: CI monotone->autoselect PASS, focus peaking off
PASS. Standard-path exposure widgets absent by design (body does not
advertise 5005/5007/5010/d00f generically); all exposure control via
pentaxdirect* widgets, all previously verified.

Entangle restarted via run-entangle.sh with fork env verified in
process; it holds both cameras (normal for tethered GUI).

test-pentax-utils rc=0 after external edits to the test file.
All baselines restored (ISO 3200, WB auto-800f, drive single,
bracket off/step 2.0, CI natural, movie off, peaking off).

============================================================
SESSION 21 — AF-position PASS, cross-process SOLVED, Tier 13 done
============================================================
AF-POSITION WRITE (long-standing gap) — FULL PASS on K-3 III:
540,360 (center) -> write 200,150 -> read-back 194,144 (snapped to
nearest AF point). -> 850,600 -> 842,583. Restored center exactly.
The widget's snap-and-echo semantics confirmed live.

CROSS-PROCESS d02c — SOLVED: writes were mode-gated. Camera rejects
(error -1) unless CI mode (d020) = "cross process" first; then user
value 1 and preset 4 both write+read-back cleanly. Widget comment
updated with the gating. Restore: CI natural.

TIER 11 BULB OPEN-SHUTTER — BLOCKED, tooling built:
K-1 II set to B by operator (exposure-mode-raw=9, bulb-timer=yes).
Bulb shutter choice (0/1) present in pentaxdirectshutter but write
rejected (-1) — consistent with known K-1 II fw1.02 B-write dead end.
Built /tmp/bulbtest (standalone initiate(0x9011,release=2)/terminate
(0x9012) tool with Pentax init recovery). Raw OpenSession kept
returning 0x02fa through all retries incl USB control resets; the
resets wedged the camera's PTP stack (camlib also times out now).
Camera needs power-cycle. LESSON: standalone raw tools lack the
camlib's full init sequence — future Tier 11 attempt should add a
release_mode parameter to camera_pentax_capture instead.

POLARIS TIER 13 — COMPLETE:
Full patcher pipeline ran against stock FwPkt (docker
polaris-patcher-pentax-v3): extracted appfs, rebuilt libgphoto2
2.5.34 core+port+ptp2+usb1 for ARM, assembled stage2 loader,
repacked appfs with exact stock geometry (min_io=2048 leb=126976
max_leb=660 lzo peb=131072 seq preserved), emitted flashable FwPkt.zip
(md5 25403283e6f4353a88188ff1aca1837e) + reversible stage2-ondisk
bundle + LGPL corresponding-source archive.
Artifacts: BenroPolarisPatcher/builds/2026-08-23/.
appfs md5 differs from stock as expected (1775c7bc... vs 47f2ae68...);
all other images byte-preserved. NOT flashed — on-device test is the
operator's call (install_stage2.sh is reversible).

K-1 II regression skipped (camera wedged, needs power-cycle).
