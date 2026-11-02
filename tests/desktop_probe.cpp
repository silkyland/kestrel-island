// Explicit live checks: never registered as an unattended CTest.
#include "notificationsservice.h"
#include "sessionstate.h"
#include <QApplication>
#include <QDBusConnection>
#include <QDBusConnectionInterface>
#include <QDBusContext>
#include <QDBusInterface>
#include <QDBusReply>
#include <QDBusVariant>
#include <QElapsedTimer>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLabel>
#include <QScreen>
#include <QTemporaryDir>
#include <QTimer>
#include <QUuid>
#include <QWindow>

class DesktopProbe : public QObject, protected QDBusContext {
  Q_OBJECT
  Q_CLASSINFO("D-Bus Interface", "io.github.kestrel.DesktopProbe")
public:
  QDBusConnection bus = QDBusConnection::sessionBus();
  QString service = "io.github.kestrel.DesktopProbe.p" +
                    QString::number(QCoreApplication::applicationPid());
  QString original;
  uint islandPid = 0;
  QTemporaryDir directory;
  QStringList scripts;
  SessionState state{this, false};
  QLabel window{"Fullscreen stacking check\nThis window closes automatically."};
  QJsonObject result;
  uint cookie = 0;
  bool quietSeen = false;
  bool manual = false;
  bool wasExpanded = false, haveExpansion = false;
  NotificationsService *inbox = nullptr;
  QList<uint> notificationIds;
  QString testApp, notificationOwner;
  QTimer notificationPoll;
  QElapsedTimer phaseClock;
  int notificationPhase = 0;
  void sendTestNotification() {
    QDBusInterface daemon("org.freedesktop.Notifications",
                          "/org/freedesktop/Notifications",
                          "org.freedesktop.Notifications", bus);
    daemon.setTimeout(1200);
    QDBusReply<uint> reply = daemon.call(
        "Notify", testApp, uint(0), QString(), QString("Kestrel privacy check"),
        QString("Synthetic test content %1").arg(notificationIds.size() + 1),
        QStringList{},
        QVariantMap{{"desktop-entry", testApp},
                    {"suppress-sound", true},
                    {"transient", true}},
        1000);
    if (reply.isValid())
      notificationIds << reply.value();
    else {
      result["error"] = reply.error().message();
      finish(3);
    }
  }
  void notifications() {
    // This mode is an explicit integration test, never used by the user app.
    // All payloads remain in memory and only a unique synthetic app is kept.
    testApp =
        "kestrel-validation-" + QUuid::createUuid().toString(QUuid::Id128);
    notificationOwner =
        bus.interface()->serviceOwner("org.freedesktop.Notifications").value();
    inbox = new NotificationsService(this, true, true, bus);
    inbox->configure(true, {testApp});
    phaseClock.start();
    notificationPoll.setInterval(80);
    connect(&notificationPoll, &QTimer::timeout, this, [this] {
      if (phaseClock.elapsed() > 3500) {
        result["error"] = "Notification validation stage timed out";
        result["phase"] = notificationPhase;
        finish(3);
        return;
      }
      if (notificationPhase == 0 && inbox->status().startsWith("Watching")) {
        sendTestNotification();
        notificationPhase = 1;
        phaseClock.restart();
      } else if (notificationPhase == 1 && inbox->entries().size() == 1) {
        result["syntheticNotificationReceived"] = true;
        QDBusInterface daemon("org.freedesktop.Notifications",
                              "/org/freedesktop/Notifications",
                              "org.freedesktop.Notifications", bus);
        daemon.setTimeout(1200);
        QDBusReply<uint> reply = daemon.call(
            "Inhibit", QString("io.github.kestrel.Island"),
            QString("Temporary Kestrel privacy validation"), QVariantMap{});
        if (!reply.isValid()) {
          result["error"] = reply.error().message();
          finish(3);
          return;
        }
        cookie = reply.value();
        notificationPhase = 2;
        phaseClock.restart();
      } else if (notificationPhase == 2 && !inbox->session()->safe() &&
                 inbox->entries().isEmpty()) {
        result["inboxClearedDuringQuiet"] = true;
        sendTestNotification();
        notificationPhase = 3;
        phaseClock.restart();
      } else if (notificationPhase == 3 && phaseClock.elapsed() > 250) {
        result["quietNotificationRejected"] = inbox->entries().isEmpty();
        release();
        notificationPhase = 4;
        phaseClock.restart();
      } else if (notificationPhase == 4 &&
                 inbox->status().startsWith("Watching")) {
        result["noReplayAfterQuiet"] = inbox->entries().isEmpty();
        sendTestNotification();
        notificationPhase = 5;
        phaseClock.restart();
      } else if (notificationPhase == 5 && inbox->entries().size() == 1) {
        result["receivedAfterResume"] = true;
        inbox->configure(false, {});
        sendTestNotification();
        notificationPhase = 6;
        phaseClock.restart();
      } else if (notificationPhase == 6 && phaseClock.elapsed() > 250) {
        result["disabledInboxEmpty"] = inbox->entries().isEmpty();
        result["notificationOwnerUnchanged"] =
            notificationOwner ==
            bus.interface()
                ->serviceOwner("org.freedesktop.Notifications")
                .value();
        result["realNotificationContentSaved"] = false;
        finish(result["quietNotificationRejected"].toBool() &&
                       result["noReplayAfterQuiet"].toBool() &&
                       result["disabledInboxEmpty"].toBool() &&
                       result["notificationOwnerUnchanged"].toBool()
                   ? 0
                   : 3);
      }
    });
    notificationPoll.start();
  }
  void expansion(bool expanded) {
    QDBusInterface island("io.github.kestrel.Island", "/Island",
                          "io.github.kestrel.Island", bus);
    island.call("SetExpanded", expanded);
  }
  ~DesktopProbe() override {
    if (inbox)
      inbox->configure(false, {});
    for (auto id : notificationIds) {
      QDBusInterface daemon("org.freedesktop.Notifications",
                            "/org/freedesktop/Notifications",
                            "org.freedesktop.Notifications", bus);
      daemon.setTimeout(1200);
      daemon.call("CloseNotification", id);
    }
    release();
    if (haveExpansion)
      expansion(wasExpanded);
    for (const auto &name : scripts) {
      QDBusInterface scripting("org.kde.KWin", "/Scripting",
                               "org.kde.kwin.Scripting", bus);
      scripting.call("unloadScript", name);
    }
  }
  void finish(int code) {
    auto bytes = QJsonDocument(result).toJson(QJsonDocument::Compact);
    fprintf(stdout, "%s\n", bytes.constData());
    qApp->exit(code);
  }
  void runScript(const QString &body) {
    const auto name =
        "kestrel-live-check-" + QUuid::createUuid().toString(QUuid::Id128);
    const auto path = directory.path() + "/" + name + ".js";
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly)) {
      result["error"] = "Could not create temporary KWin check";
      finish(2);
      return;
    }
    auto js = body + QString("\ncallDBus('%1','/"
                             "Probe','io.github.kestrel.DesktopProbe','Report',"
                             "JSON.stringify(data));")
                         .arg(service);
    file.write(js.toUtf8());
    file.close();
    QDBusInterface scripting("org.kde.KWin", "/Scripting",
                             "org.kde.kwin.Scripting", bus);
    QDBusReply<int> id = scripting.call("loadScript", path, name);
    if (!id.isValid() || id.value() < 0) {
      result["error"] = "KWin scripting unavailable";
      finish(2);
      return;
    }
    scripts << name;
    QDBusInterface script("org.kde.KWin",
                          "/Scripting/Script" + QString::number(id.value()),
                          "org.kde.kwin.Script", bus);
    auto reply = script.call("run");
    if (reply.type() == QDBusMessage::ErrorMessage) {
      result["error"] = reply.errorMessage();
      finish(2);
    }
  }
  void fullscreen() {
    if (!bus.registerService(service) ||
        !bus.registerObject("/Probe", this,
                            QDBusConnection::ExportScriptableSlots)) {
      result["error"] = "Probe D-Bus registration failed";
      finish(2);
      return;
    }
    islandPid = bus.interface()->servicePid("io.github.kestrel.Island").value();
    QDBusInterface island("io.github.kestrel.Island", "/Island",
                          "io.github.kestrel.Island", bus);
    QDBusReply<QVariantMap> snapshot = island.call("State");
    if (!snapshot.isValid()) {
      result["error"] = "Install current Kestrel before the live check";
      finish(2);
      return;
    }
    wasExpanded = snapshot.value()["expanded"].toBool();
    haveExpansion = true;
    window.setWindowTitle("Kestrel fullscreen probe");
    window.setAlignment(Qt::AlignCenter);
    window.setStyleSheet("background:#16201e;color:#f5f5f7;font-size:24px");
    window.resize(900, 500);
    window.winId();
    for (auto screen : QGuiApplication::screens())
      if (screen->name() == "DP-2")
        window.windowHandle()->setScreen(screen);
    runScript("var "
              "data={stage:'before',original:workspace.activeWindow?String("
              "workspace.activeWindow.internalId):''};");
  }
  void release() {
    if (cookie) {
      QDBusInterface notifications("org.freedesktop.Notifications",
                                   "/org/freedesktop/Notifications",
                                   "org.freedesktop.Notifications", bus);
      notifications.call("UnInhibit", cookie);
      cookie = 0;
    }
  }
  void privacy(bool watchLock) {
    manual = watchLock;
    state.start();
    connect(&state, &SessionState::changed, this, [this] {
      if (manual) {
        // Only booleans are recorded; no notification watcher is registered.
        const auto snapshot = state.snapshot();
        if (snapshot["lockedKnown"].toBool() && snapshot["locked"].toBool())
          result["lockObserved"] = true;
        if (result["lockObserved"].toBool() && state.safe()) {
          result["unlockedAndSafe"] = true;
          result["notificationPayloadsCollected"] = false;
          finish(0);
        }
      } else if (cookie && !state.safe())
        quietSeen = true;
    });
    if (manual) {
      result["lockObserved"] = false;
      result["unlockedAndSafe"] = false;
      fprintf(stderr, "Lock and unlock the desktop yourself within 5 minutes. "
                      "Only session-state booleans are observed.\n");
      QTimer::singleShot(300000, this, [this] {
        result["error"] = "Lock/unlock sequence not observed";
        finish(3);
      });
      return;
    }
    QTimer::singleShot(1400, this, [this] {
      result["initialSafe"] = state.safe();
      if (!state.safe()) {
        result["error"] =
            "Session already locked, quiet, or unknown; left unchanged";
        finish(3);
        return;
      }
      QDBusInterface notifications("org.freedesktop.Notifications",
                                   "/org/freedesktop/Notifications",
                                   "org.freedesktop.Notifications", bus);
      QDBusReply<uint> reply = notifications.call(
          "Inhibit", QString("io.github.kestrel.Island"),
          QString("Temporary Kestrel privacy check"), QVariantMap{});
      if (!reply.isValid()) {
        result["error"] = reply.error().message();
        finish(3);
        return;
      }
      cookie = reply.value();
      QTimer::singleShot(700, this, [this] {
        result["quietObserved"] = quietSeen;
        release();
        QTimer::singleShot(1400, this, [this] {
          result["safeAfterRelease"] = state.safe();
          result["notificationPayloadsCollected"] = false;
          finish(quietSeen && state.safe() ? 0 : 3);
        });
      });
    });
  }
