#include "activitycoordinator.h"
#include "focussession.h"
#include "keepawakeservice.h"
#include "languageservice.h"
#include "notificationsservice.h"
#include "systemmonitor.h"
#include "taskactivityservice.h"
#include <QDBusAbstractAdaptor>
#include <QDBusMessage>
#include <QDBusMetaType>
#include <QDBusPendingCallWatcher>
#include <QDBusPendingReply>
#include <QDBusVariant>
#include <QtTest>
#include <QTemporaryDir>
#include <QFile>
#include <QDir>

class FakeDesktop : public QObject, public QDBusContext {
  Q_OBJECT
public:
  uint next = 10;
  QSet<uint> screen, sleep;
  bool denySleep = false, locked = false, quiet = false;
  int registrations = 0, unregistrations = 0;
};
class ScreenAdaptor : public QDBusAbstractAdaptor {
  Q_OBJECT
  Q_CLASSINFO("D-Bus Interface", "org.freedesktop.ScreenSaver")
public:
  explicit ScreenAdaptor(FakeDesktop *parent)
      : QDBusAbstractAdaptor(parent), host(parent) {}
public slots:
  uint Inhibit(const QString &, const QString &) {
    auto id = ++host->next;
    host->screen.insert(id);
    return id;
  }
  void UnInhibit(uint id) { host->screen.remove(id); }
  bool GetActive() { return host->locked; }

private:
  FakeDesktop *host;
};
class PowerAdaptor : public QDBusAbstractAdaptor {
  Q_OBJECT
  Q_CLASSINFO("D-Bus Interface", "org.freedesktop.PowerManagement.Inhibit")
public:
  explicit PowerAdaptor(FakeDesktop *parent)
      : QDBusAbstractAdaptor(parent), host(parent) {}
public slots:
  uint Inhibit(const QString &, const QString &) {
    if (host->denySleep) {
      host->sendErrorReply(QDBusError::AccessDenied, "Policy denied");
      return 0;
    }
    auto id = ++host->next;
    host->sleep.insert(id);
    return id;
  }
  void UnInhibit(uint id) { host->sleep.remove(id); }

private:
  FakeDesktop *host;
};
class NotifyAdaptor : public QDBusAbstractAdaptor {
  Q_OBJECT
  Q_CLASSINFO("D-Bus Interface", "org.freedesktop.Notifications")
  Q_PROPERTY(bool Inhibited READ inhibited)
public:
  explicit NotifyAdaptor(FakeDesktop *parent)
      : QDBusAbstractAdaptor(parent), host(parent) {}
  bool inhibited() const { return host->quiet; }

private:
  FakeDesktop *host;
};
class WatcherAdaptor : public QDBusAbstractAdaptor {
  Q_OBJECT
  Q_CLASSINFO("D-Bus Interface", "org.kde.NotificationManager")
public:
  explicit WatcherAdaptor(FakeDesktop *parent)
      : QDBusAbstractAdaptor(parent), host(parent) {}
public slots:
  void RegisterWatcher() { ++host->registrations; }
  void UnRegisterWatcher() { ++host->unregistrations; }

private:
  FakeDesktop *host;
};

struct FakeLayoutNames {
  QString shortName, displayName, longName;
};
Q_DECLARE_METATYPE(FakeLayoutNames)
QDBusArgument &operator<<(QDBusArgument &argument,
                          const FakeLayoutNames &names) {
  argument.beginStructure();
  argument << names.shortName << names.displayName << names.longName;
  argument.endStructure();
  return argument;
}
const QDBusArgument &operator>>(const QDBusArgument &argument,
                                FakeLayoutNames &names) {
  argument.beginStructure();
  argument >> names.shortName >> names.displayName >> names.longName;
  argument.endStructure();
  return argument;
}
class KeyboardAdaptor : public QDBusAbstractAdaptor {
  Q_OBJECT
  Q_CLASSINFO("D-Bus Interface", "org.kde.KeyboardLayouts")
public:
  explicit KeyboardAdaptor(FakeDesktop *parent)
      : QDBusAbstractAdaptor(parent), host(parent) {}
  QList<FakeLayoutNames> names;
public slots:
  uint getLayout() { return 0; }
  QList<FakeLayoutNames> getLayoutsList() { return names; }

private:
  FakeDesktop *host;
};

