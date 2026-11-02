# Plan: Six useful activities for Kestrel Island

> Deep-plan · 2026-10-31 · โครงการ `/home/dit/Sites/Labs/Vorssaint-KDE`
> ขอบเขตและชุด 6 ฟีเจอร์ได้รับการยืนยันจากผู้ใช้ในแชตแล้ว: notch เท่านั้น, ส่งมอบรุ่นเล็กก่อนแล้วเพิ่มทีละช่วง
> เอกสารนี้เป็นแผน ไม่ได้เพิ่มฟีเจอร์หรือเปลี่ยนแอปที่กำลังทำงาน

## 1. Context

นำเฉพาะแนวคิดที่ทำให้ notch มีประโยชน์จาก Vorssaint มาใช้กับแอป native KDE เดิม: เพลง, เสียง/ไมค์, Focus/Pomodoro, keep awake, แจ้งเตือน Plasma และสถานะงาน AI/คำสั่ง ไม่มีการย้าย Swift มาทั้งชุด ไม่มี dashboard ใหม่ รักษา notch 288×40, expanded 456×304, hover และการเลือกเพลงอัตโนมัติ ไม่มี dropdown เลือก player

**Evidence convention:** `VERIFIED` คืออ่านโค้ด/เอกสาร/ผลทดสอบในรอบนี้; `CORRECTED` คือความเข้าใจที่มีหลักฐานใหม่หักล้าง; `UNVERIFIED U#` มี spike และเงื่อนไขหยุดที่ชัดเจนด้านล่าง ส่วน `DECISION` เป็นข้อกำหนดที่จะสร้าง ไม่ใช่การอ้างว่ามีแล้ว ตำแหน่ง `file:line` อ้างอิง source ณ วันเขียน; ไฟล์ใหม่ที่ระบุใน roadmap เป็นสิ่งที่จะสร้าง

**Definition of done — DECISION:** ทั้งหกฟีเจอร์เปิดใช้ได้ทีละส่วนจาก island, ฟีเจอร์ที่อ่านข้อมูลส่วนตัวเป็น opt-in, ทดสอบอัตโนมัติบน private D-Bus และตรวจด้วยเมาส์บน KDE Wayland จริง ทั้งหมดต้องรักษา input-mask, การคืน keyboard focus, hover และตำแหน่งจอเดิม การไม่ผ่าน safety/capability spike ต้องทำให้ฟีเจอร์นั้น unavailable พร้อมเหตุผล ไม่ใช่ใช้วิธีดักข้อมูลทดแทน

**Non-goals — DECISION:** ไม่ทำ per-app mixer, clipboard history อีกชุด, calendar editor, download manager, file shelf, scratchpad, screenshot/recording suite, window manager, package manager, cleaner, remote lyrics, remote artwork, audio capture/equalizer, AI cost/token/account dashboard หรือ adapter ที่อ่าน Codex/Claude session logs อัตโนมัติ ไม่เปลี่ยนค่าปุ่มลัด เสียง การแจ้งเตือน หรือ power policy ของ KDE โดยพลการ

**Confirmed constraints — VERIFIED:** ผู้ใช้ยืนยันเลือก 6 ฟีเจอร์ใน Decision Brief และต้องการแบ่งช่วง; ความชอบเดิมบันทึกใน `PRODUCT.md:27–29` และตัวแอปที่ `qml/Main.qml:37`, `qml/Main.qml:44`, `src/backend.h:44` ไม่เปลี่ยนชื่อ/โลโก้เป็น upstream; โครงการนี้เป็น implementation อิสระตาม `README.md:3`

### Research Questions

| ID | คำถามและผลค้นคว้า |
|---|---|
| R1 | Upstream island ทำอะไรจริง? **VERIFIED:** มี 16 module และ compact activities หลายชนิด; อ่าน source ที่ pin commit ใน §3/S1–S5 ไม่ถือว่า README ของ toolkit ทั้งหมดคือ scope ของ island |
| R2 | ของเรามีอะไรแล้วและติดตรงไหน? **VERIFIED:** inventory §2, audit §4; เพลง/เสียง/timer มีแล้ว แต่ยังไม่มี activity coordinator, seek, Pomodoro, inhibition, watcher หรือ task bridge |
| R3 | Linux/KDE รองรับกลไกใด? **VERIFIED:** Qt 6.12/LayerShellQt 6.8.2, MPRIS 2.2, pactl, PowerDevil และ KDE notification watcher ใน §3; runtime policy ต้องผ่าน U2/U3 |
| R4 | แสดงหลายกิจกรรมอย่างไรโดยไม่รบกวน hover? **VERIFIED:** ตอนนี้ timer แทนชื่อเพลงโดยตรงที่ `qml/Main.qml:92`; upstream มี arbitration ที่ S2; ข้อกำหนดใหม่อยู่ D3 |
| R5 | ต้องอ่าน/เก็บข้อมูลส่วนตัวอะไร? **VERIFIED:** ปัจจุบัน settings เท่านั้นและ state ใน memory (§2); **DECISION:** task/notification ไม่ persist เนื้อหา ไม่อ่าน log หรือ clipboard; lock/DND behavior เป็น U3 |
| R6 | งาน AI จะ detect อัตโนมัติจากทุกโปรแกรมได้หรือไม่? **UNVERIFIED U4:** ยังไม่มี provider-specific event contract ที่ตรวจแล้ว; แผนนี้จึงใช้ explicit local task bridge เท่านั้น และมี spike พิสูจน์ wrapper lifecycle ใน Phase 6 ไม่อ้างว่าติดตาม Codex desktop เองได้ |
| R7 | การลาก/เปลี่ยนจอและการคืน focus พิสูจน์แค่ไหน? **VERIFIED:** implementation อยู่ `src/backend.cpp:299`, `:405`; tests โหลด target เท่านั้น (`tests/test_backend.cpp:92`); **UNVERIFIED U1:** physical drag, Escape cancel, hotplug และ typing restoration ต้องผ่าน manual gate ใน Phase 0 |
| R8 | ฐานทดสอบ/ประวัติ/การติดตั้งเป็นอย่างไร? **VERIFIED:** รอบนี้ private-bus run ผ่าน 13 test cases + init/cleanup = 15 pass; ไม่มี commit ใน branch จึง **UNVERIFIED U0:** git trajectory ตรวจไม่ได้; Phase 0 มี baseline task ก่อนเริ่ม implementation |

## 2. Findings: current state

ทุกแถวเป็น **VERIFIED** จาก source ปัจจุบัน คำว่า works หมายถึงมีเส้นทางใช้งานที่ต่อครบ ไม่ได้หมายความว่า physical-session behavior ทุกอย่างผ่านแล้ว

| Component | Status | Evidence |
|---|---|---|
| QApplication, QML resource, tray, single instance | works; มี error-path defect | `src/main.cpp:31–83`, `CMakeLists.txt:8–16` |
| Black notch, compact/expanded, hover delay, settings | works; pin-after-hover ไม่ครบ | `qml/Main.qml:25–52`, `:72–140`, `:263–303` |
| รูปทรงและ input region | works; runtime animation/click-through gate ยังไม่ครบ | `qml/Main.qml:60–68`, `src/backend.cpp:281–289` |
| Monitor settings และ layer recreation | works | `src/backend.cpp:299–326`, `:380–400` |
| Drag-to-display | half-wired ในเชิงหลักฐานทดสอบ; code path ต่อครบ | `src/backend.cpp:405–442`, `qml/MoveTarget.qml:10`, `tests/test_backend.cpp:92–105` |
| Automatic media selection / MPRIS transport | works; polling 1.5 s และยังไม่มี timeline | `src/backend.cpp:42–50`, `:71–164`, `qml/Main.qml:178–210` |
| Output volume/mute, serialized/coalesced commands | works | `src/backend.cpp:192–253`; queue/debounce tests `tests/test_backend.cpp:194–247` |
| Timer presets, pause/resume/cancel, completion | works; countdown เดี่ยวและไม่มี completion-state test | `src/backend.cpp:255–274`, `qml/Main.qml:213–237`, `tests/test_backend.cpp:147–168` |
| CPU/memory | works; เก็บของเดิม ไม่เพิ่ม monitor suite | `src/backend.cpp:167–189`, `qml/Main.qml:240–261` |
| QSettings / opt-in autostart | works | `src/backend.cpp:329–367`, `:390–400` |
| `previewMode` context flag, `media.player`, icon foreground | dead/half-wired | flag ส่งแต่ QML ไม่อ่าน (`src/main.cpp:64`); normalized player ไม่แสดงแล้ว (`src/backend.cpp:85`); tint ไม่ถูก Image ใช้ (`qml/IslandButton.qml:7–30`) |
| Build, install, tests | works | `CMakeLists.txt:1–30`; `dbus-run-session -- ./build/test-backend -platform offscreen` รอบนี้: 15 passed, 0 failed, 0 skipped |
| Commit history / CI | ไม่มี commit และไม่พบ CI configuration ใน file inventory | `git log -n 30 --oneline` ตอบว่า branch ไม่มี commits; `CMakeLists.txt:17–30` เป็น local test pipeline ปัจจุบัน |

