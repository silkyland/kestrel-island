# KDE slider hit-area fix (0.3.2)

Both media timeline and volume were drawn but could not receive mouse presses with org.kde.desktop. Each custom background/handle used an explicit visual height without an implicit height. KDE's Slider.qml computes control height from implicit background/content dimensions and padding; these customized controls therefore had height 0. Basic style supplied enough padding to hide the defect in earlier tests.

Reproduction: private D-Bus fake MPRIS + fake pactl, actual QTest mouse clicks and drags through Main.qml. The original controls passed under Basic and failed under org.kde.desktop. Geometry was mediaTimeline width 308, height 0, implicitHeight 0; volumeSlider width 292, height 0, implicitHeight 0. Debugger attach to the release app was blocked by kernel ptrace restrictions; no kernel setting was changed.

Differential: changing only the runtime implicit height to 32 made both controls pass under KDE, with the original parent TapHandler and backend unchanged. This ruled out parent gesture interception and backend transport as the cause of this reproduction. Read-only real pactl JSON queries were valid, and Plasma Browser Integration advertised CanSeek=true.

Fix: give both sliders an explicit implicitHeight of 28. Their thin tracks and handles remain centered inside the actual hit area. Existing capability gates, stale-seek generation checks and volume command coalescing are preserved.

Regression: pointerDragsReachMediaAndVolume uses actual clicks/drags and verifies the fake player's seek destination plus the final audio command. It asserts that each control has a nonzero usable hit area. A dedicated backend-kde-sliders CTest exercises org.kde.desktop in addition to the Basic suite. A local private-bus portal placeholder prevents desktop portal autoactivation during this test. All four CTest suites pass. Real media/audio settings were not modified.

Ground truth inspected: installed /usr/lib/qt6/qml/org/kde/desktop/Slider.qml, qml/MediaView.qml, qml/Main.qml and the failing/passing pointer regression.