class ActivitiesTest : public QObject {
  Q_OBJECT
private slots:
  void memoryUsesAvailableIncludingReclaimableCache() {
    auto memory = SystemMonitor::parseMemory(
        "MemTotal: 1000 kB\nMemFree: 10 kB\nMemAvailable: 600 kB\nSwapTotal: "
        "200 kB\nSwapFree: 150 kB\n");
    QVERIFY(memory["ready"].toBool());
    QCOMPARE(memory["used"].toULongLong(), 400ULL * 1024);
    QCOMPARE(memory["available"].toULongLong(), 600ULL * 1024);
    QCOMPARE(memory["percent"].toDouble(), 40.0);
    QCOMPARE(memory["swapUsed"].toULongLong(), 50ULL * 1024);
    QVERIFY(!SystemMonitor::parseMemory(
                 "MemTotal: 1000 kB\nMemFree: 10 kB")["ready"]
                 .toBool());
    QCOMPARE(SystemMonitor::parseMemory(
                 "MemTotal: 1000 kB\nMemAvailable: 1200 kB")["used"]
                 .toULongLong(),
             0ULL);
  }
  void nvmeTemperaturesUseCompositeAndKeepUnavailableDevices() {
    QTemporaryDir sysfs;
    QVERIFY(sysfs.isValid());
    auto write = [&](const QString &path, const QByteArray &data) {
      const auto full = sysfs.path() + "/" + path;
      QDir().mkpath(QFileInfo(full).absolutePath());
      QFile file(full);
      QVERIFY(file.open(QIODevice::WriteOnly));
      QCOMPARE(file.write(data), data.size());
    };
    write("nvme0/model", "  Test NVMe drive  \n");
    write("nvme0/hwmon7/temp1_input", "42850\n");
    write("nvme0/hwmon7/temp2_input", "70000\n");
    write("nvme1/model", "No sensor\n");
    write("nvme2/hwmon0/temp1_input", "not a reading\n");
    write("nvme0n1/model", "Namespace, not a controller\n");
    auto drives = SystemMonitor::nvmeTemperatures(sysfs.path());
    QCOMPARE(drives.size(), 3);
    QCOMPARE(drives[0].toMap()["name"].toString(), QString("Test NVMe drive"));
    QCOMPARE(drives[0].toMap()["temperature"].toDouble(), 42.85);
    QCOMPARE(drives[1].toMap()["temperature"].toDouble(), -1.0);
    QCOMPARE(drives[2].toMap()["name"].toString(), QString("nvme2"));
    QCOMPARE(drives[2].toMap()["temperature"].toDouble(), -1.0);
    QVERIFY(SystemMonitor::nvmeTemperatures(sysfs.path() + "/missing").isEmpty());
  }
  void nvidiaReportsUnsupportedCountersAsUnavailable() {
    auto cards = SystemMonitor::parseNvidia(
        "0, NVIDIA Test GPU, 00000000:01:00.0, 23, 1024, 16384, 44, 65.5\n1, "
        "NVIDIA Other GPU, 00000000:02:00.0, [N/A], [N/A], [N/A], [N/A], "
        "[N/A]\nbroken row\n");
    QCOMPARE(cards.size(), 2);
    auto gpu = cards[0].toMap();
    QCOMPARE(gpu["id"].toString(), QString("0000:01:00.0"));
    QCOMPARE(gpu["percent"].toDouble(), 23.0);
    QCOMPARE(gpu["memoryUsed"].toDouble(), 1073741824.0);
    QCOMPARE(gpu["power"].toDouble(), 65.5);
    auto unavailable = cards[1].toMap();
    QCOMPARE(unavailable["percent"].toDouble(), -1.0);
    QCOMPARE(unavailable["memoryTotal"].toDouble(), -1.0);
    QCOMPARE(unavailable["temperature"].toDouble(), -1.0);
    QVERIFY(SystemMonitor::parseNvidia("a,b,c,d,e,f,g,h").isEmpty());
  }
  void noticePriorityDedupeAndExpiry() {
    qint64 now = 0;
    ActivityCoordinator activity(nullptr, [&] { return now; });
    activity.upsert({{"key", "music"}, {"title", "Track"}});
    activity.show("volume", "10%", 30, 1600);
    activity.show("volume", "50%", 30, 1600);
    QCOMPARE(activity.notice()["title"].toString(), "50%");
    activity.show("complete", "Finished", 40, 6000);
    QCOMPARE(activity.notice()["key"].toString(), "complete");
    now = 1700;
    activity.expire();
    activity.dismiss();
    QVERIFY(activity.notice().isEmpty());
    QCOMPARE(activity.selected()["key"].toString(), "music");
    activity.show("same", "Error", 1, 1000);
    now += 900;
    activity.show("same", "Error", 1, 1000);
    now += 200;
    activity.expire();
    QVERIFY(!activity.notice().isEmpty());
  }
  void boundedActivitiesAndCycling() {
    ActivityCoordinator activity;
    for (int i = 0; i < 30; ++i)
      activity.upsert({{"key", QString::number(i)}});
    QCOMPARE(activity.activities().size(), 8);
    auto first = activity.selected();
    activity.cycle();
    QVERIFY(activity.selected() != first);
    activity.remove(activity.selected()["key"].toString());
    QCOMPARE(activity.activities().size(), 7);
  }
  void focusCompletionAndExplicitNext() {
    qint64 now = 0;
    FocusSession focus(nullptr, [&] { return now; });
    QSignalSpy done(&focus, &FocusSession::finished);
    focus.pomodoro(1, 1, 1);
    now = 60001;
    focus.tick();
    QCOMPARE(done.size(), 1);
    QCOMPARE(focus.snapshot()["round"].toInt(), 1);
    QCOMPARE(focus.snapshot()["state"].toString(), "completed");
    now += 3600000;
    focus.tick();
    QCOMPARE(done.size(), 1);
    QCOMPARE(focus.snapshot()["phase"].toString(), "work");
    focus.next();
    QCOMPARE(focus.snapshot()["phase"].toString(), "short break");
    QCOMPARE(focus.snapshot()["round"].toInt(), 1);
    QCOMPARE(focus.remaining(), 60);
    focus.pause();
    now += 10000;
    QCOMPARE(focus.remaining(), 60);
    focus.pause();
    now += 1000;
    QCOMPARE(focus.remaining(), 59);
    focus.cancel();
    QCOMPARE(focus.snapshot()["state"].toString(), "idle");
  }
  void taskSchemaStaleAndExpiry() {
    qint64 now = 0;
    TaskActivityService tasks(nullptr, [&] { return now; });
    QVariantMap event{{"version", 1},
                      {"id", "build:1"},
                      {"state", "running"},
                      {"label", "Build"},
                      {"seq", 1}};
    QVERIFY(!tasks.report(event));
    tasks.setEnabled(true);
    QVERIFY(tasks.report(event));
    QVERIFY(!tasks.report(event));
    now = 120001;
    tasks.expire();
    QCOMPARE(tasks.tasks().first().toMap()["state"].toString(), "stale");
    event["seq"] = 2;
    event["state"] = "completed";
    QVERIFY(tasks.report(event));
    now += 60001;
    tasks.expire();
    QVERIFY(tasks.tasks().isEmpty());
    event["seq"] = 3;
    event["prompt"] = "private";
    QVERIFY(!tasks.report(event));
    event.remove("prompt");
    event["label"] = "bad\nlabel";
    QVERIFY(!tasks.report(event));
  }
  void taskCapsAndQuietShortCompletion() {
    TaskActivityService tasks;
    tasks.setEnabled(true);
    QSignalSpy notices(&tasks, &TaskActivityService::notice);
    for (int i = 0; i < 8; ++i)
      QVERIFY(tasks.report({{"version", 1},
                            {"id", QString::number(i)},
                            {"state", "running"},
                            {"label", "Test"}}));
    QVERIFY(!tasks.report({{"version", 1},
                           {"id", "9"},
                           {"state", "running"},
                           {"label", "Ninth"}}));
    QVERIFY(tasks.report({{"version", 1},
                          {"id", "0"},
                          {"state", "completed"},
                          {"label", "Done"}}));
    QCOMPARE(notices.size(), 0);
    tasks.setEnabled(false);
    QVERIFY(tasks.tasks().isEmpty());
  }
  void keepAwakePartialFailureAndRelease() {
    auto bus = QDBusConnection::sessionBus();
    QVERIFY(bus.isConnected());
    FakeDesktop host;
    ScreenAdaptor screen(&host);
    PowerAdaptor power(&host);
    QVERIFY(bus.registerService("org.freedesktop.ScreenSaver"));
    QVERIFY(bus.registerService("org.freedesktop.PowerManagement"));
    QVERIFY(bus.registerObject("/ScreenSaver", &host,
                               QDBusConnection::ExportAdaptors));
    QVERIFY(bus.registerObject("/org/freedesktop/PowerManagement/Inhibit",
                               &host, QDBusConnection::ExportAdaptors));
    {
      KeepAwakeService awake;
      awake.start(15);
      QTRY_COMPARE(awake.snapshot()["state"].toString(), "active");
      QCOMPARE(host.screen.size(), 1);
      QCOMPARE(host.sleep.size(), 1);
      awake.stop();
      QTRY_VERIFY(host.screen.isEmpty() && host.sleep.isEmpty());
      QTRY_COMPARE(awake.snapshot()["state"].toString(), "off");
      host.denySleep = true;
      awake.start(15);
      QTRY_COMPARE(awake.snapshot()["state"].toString(), "error");
      QTRY_VERIFY(host.screen.isEmpty() && host.sleep.isEmpty());
    }
    bus.unregisterObject("/ScreenSaver");
    bus.unregisterObject("/org/freedesktop/PowerManagement/Inhibit");
    bus.unregisterService("org.freedesktop.ScreenSaver");
    bus.unregisterService("org.freedesktop.PowerManagement");
  }
  void realSessionSignalRoutesFailClosedAndRecover() {
    auto bus = QDBusConnection::sessionBus();
    FakeDesktop host;
    ScreenAdaptor screen(&host);
    NotifyAdaptor notification(&host);
    QVERIFY(bus.registerService("org.freedesktop.ScreenSaver"));
    QVERIFY(bus.registerService("org.freedesktop.Notifications"));
    QVERIFY(bus.registerObject("/ScreenSaver", &host,
                               QDBusConnection::ExportAdaptors));
    QVERIFY(bus.registerObject("/org/freedesktop/Notifications", &host,
                               QDBusConnection::ExportAdaptors));
    {
      SessionState state;
      QVERIFY(!state.safe());
      QTRY_VERIFY(state.safe());
      host.locked = true;
      auto locking = QDBusMessage::createSignal(
          "/ScreenSaver", "org.kde.screensaver", "AboutToLock");
      QVERIFY(bus.send(locking));
      QTRY_VERIFY(!state.safe());
      QVERIFY(state.snapshot()["locked"].toBool());
      host.locked = false;
      auto unlocked = QDBusMessage::createSignal(
          "/ScreenSaver", "org.freedesktop.ScreenSaver", "ActiveChanged");
      unlocked << false;
      QVERIFY(bus.send(unlocked));
      QTRY_VERIFY(state.safe());
      host.quiet = true;
      auto quiet = QDBusMessage::createSignal("/org/freedesktop/Notifications",
                                              "org.freedesktop.DBus.Properties",
                                              "PropertiesChanged");
      quiet << QString("org.freedesktop.Notifications")
            << QVariantMap{{"Inhibited", true}} << QStringList();
      QVERIFY(bus.send(quiet));
      QTRY_VERIFY(!state.safe());
      host.quiet = false;
      auto invalidated = QDBusMessage::createSignal(
          "/org/freedesktop/Notifications", "org.freedesktop.DBus.Properties",
          "PropertiesChanged");
      invalidated << QString("org.freedesktop.Notifications") << QVariantMap{}
                  << QStringList{"Inhibited"};
      QVERIFY(bus.send(invalidated));
      QTRY_VERIFY(state.safe());
      bus.unregisterService("org.freedesktop.Notifications");
      QTRY_VERIFY(!state.safe());
    }
    bus.unregisterObject("/ScreenSaver");
    bus.unregisterObject("/org/freedesktop/Notifications");
    bus.unregisterService("org.freedesktop.ScreenSaver");
  }
  void languageSwitchResolvesLayoutNames() {
    auto bus = QDBusConnection::sessionBus();
    QVERIFY(bus.isConnected());
    qDBusRegisterMetaType<FakeLayoutNames>();
    qDBusRegisterMetaType<QList<FakeLayoutNames>>();
    FakeDesktop host;
    KeyboardAdaptor keyboard(&host);
    keyboard.names = {{"us", "us", "English (US)"}, {"th", "th", "Thai"}};
    QVERIFY(bus.registerService("org.kde.keyboard"));
    QVERIFY(bus.registerObject("/Layouts", &host,
                               QDBusConnection::ExportAdaptors));
    {
      LanguageService language;
      QTRY_VERIFY(language.available());
      QSignalSpy spy(&language, &LanguageService::switched);
      auto index = QDBusMessage::createSignal(
          "/Layouts", "org.kde.KeyboardLayouts", "layoutChanged");
      index << uint(1);
      QVERIFY(bus.send(index));
      QTRY_COMPARE(spy.size(), 1);
      QCOMPARE(spy.first().first().toString(), QString("Thai"));
      QCOMPARE(language.layout(), QString("Thai"));
      // The older string-based daemon resolves through the same list.
      auto legacy = QDBusMessage::createSignal(
          "/Layouts", "org.kde.KeyboardLayouts", "currentLayoutChanged");
      legacy << QString("us");
      QVERIFY(bus.send(legacy));
      QTRY_COMPARE(spy.size(), 2);
      QCOMPARE(spy.at(1).first().toString(), QString("English (US)"));
      // Re-reporting the current layout does not re-announce.
      QVERIFY(bus.send(legacy));
      QTest::qWait(300);
      QCOMPARE(spy.size(), 2);
    }
    bus.unregisterObject("/Layouts");
    bus.unregisterService("org.kde.keyboard");
  }
  void notificationGateFilteringAndSessionClear() {
    auto bus = QDBusConnection::sessionBus();
    FakeDesktop host;
    ScreenAdaptor screen(&host);
    NotifyAdaptor notification(&host);
    WatcherAdaptor watcher(&host);
    QVERIFY(bus.registerService("org.freedesktop.ScreenSaver"));
    QVERIFY(bus.registerService("org.freedesktop.Notifications"));
    QVERIFY(bus.registerObject("/ScreenSaver", &host,
                               QDBusConnection::ExportAdaptors));
    QVERIFY(bus.registerObject("/org/freedesktop/Notifications", &host,
                               QDBusConnection::ExportAdaptors));
    {
      NotificationsService inbox(nullptr, true, true);
      inbox.configure(true, {"demo"});
      QTRY_VERIFY(inbox.status().startsWith("Watching"));
      QVERIFY(host.registrations > 0);
      QVERIFY(!inbox.ingest(":spoof", 1, "demo", 0, "secret", "body"));
      QVERIFY(
          !inbox.ingest(bus.baseService(), 1, "other", 0, "secret", "body"));
      QVERIFY(inbox.ingest(bus.baseService(), 1, "demo", 0, "<b>literal</b>",
                           "body"));
      QCOMPARE(inbox.entries().size(), 1);
      QVERIFY(
          inbox.ingest(bus.baseService(), 2, "demo", 1, "replacement", "body"));
      QCOMPARE(inbox.entries().size(), 1);
      inbox.session()->setTestState(true, false);
      QVERIFY(inbox.entries().isEmpty());
      QVERIFY(!inbox.ingest(bus.baseService(), 3, "demo", 0, "locked", "body"));
      QTRY_VERIFY(host.unregistrations > 0);
      inbox.session()->setTestState(false, true);
      QVERIFY(!inbox.ingest(bus.baseService(), 3, "demo", 0, "quiet", "body"));
      inbox.session()->setTestState(false, false);
      QTRY_VERIFY(inbox.status().startsWith("Watching"));
      QVERIFY(inbox.entries().isEmpty());
      for (uint i = 0; i < 30; ++i)
        QVERIFY(
            inbox.ingest(bus.baseService(), i, "demo", 0, "summary", "body"));
      QCOMPARE(inbox.entries().size(), 20);
      inbox.configure(false, {});
      QVERIFY(inbox.entries().isEmpty());
    }
    {
      NotificationsService gated(nullptr, true, false);
      gated.configure(true, {"demo"});
      QVERIFY(gated.status().contains("validation"));
      QVERIFY(!gated.ingest(bus.baseService(), 1, "demo", 0, "hidden", "body"));
    }
    bus.unregisterObject("/ScreenSaver");
    bus.unregisterObject("/org/freedesktop/Notifications");
    bus.unregisterService("org.freedesktop.ScreenSaver");
    bus.unregisterService("org.freedesktop.Notifications");
  }
};
QTEST_MAIN(ActivitiesTest)
#include "test_activities.moc"