### Read → transform → render / action → write

**VERIFIED:** Session bus ListNames/GetAll → normalize → rank หลัง replies ครบ → `media` Q_PROPERTY → QML (`src/backend.cpp:88–150`); `/proc` → stats (`:167`); pactl JSON → volume/mute (`:192`); QSettings → hover/display/topGap (`src/backend.h:44–48`) Local artwork อ่านเฉพาะ `file://` (`qml/Main.qml:186–193`)

**VERIFIED:** ผู้ใช้กด control → capability check → MPRIS async call (`src/backend.cpp:153–164`); volume → argument-based QProcess (`:226–253`); preferences → QSettings (`:390–400`); startup opt-in → QSaveFile desktop entry (`:352–361`); accepted display drop → screen preference (`:402–442`) ยังไม่มี database, network ingestion หรือ background reader ของฟีเจอร์ใหม่

**Operational snapshot — VERIFIED:** `pacman -Q` รอบนี้ได้ Qt 6.12.0, LayerShellQt/Plasma Workspace/PowerDevil 6.8.2, libpulse 17.0, PipeWire 1.6.10, UPower 1.91.5 Runtime เป็น user service เรียก `/home/dit/.local/bin/kestrel-island --screen DP-2` สเกลที่ยืนยันคือ desktop session ของผู้ใช้คนเดียวกับหลายจอ ไม่ใช่บริการหลายผู้ใช้ ไม่มี production database migration

## 3. Findings: framework ground truth

### Upstream ที่อ่านจริง