public slots:
  Q_SCRIPTABLE void Report(const QString &json) {
    if (!calledFromDBus() ||
        message().service() !=
            bus.interface()->serviceOwner("org.kde.KWin").value())
      return;
    auto data = QJsonDocument::fromJson(json.toUtf8()).object();
    if (data["stage"] == "before") {
      original = data["original"].toString();
      window.showFullScreen();
      QTimer::singleShot(700, this, [this] {
        runScript(
            "var data={stage:'activate'}; var list=workspace.stackingOrder; "
            "for(var i=0;i<list.length;i++)if(list[i].caption==='Kestrel "
            "fullscreen probe')workspace.activeWindow=list[i];");
      });
    } else if (data["stage"] == "activate") {
      QTimer::singleShot(600, this, [this] {
        runScript(
            QString("var data={stage:'sample',windows:[]}; var "
                    "list=workspace.stackingOrder; for(var "
                    "i=0;i<list.length;i++){var "
                    "w=list[i];if(w.pid===%1||w.caption==='Kestrel fullscreen "
                    "probe')data.windows.push({island:w.pid===%1,fullscreen:w."
                    "fullScreen,active:w===workspace.activeWindow,layer:w."
                    "layer,order:i});}")
                .arg(islandPid));
      });
    } else if (data["stage"] == "sample") {
      const auto list = data["windows"].toArray();
      int island = -1, full = -1;
      bool active = false;
      for (auto value : list) {
        auto w = value.toObject();
        if (w["island"].toBool())
          island = w["order"].toInt();
        else if (w["fullscreen"].toBool()) {
          full = w["order"].toInt();
          active = w["active"].toBool();
        }
      }
      result["stack"] = list;
      result["fullscreenAboveIsland"] =
          full >= 0 && island >= 0 && full > island && active;
      window.showNormal();
      // Exercise a complete explicit expansion, including when the app was
      // already expanded at the start of this diagnostic.
      expansion(false);
      QTimer::singleShot(150, this, [this] { expansion(true); });
      QTimer::singleShot(550, this, [this] {
        runScript(QString("var a=workspace.activeWindow;var "
                          "data={stage:'focus-island',probeActiveBefore:!!a&&a."
                          "caption==='Kestrel fullscreen probe'}; var "
                          "list=workspace.stackingOrder; for(var "
                          "i=0;i<list.length;i++)if(list[i].pid===%1)workspace."
                          "activeWindow=list[i];")
                      .arg(islandPid));
      });
    } else if (data["stage"] == "focus-island") {
      result["probeActiveBeforeIsland"] = data["probeActiveBefore"];
      QTimer::singleShot(400, this, [this] {
        runScript(
            QString("var w=workspace.activeWindow;var "
                    "data={stage:'focus-sample',islandActive:!!w&&w.pid===%1};")
                .arg(islandPid));
      });
    } else if (data["stage"] == "focus-sample") {
      result["islandAcceptedFocus"] = data["islandActive"];
      expansion(false);
      QTimer::singleShot(1500, this, [this] {
        runScript(
            QString(
                "var w=workspace.activeWindow;var "
                "data={stage:'focus-return',probeActive:!!w&&w.caption==='"
                "Kestrel fullscreen "
                "probe',islandStillActive:!!w&&w.pid===%1,activeMissing:!w};")
                .arg(islandPid));
      });
    } else if (data["stage"] == "focus-return") {
      result["focusReturnedAfterCollapse"] = data["probeActive"];
      result["islandStillActiveAfterCollapse"] = data["islandStillActive"];
      result["activeMissingAfterCollapse"] = data["activeMissing"];
      QDBusInterface island("io.github.kestrel.Island", "/Island",
                            "io.github.kestrel.Island", bus);
      QDBusReply<QVariantMap> snapshot = island.call("State");
      result["expandedAfterCollapse"] =
          snapshot.isValid() ? QJsonValue(snapshot.value()["expanded"].toBool())
                             : QJsonValue();
      expansion(wasExpanded);
      haveExpansion = false;
      window.hide();
      // UUID was obtained from KWin; JSON quoting prevents code interpolation.
      const auto quoted = QString::fromUtf8(QJsonDocument(QJsonArray{original})
                                                .toJson(QJsonDocument::Compact))
                              .mid(1)
                              .chopped(1);
      runScript(
          "var data={stage:'restored'}; var list=workspace.stackingOrder; "
          "for(var i=0;i<list.length;i++)if(String(list[i].internalId)===" +
          quoted + ")workspace.activeWindow=list[i];");
    } else if (data["stage"] == "restored")
      finish(result["fullscreenAboveIsland"].toBool() &&
                     result["islandAcceptedFocus"].toBool() &&
                     result["focusReturnedAfterCollapse"].toBool()
                 ? 0
                 : 3);
  }
};
int main(int argc, char **argv) {
  QApplication app(argc, argv);
  app.setApplicationName("Kestrel desktop probe");
  app.setDesktopFileName("io.github.kestrel.Island");
  app.setQuitOnLastWindowClosed(false);
  const auto args = app.arguments();
  if (args.size() != 2 ||
      !QStringList{"--fullscreen", "--quiet", "--watch-lock", "--notifications"}
           .contains(args[1])) {
    fprintf(stderr, "Use --fullscreen, --quiet, --watch-lock or "
                    "--notifications. Live desktop "
                    "checks, never unattended tests.\n");
    return 2;
  }
  DesktopProbe probe;
  QTimer::singleShot(0, &probe, [&] {
    if (args[1] == "--fullscreen")
      probe.fullscreen();
    else if (args[1] == "--notifications")
      probe.notifications();
    else
      probe.privacy(args[1] == "--watch-lock");
  });
  if (args[1] != "--watch-lock")
    QTimer::singleShot(9000, &probe, [&] {
      probe.result["error"] = "Probe timed out";
      probe.finish(3);
    });
  return app.exec();
}
#include "desktop_probe.moc"