**VERIFIED:** pin `vorssaint/vorssaint-utils` ที่ commit [`a5da7633e09d0ebbf5b5607c0434ded28a239469`](https://github.com/vorssaint/vorssaint-utils/commit/a5da7633e09d0ebbf5b5607c0434ded28a239469) ไม่ใช้ `main` ลอยเป็นสัญญาการออกแบบ

| Ref | ข้อเท็จจริงที่ยืนยัน | Source ที่ pin แล้ว |
|---|---|---|
| S1 | Island มี module ย่อยจำนวนมากและบาง module พึ่ง service ของ toolkit อื่น; ไม่ใช่ทุกอย่างแยก port ได้ทันที | [NotchSupport.swift:9–84](https://github.com/vorssaint/vorssaint-utils/blob/a5da7633e09d0ebbf5b5607c0434ded28a239469/Sources/Vorssaint/Services/Notch/NotchSupport.swift#L9-L84) |
| S2 | Compact activity selection รวม timer/music/keep-awake และอื่น ๆ; transient feedback มี priority/duration; event ซ้ำไม่จำเป็นต้องเริ่ม resize ใหม่ | [NotchService.swift:541–555](https://github.com/vorssaint/vorssaint-utils/blob/a5da7633e09d0ebbf5b5607c0434ded28a239469/Sources/Vorssaint/Services/Notch/NotchService.swift#L541-L555), [NotchSupport.swift:1604–1645](https://github.com/vorssaint/vorssaint-utils/blob/a5da7633e09d0ebbf5b5607c0434ded28a239469/Sources/Vorssaint/Services/Notch/NotchSupport.swift#L1604-L1645), [NotchService.swift:2319–2349](https://github.com/vorssaint/vorssaint-utils/blob/a5da7633e09d0ebbf5b5607c0434ded28a239469/Sources/Vorssaint/Services/Notch/NotchService.swift#L2319-L2349) |
| S3 | Music มี scrub timeline และจัดการ pending seek/track context; ไม่ใช่ progress bar ตกแต่ง | [NotchMusicView.swift:631–707](https://github.com/vorssaint/vorssaint-utils/blob/a5da7633e09d0ebbf5b5607c0434ded28a239469/Sources/Vorssaint/UI/Notch/NotchMusicView.swift#L631-L707) |
| S4 | Timer มี Pomodoro state, anchor-based time, explicit next phase และ completed activity ที่รอ dismiss | [NotchTimerSupport.swift:96–195](https://github.com/vorssaint/vorssaint-utils/blob/a5da7633e09d0ebbf5b5607c0434ded28a239469/Sources/Vorssaint/Services/Notch/NotchTimerSupport.swift#L96-L195) |
| S5 | AI activity แสดงเวลา/เหตุการณ์เสร็จได้ แต่ upstream ยังมี account/usage dashboard ซึ่งไม่อยู่ใน scope นี้ | [NotchAgentSupport.swift:9–73](https://github.com/vorssaint/vorssaint-utils/blob/a5da7633e09d0ebbf5b5607c0434ded28a239469/Sources/Vorssaint/Services/Notch/NotchAgentSupport.swift#L9-L73), [NotchService.swift:4253–4280](https://github.com/vorssaint/vorssaint-utils/blob/a5da7633e09d0ebbf5b5607c0434ded28a239469/Sources/Vorssaint/Services/Notch/NotchService.swift#L4253-L4280) |
| S6 | Download fallback มี received bytes แต่ไม่รู้ total; scanner ไม่สามารถสรุป % ได้ | [NotchDownloadService.swift:351–384](https://github.com/vorssaint/vorssaint-utils/blob/a5da7633e09d0ebbf5b5607c0434ded28a239469/Sources/Vorssaint/Services/Notch/NotchDownloadService.swift#L351-L384) |

### Mechanisms ที่รองรับเป้าหมาย KDE

แต่ละแถวเป็น **VERIFIED** เฉพาะ API/contract; runtime acceptance ที่ยังไม่ทดลองอยู่ใน U1–U4 ของ §1 และ Phase spikes

| Ref | Mechanism / fact | Versioned source / installed evidence |
|---|---|---|
| F1 | MPRIS มี Position, Rate, CanSeek, SetPosition และ Seeked; Position ไม่ broadcast property change ตามปกติ จึงต้อง sample + extrapolate | [MPRIS 2.2 Player](https://specifications.freedesktop.org/mpris/latest/Player_Interface.html) |
| F2 | Qt D-Bus รองรับ asyncCall, signal subscription และ service-owner watcher | Qt 6.12 `/usr/include/qt6/QtDBus/qdbusconnection.h:108–143`, `/usr/include/qt6/QtDBus/qdbusservicewatcher.h:30–59` |
| F3 | LayerShell มี overlay/top anchor, screen, margins และ None/OnDemand keyboard modes | LayerShellQt 6.8.2 `/usr/include/LayerShellQt/window.h:40–87`; แอปใช้จริงที่ `src/backend.cpp:299–326` |
| F4 | pactl มี event subscription ที่อยู่ยาว และ get/set source mute; ตั้ง argv ได้โดยไม่ประกอบ shell command | Installed libpulse 17.0 `man pactl`: subscribe, get-source-mute, set-source-mute; [pactl command reference](https://man.archlinux.org/man/pactl.1); current command execution `src/backend.cpp:226–246` |
| F5 | จอ idle inhibition กับ sleep inhibition เป็นคนละ service; รับและคืน cookie แบบ unsigned uint32 | `/usr/share/dbus-1/interfaces/kf6_org.freedesktop.ScreenSaver.xml:24–30`; [PowerDevil 6.8.2 fdoconnector.cpp:84–97](https://github.com/KDE/powerdevil/blob/v6.8.2/daemon/powerdevilfdoconnector.cpp#L84-L97), [policyagent.cpp:179–183](https://github.com/KDE/powerdevil/blob/v6.8.2/daemon/powerdevilpolicyagent.cpp#L179-L183) |
| F6 | KDE มี RegisterWatcher/UnRegisterWatcher; ส่งข้อมูลไป `/NotificationWatcher` interface `org.kde.NotificationWatcher` โดยไม่ต้องยึดชื่อ notification server | [Plasma 6.8.2 watchednotificationsmodel.cpp:29–79](https://github.com/KDE/plasma-workspace/blob/v6.8.2/libnotificationmanager/watchednotificationsmodel.cpp#L29-L79), [server_p.cpp:242–260](https://github.com/KDE/plasma-workspace/blob/v6.8.2/libnotificationmanager/server_p.cpp#L242-L260); read-only introspection ยืนยันติดตั้ง extension นี้จริง |
| F7 | Watcher รับ raw body และต้องจัดการ replace/close; ไม่ใช่ historical replay API | [watchednotificationsmodel.cpp:80–117](https://github.com/KDE/plasma-workspace/blob/v6.8.2/libnotificationmanager/watchednotificationsmodel.cpp#L80-L117), [server_p.cpp:242–260](https://github.com/KDE/plasma-workspace/blob/v6.8.2/libnotificationmanager/server_p.cpp#L242-L260) |
| F8 | ตรวจ sender context ได้จาก exported QObject; อย่า inherit QDBusContext ลง adaptor โดยตรง | [Qt 6.12 QDBusContext](https://doc.qt.io/qt-6/qdbuscontext.html) |
| F9 | QElapsedTimer ใช้ steady clock; บน Linux MONOTONIC กับ BOOTTIME มีพฤติกรรมช่วง suspend ต่างกัน | Qt 6.12 `/usr/include/qt6/QtCore/qelapsedtimer.h:27–43`; [Qt QElapsedTimer](https://doc.qt.io/qt-6/qelapsedtimer.html), [Linux clock_gettime(2), CLOCK_BOOTTIME](https://www.man7.org/linux/man-pages/man2/clock_gettime.2.html) |
| F10 | QFileSystemWatcher แจ้งเปลี่ยนไฟล์ ไม่ได้ให้ total bytes ของ download; KJobWidgets registerJob เป็นงานที่แอปเป็นเจ้าของ ไม่ใช่ observer ทั่วระบบ | [Qt 6.12 QFileSystemWatcher](https://doc.qt.io/qt-6/qfilesystemwatcher.html); `/usr/include/KF6/KJobWidgets/kuiserverv2jobtracker.h:45–49`; [Plasma 6.8.2 jobsmodel_p.cpp:120–177](https://github.com/KDE/plasma-workspace/blob/v6.8.2/libnotificationmanager/jobsmodel_p.cpp#L120-L177) |

**VERIFIED runtime introspection:** `org.freedesktop.Notifications.Inhibited`, ScreenSaver `GetActive`, `ActiveChanged`, KDE `AboutToLock` มีใน session นี้ แต่ **UNVERIFIED U3:** ความหมายและ race ของ lock/DND ใน watcher path ต้องทดลองด้วย fake/controlled session ก่อนเปิดรับเนื้อหาจริง

### คัดเลือกตามประโยชน์ ไม่คัดตามจำนวนฟีเจอร์

**DECISION; user-confirmed:** คะแนนต่อไปนี้เป็นดุลยพินิจการออกแบบ ไม่ใช่ benchmark

| ลำดับ | หยิบมาใช้ | คุณค่าบน notch | ขอบเขตที่รับ / ตัด |
|---|---|---|---|
| 1 | Music timeline | ผู้ใช้กำลังใช้งานอยู่แล้ว; ดูเวลา/seek ได้ทันที | local artwork + auto current; ตัด lyrics/queue/account/source picker |
| 2 | Audio/mic feedback | feedback ของปุ่มเสียง/ไมค์มองเห็นโดยไม่เปิด panel | default output/source mute; ตัด per-app mixer และคำอ้างว่า mute เป็น privacy lock |
| 3 | Focus/Pomodoro | countdown/completion อยู่ให้เห็นตลอด | session เดียว, explicit next phase; ไม่สร้าง alarm/calendar app |
| 4 | Keep awake | เปิดชั่วคราวและเห็นว่าลืมปิดหรือไม่ | screen/sleep แยกสถานะ, มีเวลาหมด; ไม่ override lid/thermal/user power policy |
| 5 | AI/command activity | งานยาวเสร็จหรือรอ input เห็นได้โดยไม่สลับหน้าต่าง | explicit local bridge; ตัด provider logs, tokens/cost, hidden authentication |
| 6 | Plasma notifications | เห็น badge/อ่านข้อความล่าสุดจาก notch เดิม | opt-in, memory-only, ให้ Plasma เป็นเจ้าของ; ตัด history sync/inline reply ในรุ่นนี้ |

**DECISION:** ทำ AI bridge ก่อน notification แม้ผู้ใช้ยืนยันทั้งคู่ เพราะ contract ที่เราเป็นเจ้าของทดสอบได้โดยไม่อ่านข้อมูลส่วนตัว; notification มี lock/DND gate เพิ่มเติม พัก calendar, downloads และ shelf ไว้นอก roadmap นี้ การแก้ scope ต้องเป็นแผนใหม่ ไม่แอบเพิ่มหลัง phase สุดท้าย

## 4. Audit findings

ข้อด้านล่างเป็น **VERIFIED source-level findings** ซึ่งมี acceptance tests ที่ต้องเพิ่มใน Phase 0 ไม่อ้างว่ารอบวิจัยนี้ reproduce ทุกข้อด้วย native input แล้ว

1. **P2 correctness — Hover-expanded view pin ไม่ได้ตาม copy:** compact click handler เท่านั้นที่ตั้ง `pinned=true`; handler นั้น invisible เมื่อ expanded แล้ว ขณะที่ข้อความยังบอกว่าคลิกค้างได้ ดู `qml/Main.qml:25–26`, `:38`, `:124–140`, `:301` ผลคือ hover → ใช้ control → ออกจากแถบยังย่อตาม timer; settings เป็นข้อยกเว้นเพราะ showSettings pins
2. **P2 reliability — Startup กลืน D-Bus error:** registerService ล้มเหลวแบบใดก็ unchecked Toggle แล้ว exit 0 (`src/main.cpp:51–55`) ไม่มีหลักฐานว่า instance อื่นตอบรับจริง; bus หายจึงกลายเป็นเปิดสำเร็จปลอม
3. **P3 reliability — ข้อความเดิมถูกล้างเร็ว:** `setMessage` ตั้ง singleShot ทุกครั้งและตรวจเพียงข้อความ; message เดิมที่ refresh ตอนวินาที6 จะโดน callback แรกเคลียร์ตอนวินาที7 (`src/backend.cpp:276–278`)
4. **P3 correctness — Foreground tint ไม่ส่งถึง custom image:** `foreground` เปลี่ยน icon.color แต่ Image ใช้เพียง source/opacity (`qml/IslandButton.qml:7–30`); สี lavender ที่ขอสำหรับปุ่ม play (`qml/Main.qml:207`) ไม่มีผลตาม contract
5. **Verification gap — private-bus fallback:** ถ้าไม่มี dbus-run-session CTest ใช้ bus ที่สืบทอดมา (`CMakeLists.txt:24–28`); ไม่ควรใช้เป็นหลักฐานว่า integration tests isolated เสมอ

ยังไม่พบหลักฐานยืนยัน code execution จาก media metadata: transport และ pactl ใช้ชื่อ method/argv คงที่ (`src/backend.cpp:153–164`, `:226–253`) อย่ายกระดับข้อสงสัยเป็น security finding โดยไม่มี reproduction

## 5. Corrections

- **CORRECTED:** “คืน click-to-pin แล้ว” ยังไม่ครอบคลุม hover-expanded content; handler coverage ใน §4.1 หักล้าง ต้องแก้ก่อนเพิ่มหลาย activity
- **CORRECTED:** ชื่อจอใน QWindow อย่างเดียวไม่พิสูจน์ monitor remap; source มี surface recreation (`src/backend.cpp:306–325`) และต้องมี native/physical gate U1 การดู JSON screen อย่างเดียวไม่พอ
- **CORRECTED:** baseline test count ในเอกสารเดิมแปด/สิบสองและรายงาน audit ย่อยหนึ่งฉบับไม่ใช่จำนวนปัจจุบัน รอบนี้อ่าน test methods และรันจริงได้ **13 cases, 15 total pass รวม lifecycle** (`tests/test_backend.cpp:38–247`)
- **CORRECTED:** freedesktop standard ไม่ได้ให้ passive notification feed ทั่วไป แต่ KDE 6.8.2 มี watcher extension จริง (F6) ดังนั้นไม่ปฏิเสธ feature นี้ว่าเป็นไปไม่ได้ และไม่ register `org.freedesktop.Notifications` เอง
- **CORRECTED:** การดู partial files ไม่ได้ให้เปอร์เซ็นต์ดาวน์โหลด (S6/F10); จึงตัด universal downloads ออก ไม่วาด progress สมมุติ
- **CORRECTED:** animated playback bars ไม่ใช่ audio spectrum; ของเดิมขยับตาม `playing` (`qml/Main.qml:103–110`) จึงไม่ใช้ข้ออ้างว่า equalizer ทำไว้แล้ว
- **CORRECTED:** “hover opt-in / compact244×32” ใน ledger เป็นประวัติคำขอเก่า; คำขอล่าสุดและ source ยืนยัน 288×40 + hover default true (`src/backend.h:44`, `qml/Main.qml:44–45`)

## 6. Gap analysis

| Gap | Class | Concrete mismatch / evidence |
|---|---|---|
| G1 | BROKEN | Click-to-pin, startup error handling, renewed message lifetime และ button tint ตาม §4 |
| G2 | BROKEN | Integration test fallback ไม่รับประกัน isolation; native drag/focus ยังไม่มี acceptance gate (§2, §4.5, U1) |
| G3 | MISSING | ทุก activity ใช้พื้นที่เดียวกันแต่ไม่มี event arbitration; ปัจจุบัน timer แทน music โดย hard-coded ternary (`qml/Main.qml:92`), เทียบ upstream S2 |
| G4 | MISSING | Media ไม่มี timeline/seek, Position/Rate/TrackId และ signal-driven updates (F1/F2 vs `src/backend.cpp:71–85`, `:88–136`) |
| G5 | MISSING | Audio มีแค่ output polling/control; ยังไม่มี source-mute state, subscription หรือ transient feedback (F4 vs `src/backend.cpp:192–253`) |
| G6 | MISSING | Focus มี countdown เดี่ยว ไม่เก็บ completed activity/Pomodoro phases (S4 vs `src/backend.cpp:255–274`) |
| G7 | MISSING | ยังไม่มี keep-awake cookie lifecycle/service owner handling (F5 vs current `src/backend.h:18–68`) |
| G8 | MISSING | ไม่มี explicit task-event bridge หรือ run/completed/attention state (S5 vs `src/main.cpp:20–29`) |
| G9 | MISSING | ยังไม่มี opt-in KDE watcher, privacy state และ bounded notification inbox (F6–F8 vs current dependencies `CMakeLists.txt:5–12`) |

ไม่ใส่ FIGHTS-THE-FRAMEWORK เพื่อทำให้ครบหมวด: polling ปัจจุบันเป็น implementation ทางเลือก ไม่ได้มีหลักฐานว่าใช้ API ผิด เพียงต้องปรับเพื่อรองรับ latency/จำนวน activity ใหม่

## 7. Design decisions

**Decision Brief gate — user-confirmed:** ผู้ใช้เลือก “ใช้ชุดนี้เขียนแผนเลย” หลังเห็น 6 ฟีเจอร์ ลำดับช่วง และข้อจำกัด opt-in ทุก decision ด้านล่างเป็น **REVERSIBLE** ไม่มี ONE-WAY schema migration, destructive cleanup หรือการแทน desktop daemon

### D1 / G1: แก้ฐานเดิมก่อน

- **Options:** แก้ source ที่มีอยู่ / เปลี่ยนทั้ง shell และ backend
- **DECISION; REVERSIBLE:** แก้เฉพาะ handler/error path/timer lifetime ใช้ QTimer เดี่ยว restart แทนการเทียบข้อความ ปุ่ม SVG ต้องแสดงได้ใน KDE style ต่อไป; เอา foreground API ที่ไม่มีผลออกและใช้สี SVG จริงอย่างสอดคล้องทั้ง source/copy/tests
- **Commitment:** สำหรับแผนนี้เลือก **เอา tint API ที่ไม่มีผลออก** และใช้ SVG ขาวตามที่เห็น ไม่เพิ่ม graphics dependency เพียงเพื่อปุ่มเดียว
- **Rationale/evidence:** โครงสร้างที่ต่อครบอยู่แล้ว (§2); โค้ดที่เสียมีขอบเขตชัด (§4) Reject rewrite เพราะเพิ่ม blast radius โดยไม่จำเป็น

### D2 / G2: private-bus tests และ physical gate เป็นเงื่อนไขส่งมอบ

- **Options:** ถือว่า offscreen ผ่านเพียงพอ / แยก pure UI/unit กับ private-bus integration และ native checklist
- **DECISION; REVERSIBLE:** เลือกแบบหลัง หากไม่มี dbus-run-session integration test ให้ skip พร้อมเหตุผล ห้าม fallback ไปคุม bus จริง ส่วน unit tests ยังรันได้ แก้ startup test ให้ปล่อย D-Bus ไปใน subprocess ที่ควบคุมได้ ใช้ U1 manual gate ก่อน release
- Reject global native-input injection scripts และการปรับเพลง/volume จริงเป็น test fixture ตาม `AGENTS.md:11`

### D3 / G3: ActivityCoordinator ขนาดเล็ก ไม่สร้าง plugin framework

- **Options:** เพิ่ม if/else ลง Main.qml ทุกฟีเจอร์ / coordinator เดี่ยวที่กำหนด priority, dedupe, expiry
- **DECISION; REVERSIBLE:** เพิ่ม `src/activitycoordinator.{h,cpp}` แบบ QObject, methods `upsertActivity`, `removeActivity`, `showNotice`, `dismissNotice`, clock injectable; Q_PROPERTY `activities`, `selectedActivity`, `notice` เป็น QVariantList/Map; คง Backend เป็น facade สำหรับ QML
- **Contract:** activity มี key/kind/title/detail/state; notice มี key/kind/priority/payload/expiresAt; ห้ามนำ arbitrary payload มาเรียก method หรือ shell มี persistent activity ไม่เกิน8, transient queue ไม่เกิน8, expired ทิ้ง Event key ซ้ำ update ค่าและ reset TTL โดยไม่เริ่ม resize animation ใหม่
- **Priority:** timer complete / task attention=40, direct audio/mic feedback=30, task complete/failure=20, notification badge/track change=10; ภายใน priority ใช้ล่าสุดก่อน Stable selected activity ไม่สลับเองเพราะ polling เฉย ๆ การเริ่ม Focus โดยผู้ใช้เลือก Focus เป็น compact activity จนผู้ใช้สลับเองหรือจบ session; มี indicator สลับกิจกรรมได้โดยไม่มี player dropdown
- **UX:** transient ไม่ force-expand, hover ยังคง220ms, leave800ms, motion280msตามเดิม Notice จบกลับ activity เดิม ใช้ `qml/ActivityStrip.qml` และ `qml/NoticeView.qml` แสดงทีละเรื่อง ไม่เพิ่มแถว tab ยาว6อัน: คง Music/Focus/System และเปิด context panel จาก activity indicator; settings เป็นที่เปิด/ปิดฟีเจอร์
- Reject event bus/plugin loader/database เพราะยังเป็น single desktop process; รูปแบบ priority อ้างแนวคิด S2 ไม่ copy Swift

### D4 / G4: MPRIS event updates + capability-aware timeline

- **Options:** poll GetAll ทุก1.5sต่อไป / subscribe properties/Seeked/owner แล้วมี fallback reconciliation
- **DECISION; REVERSIBLE:** เพิ่ม `src/mediaservice.{h,cpp}` ค่อยย้ายโค้ด media จาก Backend โดยรักษา `currentMedia()` และ QML-facing shape เดิม Signals ใช้ F2; discovery เริ่มต้นและเมื่อ owner เปลี่ยน; full reconciliation ทุก10sเป็น safety net
- **Contract:** เพิ่ม `trackId`, `durationUs`, `positionUs`, `sampledTick`, `rate`, `canSeek`; update selected source แล้ว sample Position ก่อน render timeline; extrapolateเฉพาะ playing จาก monotonic anchor refresh progress UI1Hzขณะมองเห็น แสดง read-only progress เมื่อ CanSeek=false; ถ้า durationไม่รู้ให้ซ่อน timeline ไม่แสดง0%ปลอม
- Drag scrub จับ `{service,trackId,generation}` ณ press; release ส่ง SetPosition เพียงครั้งเดียวเมื่อ contextยังตรงและ CanSeek/CanControlจริง; clamp0..duration; track/serviceเปลี่ยนระหว่างลากต้องยกเลิก pending seek timeout1.5sแล้วคืน observed position ไม่มีsource dropdown ไม่มีremote artwork (F1)
- Reject lyrics/queue/shuffleรอบนี้เพราะไม่ใช่ user need ที่ยืนยัน และเพิ่ม API/data surface

### D5 / G5: ยึด audio backend เดิม เพิ่ม observer ไม่เปลี่ยน audio stack

- **Options:** เปลี่ยนเป็น PulseAudioQt ทันที / reuse pactl adapter และเพิ่ม long-lived subscription
- **DECISION; REVERSIBLE:** เพิ่ม `src/audioservice.{h,cpp}` ย้าย queue/pollเดิมอย่างรักษาพฤติกรรม เพิ่มหนึ่ง `pactl subscribe` process โดย LC_ALL=C, invalidate queryเฉพาะ sink/source/server events, debounce100ms; fallback snapshot10s, restart backoff1/2/5/15s; capอ่าน event buffer64KiB
- Source microphone ใช้ `get-source-mute @DEFAULT_SOURCE@` และ `set-source-mute @DEFAULT_SOURCE@ 0|1` เฉพาะ explicit click เก็บ confirmed stateหลัง query ไม่ optimisticว่า mute สำเร็จ Audio change จากภายนอกแสดง compact OSD1.6sและ coalesce burst; ปุ่มเสียงภายในไม่ยิงซ้ำสอง notice
- **Semantics:** label “Default microphone muted” ไม่ใช่ “ไม่มีแอปฟังได้”; ไม่ทำ recording/speech detector ไม่แตะ per-app routing ไม่เปลี่ยนvolumeจริงระหว่าง tests (F4)
- Reject new native audio dependencyรอบนี้เพราะของเดิมมี queue/tests ที่ใช้ได้และเป้าหมายแคบกว่านั้น; ไม่ใช้loopเร็วเพื่อเลียนแบบevent

### D6 / G6: Pomodoro session เดียว และ completion ต้องรอ dismiss

- **Options:** timerหลายตัวพร้อมกัน / timerหรือPomodoro sessionเดียว
- **DECISION; REVERSIBLE:** เพิ่ม `src/focussession.{h,cpp}` แบบ state machine `Idle/Running/Paused/Completed`, mode Countdown/Pomodoro, phase Work/ShortBreak/LongBreak ค่าเริ่ม25/5/15นาที, long breakทุก4work phases; custom durations1–180นาที validated
- Completed คง activityจนDismissหรือStart next; next phaseต้องกดเอง ไม่เดินหลายรอบระหว่าง sleep Sessionเป็นmemory-only, restart appแล้วIdle, ไม่persist deadlineข้ามboot ใช้ injectable Linux CLOCK_BOOTTIME millisecondsสำหรับelapsedรวมsuspend โดยไม่ผูกกับwallclock; non-Linux fallback steady clockมีlabel/ทดสอบแยก ไม่ใช้ QTimer tick decrement (F9)
- completion1ครั้งต่อphase, ไม่auto-pinแถบใหญ่จาก timerอีก; ส่ง noticeผ่านcoordinator และ system notificationเฉพาะตั้งค่าอนุญาต
- Reject history/stats databaseและwake alarms เพราะเกินsessiontoolที่ต้องการ

### D7 / G7: Keep awake เป็น explicit leased inhibition

- **Options:** เรียก systemd-inhibit childprocess / ถือสอง D-Bus cookiesในconnectionเดิม
- **DECISION; REVERSIBLE:** เพิ่ม `src/keepawakeservice.{h,cpp}` ใช้ F5; ScreenSaverที่ `/ScreenSaver` และ PowerManagementที่ `/org/freedesktop/PowerManagement/Inhibit` ใช้ interfaceตามF5; application=Kestrel Island, reasonตามuseraction
- UI15/30/60นาทีหรือUntil off; defaultOff, apprestartไม่เปิดเอง `Requesting/Active/Releasing/Error` ระบุ screen/sleep acknowledgementแยก หากcookieที่สองล้มเหลว releaseอันแรกแล้วแสดงError ห้ามแสดงActiveปลอม UnInhibitส่งuint32; watcherจับ daemon owner lossล้างcookieเก่าและแสดงstateสูญเสียการยืนยัน ไม่re-enableเองเงียบ ๆ
- Exit/Cancel/expiry releaseทั้งสองcookie; retries bounded3ครั้งแล้วแจ้งวิธีquitเพื่อปล่อยconnection ไม่overridelid/thermal/forcedsleeppolicy Focusอาจมี “Keep awake during this session” opt-in ซึ่งเริ่ม/จบตามsessionเท่านั้น (F5)
- Reject launching sleep childเพื่อซ่อนcookie lifecycle; ทดสอบpartial failureได้ชัดกว่าด้วยmockbus

### D8 / G8: Task bridge ที่ไม่อ่าน AI logs

- **Options:** parse provider private logs / explicit versioned local events
- **DECISION; REVERSIBLE:** เพิ่ม `src/taskactivityservice.{h,cpp}`, `src/islandcontrol.{h,cpp}` และ `scripts/kestrel-task` ภายหลังในPhase6 แทน adaptor เดิมด้วย `IslandControl : QObject, public QDBusContext` มี scriptable slots Toggle() และ ReportTask(QVariantMap)→bool; register exported QObject ด้วย ExportScriptableSlots บน `/Island`, interface `io.github.kestrel.Island` รักษา Toggle contract เดิม CLI `--task-event-file PATH` อ่านJSON≤16KiBแล้วส่งไปinstanceที่รันอยู่ ถ้าไม่มีinstanceหรือfeatureปิดให้exitnonzero ไม่toggleไม่autostart
- Schema v1: `version:1`, `id`1–128ASCIIอักษร/เลข/._:-, `state`running/completed/failed/attention, `label`plain text≤160ตัวอักษร; optional `project`basename≤80, `seq`uint64 ถ้ามีต้องเพิ่มขึ้น ตัดcontrol chars, unknownfieldsreject ไม่มีstdout/prompt/token/secret field
- Runningเริ่มelapsed, heartbeat15s; ไม่มีevent120s→stale/unknown ไม่อนุมานCompleted; closedterminal/terminatedwrapperต้องรายงานfailedเมื่อทำได้ Bound8activities, rate20events/sต่อsender; completed/failed หมดอายุหลัง60sหรือถูกDismissก่อนหน้านั้น ส่วนattentionคงจนDismiss หากมี8taskrecordsแล้วให้rejectidใหม่จนมีช่องว่าง ไม่ทิ้งrunning/attentionเงียบ ๆ notificationตอนจบเฉพาะงาน≥60sโดยdefault
- featuredefaultOff ไม่เขียนเนื้อหาลงdisk; wrapperส่งargvตรงและpreserveexitcodeกับsignal behaviorตามU4 ไม่ใช้interpolatedshellcommand ไม่อ้างว่าhookนี้เข้าใจทุกCodex/Claudeappโดยอัตโนมัติ ใช้ F2/F8 ตรวจcallerเมื่อ export QObject จริง; same-user busไม่ถือเป็นsecuritysandbox
- Reject tokens/cost/limits/reset/accountและprovider adaptersจนมีงานวิจัยเฉพาะ

### D9 / G9: KDE watcher, opt-in, fail closed

- **Options:** ยึดnotificationdaemon/อ่านhistorydatabase / KDE watcher capability-gated / งดfeature
- **DECISION; REVERSIBLE:** ใช้KDEwatcherตามF6เมื่อผ่านU3; เพิ่ม `src/notificationsservice.{h,cpp}`, `src/sessionstate.{h,cpp}`, `qml/NotificationsView.qml` ไม่link/initialize NotificationManager::Server และไม่registerชื่อorg.freedesktop.Notifications
- ลงทะเบียนexported QObjectที่ `/NotificationWatcher`, interface `org.kde.NotificationWatcher`; methods `Notify(uint id,QString app,uint replaces,QString icon,QString summary,QString body,QStringList actions,QVariantMap hints,int timeout)` และ `CloseNotification(uint id)`; ใช้ QDBusContextในQObjectตรวจsenderตรงกับuniqueownerของPlasma server ไม่ใช้contextบนadaptor (F6–F8)
- Initial session lock/DNDเป็นunknown→suppress; subscribe GetActive/ActiveChanged/AboutToLock และ Notifications.Inhibited/PropertiesChanged รับเนื้อหาจริงต่อเมื่อ unlockedและnotinhibitedยืนยันแล้ว Notification featuredefaultOff และuserappallowlistdefaultempty; เปิดfeatureแล้วยังต้องเลือกappก่อน render
- เริ่ม RegisterWatcher ต่อเมื่อ feature เปิดและ allowlist ไม่ว่างเท่านั้น UI ต้องอธิบายว่า KDE ส่งทุกแอปมายัง watcher แล้วเรา filter ก่อนแสดง; allowlist นี้ไม่ใช่สิทธิ์อ่านแบบ server-side ไม่บันทึก notification ที่ถูกกรองและไม่เปิด historical store
- Inboxmemory-only≤20รายการ, TTL10นาที, summary≤160/body≤512plain text, rejectpayload>64KiB; ไม่โหลดexternalimages/markuplinks ไม่persist เมื่อlock/DND/disablingให้clearเนื้อหา, unregisterwatcher; unlockต้องresyncstateแล้วregisterใหม่ ไม่replayhistory
- Eventเพียงเพิ่มcompactbadge ไม่แสดงpopupซ้อนPlasma ไม่force-expand; hoverจึงอ่านได้ Clearในislandไม่dismissที่Plasma ไม่มีinline replyหรือInvokeActionในรุ่นนี้ Ownerchangeต้องunregister/ล้างownerเก่าและcapability-checkใหม่ Closed/replacedidsต้องไม่ค้าง
- หากU3พิสูจน์lock/DND suppressionไม่ได้ ให้featureunavailable, เก็บmusic/audio/focus/keepawake/taskที่ผ่านไว้ ไม่ใช้BecomeMonitor/eavesdropแทน

## 8. Phased roadmap

**DECISION — estimates:** S≈0.5–1 engineer-day, M≈1–3, L≈3–5 เป็นประมาณงานไม่ใช่ผลวัดหรือdeadline รวมค่าต่ำสุด–สูงสุดของแต่ละphaseเป็น **11–23 engineer-days** รวมmanualverificationตามเครื่องจริง รุ่นแรก Phase0–2ประมาณ4–7วัน ไม่รอครบหกฟีเจอร์ ไม่มีการdeployอัตโนมัติในรอบวางแผนนี้

### Phase 0: Shippable correctness and verification baseline (S–M, 1–2 days)

- **Objective:** แก้G1/G2โดยไม่เพิ่มfeature
- **Depends on:** nothing
- **Entry criteria:** ยืนยันsource/preferencebaselineตรง§2; ตรวจว่าไม่มีงานผู้ใช้ที่ต้องpreserveก่อนrestart
- **Tasks:**
  - [ ] **Spike U0 — 15min:** ทำinventory/checksum baselineและตรวจhistoryอีกครั้ง; หากยังไม่มีcommitsให้บันทึกsnapshotใน`docs/VALIDATION.md`และเก็บสำเนาsourceนอกprojectก่อนimplementation ไม่สร้างcommits/stageuserfilesโดยปริยาย
  - [ ] **Spike U1 — 60min:** ตรวจdragDP-2↔DP-1, Escape cancel, hotplug, คืนtypingหลังcollapse, maskระหว่างanimationด้วยมือ; บันทึกขั้นตอน/ผลใน`docs/VALIDATION.md` หากdragไม่ผ่านปิดdraggestureชั่วคราวและคงSettings→Displayที่verifyแล้ว; releaseที่อ้างnative interactionต้องผ่านgateนี้
  - [ ] `qml/Main.qml`: ให้pointerinteractionในexpandedcontent pinได้โดยไม่แย่งgestureของSlider/DropAreaและไม่ให้Collapseกลับไปpin; เพิ่มQtQuickinteractiontestใน`tests/test_backend.cpp`
  - [ ] `src/main.cpp`: แยกbusdisconnected/registrationfailedจากexistingowner ตรวจreplyToggleมีtimeoutและexitnonzeroเมื่อส่งไม่ได้; subprocess testsใน`tests/test_startup.cpp`
  - [ ] `src/backend.{h,cpp}`: ใช้messageexpiryQTimerเดี่ยวที่restartทุกmessage; เพิ่มdeterministictestไม่รอ7sจริง; `qml/IslandButton.qml`/`qml/Main.qml`เอาforegroundproperty/callsiteที่ไม่ทำงานออก
  - [ ] `CMakeLists.txt`: ให้privatebusintegrationrequiredสำหรับintegrationcases, puretestsรันแยกได้; ห้ามfallbackbusจริง `docs/ARCHITECTURE.md`ปรับcount/hoverเอกสารเก่าโดยระบุhistoricalledger
- **Exit criteria:** testsbaselineทั้งหมดผ่านพร้อมregressions4ข้อ; startupfaultมีnonzeroexit; nativegateผ่านหรือgestureที่ไม่ผ่านถูกปิดพร้อมเหตุผล
- **Rollback:** ติดตั้งbinarybaselineกลับ; ไม่มีsettingsschema/dataที่ลบ; ไม่ย้อนแพตช์monitor/iconที่ผู้ใช้ยืนยันแล้ว

### Phase 1: Walking skeleton — one media activity through the coordinator (M, 1–2 days)

- **Objective:** end-to-end sliceจริงจากMPRIS→coordinator→compact/hoverแล้วกลับได้
- **Depends on:** Phase0
- **Entry criteria:** 288×40/hover/privatebusbaselineผ่าน
- **Tasks:**
  - [ ] สร้าง `src/activitycoordinator.{h,cpp}`, `qml/ActivityStrip.qml`, `qml/NoticeView.qml`, `tests/test_activity.cpp`; wireใน`src/backend.{h,cpp}`, `qml/Main.qml`, resource/targetsที่`CMakeLists.txt`
  - [ ] นำเฉพาะcurrentmediaเป็นfirstactivity; outputvolumechangeหนึ่งชนิดเป็นfirstnotice ทดสอบTTL/coalesce/priority/return-to-previousด้วยfakeclock
  - [ ] จำกัดrecords8+8และpayloadknownkeysตามD3; `NoticeView.qml`ใช้PlainText ไม่renderarbitrarymarkup
  - [ ] เพิ่มper-featureflagsในQSettingsผ่านSettings; existingmusic/focusยังenabled ไม่เปิดopt-infeaturesล่วงหน้า
- **Exit criteria:** runinstalledbinaryแล้วเล่น/หยุดfakeMPRISผ่านcoordinatorได้; noticeburst50eventsเหลือkeyเดียวและไม่resizeburst; ปิดnoticesแล้วbehaviorเดิมยังใช้ได้
- **Rollback:** ปิดfeatureflagsและใช้media/directviewเดิมโดยเก็บfacade; no data migration

### Phase 2: Rich current media (M, 2–3 days) — first release milestone

- **Objective:** automaticmusicพร้อมtimelineโดยไม่คืนsourcepicker
- **Depends on:** Phase1
- **Entry criteria:** coalescing/focus/maskbaselineผ่าน
- **Tasks:**
  - [ ] สร้าง `src/mediaservice.{h,cpp}`, `qml/MediaView.qml`, `tests/test_media.cpp`; ย้ายmediaflowออกจากBackendทีละส่วนและwireผ่านfacade/CMake
  - [ ] F1/F2: subscribeowner/properties/Seeked, reconcile10s, sample/extrapolateposition, guardRate/duration/CanSeek/TrackIdตามD4
  - [ ] ขยายfakeMPRISในtestsให้มีPosition,Rate,MetadataTrackId,CanSeek,SetPositionและsignals; ทดสอบnativebrowser/Plasmaduplicate, reversedreplyorder, disappear/restart, trackchangeduringscrub, delayedseekreply
  - [ ] `qml/MediaView.qml`: localart/title/artist/transport/timeline; unknownlengthซ่อนseek; ไม่มีplayeridentifier/dropdown; updateofflinepreviewและ`README.md`
- **Exit criteria:** click/seekcapabilitytestsผ่าน; stalecontextส่งSetPositionไม่ได้; signalsเปลี่ยนtrackในUIภายใน500msในmocktest; manualnativebrowserselectionไม่สลับเองเมื่อmetadataไม่เปลี่ยน
- **Rollback:** ถอดseekและกลับsnapshotadapterเดิมหลังfacade; compact/hover/layoutไม่เปลี่ยน

### Phase 3: Context audio and microphone feedback (M, 1–3 days)

- **Objective:** output/source muteที่ยืนยันสถานะจริง และcompactOSD
- **Depends on:** Phase1; deliverหลังPhase2ตามลำดับrelease
- **Entry criteria:** ActivityCoordinatorผ่านและfeaturetoggleพร้อม
- **Tasks:**
  - [ ] สร้าง `src/audioservice.{h,cpp}`, `qml/AudioView.qml`, `tests/test_audio.cpp`; reusecoalescedqueueไม่เขียนใหม่เป็นparallelcommands
  - [ ] implementpactlsubscription/parserbounded/restartbackoffและsourcequeriesตามD5; โหลดdefaultsourceใหม่เมื่อserverchange; detachobserverเมื่อfeatureปิด
  - [ ] fakepactlรองรับeventbursts, timeout,missingbinary,serverrestart,defaultdevicechangeและdeniedcommand; ตรวจว่าค่าท้ายsliderไม่หาย
  - [ ] แสดงconfirmedmuteและexplicitmicbuttonในAudioView; plainlabelจำกัดความหมาย; sourceไม่มีให้ซ่อนcontrolโดยไม่กระทบเพลง
- **Exit criteria:** eventburstไม่เพิ่มprocessไม่จำกัด; fallback/recoveryและsnapshotstateตรงfake; realmanualปุ่มเสียง/ไมค์ตรวจหลังtestแยกโดยผู้ใช้ยินยอม ไม่ใช่autotest
- **Rollback:** disableOSD/micfeature, reuseoutputvolumeเดิม; ไม่แก้KDEglobalOSDsettings

### Phase 4: Focus/Pomodoro and durable completion UI (M, 1–3 days)

- **Objective:** sessionphaseครบและไม่ทำcompletionหาย
- **Depends on:** Phase1; releaseหลังPhase3
- **Entry criteria:** noticeexpiryและactivitypriorityผ่าน
- **Tasks:**
  - [ ] สร้าง `src/focussession.{h,cpp}`, `src/sessionclock.{h,cpp}`, `qml/FocusView.qml`, `tests/test_focus.cpp`; wrapexistingBackendtimerAPIชั่วคราวเพื่อให้oldtestsทำงาน
  - [ ] ใช้BOOTTIMEelapsedและinjectableclockตามD6; validatecustomduration; testsdelayedticks,pausefractions,simulatedsleep,clockjump,completedexactlyonce
  - [ ] Pomodoro25/5/15/4, explicitStartnext, completedpersistinmemoryuntilDismiss, UIมีPause/Resume/Cancel/Next; ไม่มีautostartbreak
  - [ ] `qml/Main.qml`timerfinishedไม่forcepin/expandอีก; ส่งผ่านActivityCoordinator; systemnotificationเคารพuserfeatureflag
- **Exit criteria:** sleep/timejumpmockไม่วนหลายPomodorophases; completionครั้งเดียวและยังเห็นหลังnoticeTTL; restartappIdleตามdocumentedcontract
- **Rollback:** disablePomodoromode, preservetimerpresets; memoryonlyไม่มีdataย้อนย้าย

### Phase 5: Timed keep awake (M, 1–2 days)

- **Objective:** explicitkeepawakeพร้อมtruthfulleaseownership
- **Depends on:** Phase1; optionalfocuslinkใช้Phase4
- **Entry criteria:** opt-inUIพร้อม; ไม่มีcookieหลงจากtest
- **Tasks:**
  - [ ] **Spike U2 — 90min:** `tests/test_keepawake.cpp`ใช้privatebusfakeScreenSaver/PowerManagementพิสูจน์uintcookies, partialfailure,serviceownerlossและdisconnectcleanup; realopt-incheckแยกภายหลังแล้วreleaseทันที บันทึก`docs/VALIDATION.md`
  - [ ] สร้าง `src/keepawakeservice.{h,cpp}`, `qml/KeepAwakeView.qml`; wireD7statesและ15/30/60/Untiloff, remaining/stop compactactivity
  - [ ] หากU2พบpolicyrejectไม่retryเป็นActive; ถ้าinterfacesไม่ครบแสดงUnavailableและไม่เก็บhalflease; focuscheckboxdefaultOff
- **Exit criteria:** stop/expiry/normalquit/daemonlossและsecondcookiefailureไม่เหลือhalfleaseในfake; manualactiveindicatorตรงKDEinhibitionstateและกลับปกติหลังquit
- **Rollback:** toggleOffและreleaseทุกcookieก่อนย้ายbinary; apprestartไม่autoenable

### Phase 6: Explicit AI/command activity bridge (M, 1–3 days)

- **Objective:** งานยาว/needsattention/completeผ่านhookที่เปิดเอง ไม่มีprivate-logreader
- **Depends on:** Phase1; releaseหลังPhase5
- **Entry criteria:** taskfeaturedefaultOffและpayloadlimitsออกแบบตามD8
- **Tasks:**
  - [ ] **Spike U4 — 90min:** `tests/test_tasks.cpp`ส่งrunning/heartbeat/completed/failed/attentionผ่านprivatebusและทดสอบwrappernormalexit/nonzero/signal; หากtoolไม่มีhookใช้documentedwrapperได้ แต่ห้ามโฆษณาnativeCodexdesktopauto-detection บันทึกmatrixใน`docs/INTEGRATIONS.md`
  - [ ] เพิ่ม `src/taskactivityservice.{h,cpp}`, `src/islandcontrol.{h,cpp}`; ย้ายToggleadaptorรักษาชื่อเดิมและReportTaskตามD8; branchCLI`--task-event-file`ก่อนsingle-instanceToggleใน`src/main.cpp`
  - [ ] สร้าง `scripts/kestrel-task`ที่forwardargvและexitcodeตรง ส่งeventsโดยไม่ขโมยstdout/stderr; ไม่includecommandline/promptในlabelโดยอัตโนมัติ
  - [ ] `qml/TasksView.qml`แสดงlabel/elapsed/status8itemsสูงสุด; staleไม่ถือว่าcompleted; sender/payload/rate/schema validationtests; disabledfeatureต้องrejectevents
- **Exit criteria:** wrappermockpreserveexitcode, canceled/stalestatusจริง, maliciouspayloadไม่เรียกshell/เปิดURL, restartไม่replaycontent; docsระบุวิธีopt-inและข้อจำกัดชัด
- **Rollback:** disabletaskfeature, adapterrejectevents; removeuser-installedhooksเมื่อผู้ใช้เลือกเอง ไม่แก้third-partyconfigอัตโนมัติ

### Phase 7: Opt-in Plasma notification inbox (L, 3–5 days)

- **Objective:** session-onlynotificationbadge/inboxที่ไม่รบกวนPlasmaและไม่เผยข้อมูลตอนlock/DND
- **Depends on:** Phase1; releaseหลังPhase6
- **Entry criteria:** contentprivacyfiltersและownerwatcherมีtestsแล้วก่อนregisterกับrealserver
- **Tasks:**
  - [ ] **Spike U3 — 2h:** `tests/test_notifications.cpp`/`tests/test_sessionstate.cpp`จำลองRegisterWatcher, rawNotify,close/replace,lock/DNDunknown/transitions,daemonrestartและspoofedsender; หลังผ่านทำcontrolledmanualcheckกับnotificationทดสอบเท่านั้น หากunknownstateมีแม้ช่วงหนึ่งที่renderได้ ให้featureUnavailableจนแก้ผ่าน ห้ามใช้fallbackeavesdrop
  - [ ] สร้าง `src/notificationsservice.{h,cpp}`, `src/sessionstate.{h,cpp}`, `qml/NotificationsView.qml`; wirewatcherF6/F7พร้อมsendercheckF8และD9limits
  - [ ] Settingsเพิ่มoff-by-defaultenable/appallowlist/clear ใช้QSettingsเก็บเฉพาะpreferencesไม่เก็บmessage; listenlock/DNDก่อนregisterwatcher; รักษาPlasmaownerเดิมเสมอ
  - [ ] ทดสอบ20recordcap,TTL,64KiBcap,escapedmarkup,allimagesignored,replacement,close,disable,serverownerchange; allowlistemptyต้องไม่renderbodyแม้รับevent
- **Exit criteria:** notificationtestไม่เปลี่ยนownerorg.freedesktop.Notifications; lock/DND/disableล้างและหยุดรับ; unlockไม่มีhistoricalreplay; ไม่มีpopupซ้อนหรือhistorydatabaseใหม่
- **Rollback:** unregisterwatcher, clearinmemory, flagOff; ไม่ต้องคืนnotificationdaemonเพราะไม่เคยยึดownership

### Unknown-to-spike closure matrix

| Unknown | First dependent phase / task | หลักฐานปิด / ถ้าไม่ผ่าน |
|---|---|---|
| U0 ไม่มีgittrajectory | Phase0 baseline15min | ไม่มีhistoryให้สร้างย้อนขึ้นมา; ลงtimestamp/checksumและสำเนาก่อนเริ่ม แล้วใช้sourceevidenceต่อไป |
| U1 physicaldrag/focus/hotplug | Phase0 manual60min | logขั้นตอนพร้อมผลจริง; ถ้าdragไม่ผ่านปิดgestureและคงdisplaysettingsที่ผ่าน; focus/maskเสียถือว่าreleaseblocked |
| U2 actualinhibitionlifecycle/policy | Phase5 spike90min | mocksทั้งfailure/disconnectและexplicitmanualrelease; interface/policyไม่ผ่าน→Unavailableไม่claimActive |
| U3 watcherlock/DNDbehavior | Phase7 spike2h | privacytestsและcontrolledmanualproof; failclosed/featureunavailableจนผ่าน |
| U4 providerhook/wrapperlifecycle | Phase6 spike90min | owncontract/wrapperผ่าน; nativeCodex/Claudeintegrationที่ไม่มีevidenceยังoutofscope ไม่parseprivatefilesแทน |

## 9. Risk register

Pre-mortem — สมมุติผ่านไป3เดือนแล้วผู้ใช้เลิกใช้: เหตุหลักที่ต้องป้องกันคือแถบแย่งความสนใจ, สิทธิ์ข้อมูล/keepawakeค้าง, และคำว่า“current”ไม่ตรงกับสิ่งที่กำลังใช้

| Risk | Likelihood | Impact | Mitigation / rollback |
|---|---|---|---|
| Audio bursts, timer completion และงานAIแย่งmusicจนnotchขยายตลอด | medium | high UX | D3priority/dedupe/TTL/caps; ห้ามforce-expandจากtransient; replaybursttests; ปิดproviderflagทีละตัวโดยmediaยังใช้ได้ |
| RawnotificationมาหลังlocksignalหรือDNDเปลี่ยนแล้วbodyค้าง | medium | high privacy | initialunknownsuppress, lock/DNDclear+unregister, sender/ownercheck, plaintextcaps; U3gate; unregisterและflagOffทันทีหากproofไม่ครบ |
| Keepawakeได้screenleaseแต่sleepleasefailหรือdaemonrestartทำindicatorโกหก | medium | high power/battery | twocookieatomicintent/rollback, Activeหลังacksครบ, ownerlossclearsconfirmedstate, explicitre-enable; U2tests; quitconnectionเพื่อreleaseท้ายสุด |
| BrowserbridgeกับnativeMPRISส่งtrackช้าต่างกัน, seekไปโดนเพลงใหม่ | medium | high correctness | คงautomaticrankingและcontextgeneration; cancelonsource/trackchange, capabilityguard, seektimeout; ถอดseekได้โดยtransport/basicmusicยังทำงาน |
| Third-partyAIhookเปลี่ยนหรือterminalหายแล้วแสดงWorkingไม่จบ | medium | medium | ownv1contract, heartbeat/staleunknown, explicitwrappersเท่านั้น; ไม่มีproviderlogs; disablebridgeและล้างmemoryไม่กระทบfeatureอื่น |

## 10. Verification

### Baseline ที่ทำแล้วในรอบวางแผน — VERIFIED

```sh
cd /home/dit/Sites/Labs/Vorssaint-KDE
dbus-run-session -- ./build/test-backend -platform offscreen
```

ผลรอบนี้: **15 passed, 0 failed, 0 skipped** รวมinit/cleanup; ไม่เปลี่ยนmediaที่ผู้ใช้เล่น ไม่มีการrestartแอปจริงในรอบวิจัยนี้ แผนฉบับนี้เป็นไฟล์projectเพียงไฟล์เดียวที่เขียนในรอบdeep-plan

### Automated commands ระหว่าง implementation — DECISION

```sh
cd /home/dit/Sites/Labs/Vorssaint-KDE
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=ON
cmake --build build -j 4
ctest --test-dir build --output-on-failure
QT_QUICK_CONTROLS_STYLE=org.kde.desktop ctest --test-dir build --output-on-failure
```

**Pass contract:** ไม่มีfailedcases; criticalintegrationcasesไม่skipบนเครื่องreleaseที่มีdbus-run-session; เมื่อไม่มีprivatebusเครื่องอื่นให้skipเฉพาะintegrationโดยชัดเจน ห้ามใช้realbusแทน `CMakeLists.txt`แต่ละPhaseต้องregistertestsที่สร้างใหม่ให้ctestครอบคลุม ไม่เพียงcompilebinaryทิ้งไว้

Phase0เพิ่มstartupfailures, renewedmessageexpiryและhover-pin tests; Phase1เพิ่มfakeclocknotice tests; Phase2เพิ่มprivateMPRISposition/seek/owner/latency tests; Phase3fakepactlทั้งqueueและsubscription; Phase4fakeBOOTTIME; Phase5fakecookies; Phase6tasktransport/payload/wrapper; Phase7fakewatcher/sessionprivacy นี่เป็นexitcriteriaที่ต้องสร้าง ไม่ใช่ผลที่อ้างว่าทดสอบผ่านแล้ว

**Offline visual QA:**

```sh
QT_QPA_PLATFORM=offscreen QT_QUICK_BACKEND=software QT_QUICK_CONTROLS_STYLE=org.kde.desktop \
  ./build/kestrel-island --screenshot /tmp/kestrel-plan-preview --expanded
```

เปิดPNGตรวจicons, Thai/longtitle, disabled/empty/errorstatesและfocusring ถ้าระบบprimaryappยังทำงานอยู่อย่าใช้`--check-session`แล้วตีความexit0เป็นsessiontest เพราะexistinginstancepathอาจเพียงToggle (แก้ตามPhase0ก่อน)

### Manual E2E release checklist — DECISION

1. Quitinstanceเดิมจากtrayโดยคงค่าผู้ใช้; เปิด`./build/kestrel-island --screen DP-2` ตรวจcompact288×40และhover220ms; เลื่อนออกย่อ, hoverแล้วclickcontrolค้างได้, Escapeคืนtypingไปแอปเดิม settingsgearยังมี
2. Settings→DisplayสลับDP-2/DP-1, ลากcompacttitleข้ามจอและEscapecancel; ตรวจตำแหน่งบนmonitorจริง ไม่เพียงQWindowname คลิกพื้นที่โปร่งใสรอบnotchต้องไปappข้างหลัง ระหว่างanimationด้วย Hotplugต้องกลับจอที่มีอยู่โดยไม่ทิ้งfullscreendroptarget
3. หลังquitinstanceเดิมแล้วใช้ `./build/kestrel-island --check-session --cycle-displays --screen DP-2` ตรวจexposed=true, compactmaskheight40, keyboard0 และsurfacecreationเปลี่ยนoutputในWaylandprotocol; ห้ามถือJSONscreenอย่างเดียวเป็นproof
4. ใช้mockMPRISสำหรับautotests; manualด้วยเพลงของผู้ใช้ต่อเมื่ออนุญาต ทดสอบplay/pause/title/artist/timeline/CanSeek=false/sourceหาย/trackเปลี่ยนตอนลาก ต้องไม่มีsourcepickerหรือbrowserชื่อหลุดเป็นศิลปิน
5. เปลี่ยนเสียงด้วยปุ่มhardwareโดยผู้ใช้: compactOSDจบใน1.6sและกลับmusic กดmicmuteแล้วstatusตรงdefaultsourceจริง; ไม่อ้างว่าหยุดทุกappฟังได้
6. Focus25→pause→resume→completed, พัก/กลับ; nextphaseต้องกดเอง Completedไม่หายก่อนDismissและไม่autoexpand หนึ่งphaseต้องแจ้งครั้งเดียว Suspend/resumeที่ควบคุมได้ต้องไม่วนหลายรอบ
7. Keepawake15min→cancel/quit: indicatorตรงสถานะKDEและหยุดหลังcancel ตรวจPowerDevilreject pathด้วยfakeไม่เปลี่ยนuserpolicyจริง
8. เปิดtaskfeatureเอง ส่งผ่านwrapperกับคำสั่งdemoไม่อ่อนไหว: running→completed, nonzero→failed, heartbeatหาย→stale ไม่completedเอง ตรวจexitcodeและstdout/stderrของcommandไม่ถูกกลืน
9. Notificationsปิดโดยdefaultและappallowlistว่าง เปิดเฉพาะdemoapp ส่งnotificationtestแล้วเห็นbadge1ตัว Plasmaยังทำงาน Lock/DNDล้างรายละเอียดทันทีและไม่replayหลังunlock; disableต้องunregisterและclearmemory ไม่มีข้อมูลเขียนลงdisk
10. Eventstorm/Thai/longlabels/reducedmotion/missingpactl/daemonrestart: ไม่growqueueหรือspawnprocessไม่จำกัด ไม่มีfocusgrabทั้งจอ วัดidleCPU/processcountเทียบbaselineเดิมอย่างน้อย60s; ถ้าidleCPUเพิ่มเกิน1percentagepointต่อเนื่องต้องtraceก่อนrelease ไม่อ้างperformanceจากoffscreenrender
11. ก่อนinstallreleaseให้savecopybinarybaselineที่ผู้ใช้ยอมรับ; `cmake --install build --prefix "$HOME/.local"` แล้วเปิดuserbinaryตรวจซ้ำ Phaseflagsต้องrollbackทีละproviderได้โดยnotch/music/hoverยังใช้ได้

### Deep Plan self-review

- [x] Phase0 framing: scope/rolloutถามและผู้ใช้ตอบแล้ว; inputs/RQs/non-goalsครบ
- [x] Phase1 inventory: sourcepaths, works/half-wired/dead, read/writeflow, operationalreality; nohistoryระบุU0
- [x] Phase2 groundtruth: upstreamcommit, Qtinstalledheaders, versionedMPRIS/KDEsources; unknownruntimeไม่ปนกับAPIverified
- [x] Phase3 adversarialaudit: defectfile:line, correctionsและข้อจำกัดการreproduceระบุชัด
- [x] Phase4 gaps: numbered/typed/evidence-linked ไม่มีข้อกว้างลอย ๆ
- [x] Phase5 decisions:หนึ่งcommitmentต่อgap, alternatives/rejections, ทั้งหมดREVERSIBLE, DecisionBriefผู้ใช้ยืนยัน
- [x] Phase6 roadmap: correctnessPhase0, walking-skeletonPhase1, phasesมีfiles/entry/exit/estimate/rollback, U0–U4มีfirstspikeและfailurepolicy
- [x] Phase7 review: commandsและmanualE2Eครบ; ไม่อ้างว่าplannedtestsผ่านแล้ว; implementerไม่ต้องเลือกarchitecture/API/providerเอง

หลังยอมรับแผนและเริ่มimplementation ค่อยใช้ `deep-plan-ingest` กระจาย verifiedknowledgeไป AGENTS.md / ARCHITECTURE.md / ROADMAP.md; รอบนี้ไม่แก้ไฟล์เหล่านั้นหรือโค้ดแอป

## Implementation follow-through, 2026-10-31

User authorized implementation. See [IMPLEMENTATION.md](IMPLEMENTATION.md) for the delivered modules, tests, deviations and still-open native-input/notification privacy gates. This planning snapshot is preserved; checked research phases above do not certify implementation acceptance.
