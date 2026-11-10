#include "backend.h"
#include <QDBusAbstractAdaptor>
#include <QDBusConnection>
#include <QDBusObjectPath>
#include <QDBusVariant>
#include <QFile>
#include <QImage>
#include <QQmlApplicationEngine>
#include <QQmlComponent>
#include <QQmlContext>
#include <QQuickItem>
#include <QQuickWindow>
#include <QScopeGuard>
#include <QTemporaryDir>
#include <QtTest>

static QQuickItem *findVisualItem(QQuickItem *parent, const QString &name) {
  if (!parent)
    return nullptr;
  if (parent->objectName() == name)
    return parent;
  for (auto *child : parent->childItems())
    if (auto *match = findVisualItem(child, name))
      return match;
  return nullptr;
}

class FakePlayer : public QObject {
  Q_OBJECT
  Q_CLASSINFO("D-Bus Interface", "org.mpris.MediaPlayer2.Player")
  Q_PROPERTY(QString PlaybackStatus READ status)
  Q_PROPERTY(QVariantMap Metadata READ metadata)
  Q_PROPERTY(bool CanControl READ yes)
  Q_PROPERTY(bool CanPlay READ yes)
  Q_PROPERTY(bool CanPause READ yes)
  Q_PROPERTY(bool CanGoNext READ yes)
  Q_PROPERTY(bool CanGoPrevious READ yes)
  Q_PROPERTY(bool CanSeek READ canSeek)
  Q_PROPERTY(qlonglong Position READ position)
  Q_PROPERTY(double Rate READ rate)
  Q_PROPERTY(bool Shuffle READ shuffle WRITE setShuffle)
  Q_PROPERTY(QString LoopStatus READ loopStatus WRITE setLoopStatus)
public:
  bool playing = true;
  int skips = 0;
  int seeks = 0;
  qlonglong currentPosition = 10000000;
  QString track = "/track/one";
  QString art;
  bool seekable = true;
  bool shuffleOn = false;
  QString loop = "None";
  bool shuffle() const { return shuffleOn; }
  void setShuffle(bool value) { shuffleOn = value; }
  QString loopStatus() const { return loop; }
  void setLoopStatus(const QString &value) { loop = value; }
  QString status() const { return playing ? "Playing" : "Paused"; }
  QVariantMap metadata() const {
    auto map = QVariantMap{
        {"xesam:title", "Integration test song"},
        {"xesam:artist", QStringList{"Test artist"}},
        {"mpris:length", qlonglong(180000000)},
        {"mpris:trackid", QVariant::fromValue(QDBusObjectPath(track))}};
    if (!art.isEmpty())
      map["mpris:artUrl"] = art;
    return map;
  }
  bool yes() const { return true; }
  bool canSeek() const { return seekable; }
  qlonglong position() const { return currentPosition; }
  double rate() const { return 1.0; }
  void publish() {
    auto event = QDBusMessage::createSignal("/org/mpris/MediaPlayer2",
                                            "org.freedesktop.DBus.Properties",
                                            "PropertiesChanged");
    event << "org.mpris.MediaPlayer2.Player"
          << QVariantMap{{"Metadata", metadata()}} << QStringList();
    QDBusConnection::sessionBus().send(event);
  }
public slots:
  void PlayPause() { playing = !playing; }
  void Next() { ++skips; }
  void SetPosition(const QDBusObjectPath &id, qlonglong value) {
    if (id.path() == track) {
      ++seeks;
      currentPosition = value;
    }
  }
};

class FakeDesktopHost : public QObject {
  Q_OBJECT
public:
  uint next = 10;
  bool locked = false, quiet = false;
  int registrations = 0;
};
class FakeScreenAdaptor : public QDBusAbstractAdaptor {
  Q_OBJECT
  Q_CLASSINFO("D-Bus Interface", "org.freedesktop.ScreenSaver")
public:
  explicit FakeScreenAdaptor(FakeDesktopHost *parent)
      : QDBusAbstractAdaptor(parent), host(parent) {}
public slots:
  uint Inhibit(const QString &, const QString &) { return ++host->next; }
  void UnInhibit(uint) {}
  bool GetActive() { return host->locked; }

private:
  FakeDesktopHost *host;
};
class FakeInhibitAdaptor : public QDBusAbstractAdaptor {
  Q_OBJECT
  Q_CLASSINFO("D-Bus Interface", "org.freedesktop.Notifications")
  Q_PROPERTY(bool Inhibited READ inhibited)
public:
  explicit FakeInhibitAdaptor(FakeDesktopHost *parent)
      : QDBusAbstractAdaptor(parent), host(parent) {}
  bool inhibited() const { return host->quiet; }

private:
  FakeDesktopHost *host;
};
class FakeWatcherAdaptor : public QDBusAbstractAdaptor {
  Q_OBJECT
  Q_CLASSINFO("D-Bus Interface", "org.kde.NotificationManager")
public:
  explicit FakeWatcherAdaptor(FakeDesktopHost *parent)
      : QDBusAbstractAdaptor(parent), host(parent) {}
public slots:
  void RegisterWatcher() { ++host->registrations; }
  void UnRegisterWatcher() {}

private:
  FakeDesktopHost *host;
};

class BackendTest : public QObject {
  Q_OBJECT
private slots:
  void hoverEnabledByDefault() {
    const auto oldOrganization = QCoreApplication::organizationName();
    const auto oldApplication = QCoreApplication::applicationName();
    QTemporaryDir config;
    const auto restore = qScopeGuard([&] {
      QCoreApplication::setOrganizationName(oldOrganization);
      QCoreApplication::setApplicationName(oldApplication);
    });
    QCoreApplication::setOrganizationName("KestrelRegressionTests");
    QCoreApplication::setApplicationName("ClickOnly");
    QSettings::setPath(QSettings::NativeFormat, QSettings::UserScope,
                       config.path());
    Backend backend(nullptr, false);
    QVERIFY2(backend.hoverExpand(),
             "Hover peek is enabled by default.");
  }
  void settingsDiscoverableWhileCollapsed() {
    Backend backend(nullptr, false);
    QQmlApplicationEngine engine;
    engine.rootContext()->setContextProperty("island", &backend);
    engine.rootContext()->setContextProperty("previewMode", true);
    engine.rootContext()->setContextProperty("startExpanded", false);
    engine.load(QUrl("qrc:/qml/Main.qml"));
    QVERIFY(!engine.rootObjects().isEmpty());
    auto *settings = engine.rootObjects().first()->findChild<QQuickItem *>(
        "compactSettingsButton");
    QVERIFY2(settings, "Settings can only be found after expansion or through "
                       "an undisclosed context menu.");
    auto *icon = settings->findChild<QQuickItem *>("buttonIcon");
    QVERIFY2(
        icon,
        "Use a bundled Image instead of the platform's missing icon delegate.");
    QTRY_COMPARE(icon->property("status").toInt(), 1); // Image.Ready
    QVERIFY(QMetaObject::invokeMethod(settings, "clicked"));
    QCOMPARE(engine.rootObjects().first()->property("page").toInt(), 3);
    backend.preferencesChanged();
    QCOMPARE(engine.rootObjects().first()->property("page").toInt(), 3);
    QVERIFY(engine.rootObjects().first()->property("expanded").toBool());
  }
  void compactPeeksOnHoverAndExpandsOnClick() {
    Backend backend(nullptr, false);
    QQmlApplicationEngine engine;
    engine.rootContext()->setContextProperty("island", &backend);
    engine.rootContext()->setContextProperty("previewMode", true);
    engine.rootContext()->setContextProperty("startExpanded", false);
    engine.load(QUrl("qrc:/qml/Main.qml"));
    QVERIFY(!engine.rootObjects().isEmpty());
    auto *root = qobject_cast<QQuickWindow *>(engine.rootObjects().first());
    QVERIFY(root);
    root->show();
    QVERIFY(QTest::qWaitForWindowExposed(root));
    auto *surface = root->findChild<QQuickItem *>("islandSurface");
    QVERIFY(surface);
    QCOMPARE(surface->width(), 288.0);
    QCOMPARE(surface->height(), 40.0);
    QTest::mouseMove(root, QPoint(10, 200));
    QTest::qWait(20);
    QTest::mouseMove(root, QPoint(242, 16));
    QTRY_COMPARE(surface->height(), 54.0);
    QTRY_COMPARE(surface->width(), 316.0);
    QVERIFY(!root->property("expanded").toBool());
    backend.setPreferredPage(2);
    QTest::mouseClick(root, Qt::LeftButton, Qt::NoModifier, QPoint(242, 16));
    QTRY_VERIFY(root->property("expanded").toBool());
    QVERIFY(!root->property("pinned").toBool());
    QCOMPARE(root->property("page").toInt(), 2);
    QTRY_COMPARE(surface->height(), 432.0);
    QTRY_COMPARE(surface->width(), 520.0);
    QVERIFY(QMetaObject::invokeMethod(root, "collapse"));
    QTest::qWait(350);
    QVERIFY(!root->property("expanded").toBool());
    QVERIFY(!root->property("pinned").toBool());
    QTest::mouseMove(root, QPoint(10, 300));
    QTest::qWait(20);
    QTest::mouseMove(root, QPoint(242, 16));
    QTRY_COMPARE(surface->height(), 54.0);
    QTRY_COMPARE(surface->width(), 316.0);
    QVERIFY(!root->property("expanded").toBool());
    backend.setPreferredPage(0);
  }
  void expandedStaysUntilExplicitClose() {
    Backend backend(nullptr, false);
    QQmlApplicationEngine engine;
    engine.rootContext()->setContextProperty("island", &backend);
    engine.rootContext()->setContextProperty("previewMode", true);
    engine.rootContext()->setContextProperty("startExpanded", true);
    engine.load(QUrl("qrc:/qml/Main.qml"));
    QVERIFY(!engine.rootObjects().isEmpty());
    auto *root = qobject_cast<QQuickWindow *>(engine.rootObjects().first());
    root->setProperty("page", 0);
    root->show();
    QVERIFY(QTest::qWaitForWindowExposed(root));
    QTest::qWait(350);
    auto *pin = root->findChild<QQuickItem *>("pinButton");
    QVERIFY(pin);
    auto moveInside = [&] {
      QTest::mouseMove(root, QPoint(294, 90));
      QTest::qWait(30);
    };
    auto moveOutside = [&] { QTest::mouseMove(root, QPoint(10, 450)); };
    auto clickPin = [&] {
      QTest::mouseClick(
          root, Qt::LeftButton, Qt::NoModifier,
          pin->mapToScene(QPointF(pin->width() / 2, pin->height() / 2))
              .toPoint());
    };
    // Expansion does not pin itself and pointer leave never closes it;
    // only an outside dismissal, explicit collapse or unpin does.
    QVERIFY(!root->property("pinned").toBool());
    moveInside();
    moveOutside();
    QTest::qWait(350);
    QVERIFY(root->property("expanded").toBool());
    // Simulated outside click (window deactivation) dismisses it.
    moveInside();
    QVERIFY(QMetaObject::invokeMethod(root, "dismissOutside"));
    QVERIFY(!root->property("expanded").toBool());
    // Expanded again and explicitly pinned: outside clicks must not close.
    root->setProperty("expanded", true);
    QTest::qWait(350);
    moveInside();
    clickPin();
    QVERIFY(root->property("pinned").toBool());
    QVERIFY(pin->property("checked").toBool());
    QVERIFY(QMetaObject::invokeMethod(root, "dismissOutside"));
    QVERIFY(root->property("expanded").toBool());
    // Unpin restores outside dismissal.
    clickPin();
    QVERIFY(!root->property("pinned").toBool());
    QVERIFY(QMetaObject::invokeMethod(root, "dismissOutside"));
    QVERIFY(!root->property("expanded").toBool());
    backend.setPreferredPage(0);
  }
  void languageAndNotificationNotices() {
    const auto oldOrganization = QCoreApplication::organizationName();
    const auto oldApplication = QCoreApplication::applicationName();
    QTemporaryDir config;
    const auto restore = qScopeGuard([&] {
      QCoreApplication::setOrganizationName(oldOrganization);
      QCoreApplication::setApplicationName(oldApplication);
    });
    QCoreApplication::setOrganizationName("KestrelRegressionTests");
    QCoreApplication::setApplicationName("Notices");
    QSettings::setPath(QSettings::NativeFormat, QSettings::UserScope,
                       config.path());
    QSettings().setValue("notifications/validated", true);
    QSettings().setValue("notifications/enabled", true);
    QSettings().setValue("notifications/apps", "demo");
    auto bus = QDBusConnection::sessionBus();
    QVERIFY(bus.isConnected());
    FakeDesktopHost host;
    FakeScreenAdaptor screen(&host);
    FakeInhibitAdaptor inhibit(&host);
    FakeWatcherAdaptor watcher(&host);
    QVERIFY(bus.registerService("org.freedesktop.ScreenSaver"));
    QVERIFY(bus.registerService("org.freedesktop.Notifications"));
    QVERIFY(bus.registerObject("/ScreenSaver", &host,
                               QDBusConnection::ExportAdaptors));
    QVERIFY(bus.registerObject("/org/freedesktop/Notifications", &host,
                               QDBusConnection::ExportAdaptors));
    {
      Backend backend(nullptr, true);
      QTRY_VERIFY(backend.notifications()->status().startsWith("Watching"));
      backend.language()->report("Thai");
      QCOMPARE(backend.activity()->notice()["title"].toString(),
               "Language · Thai");
      QVERIFY(backend.notifications()->ingest(bus.baseService(), 1, "demo", 0,
                                              "Hello", "Body"));
      QCOMPARE(backend.activity()->notice()["title"].toString(),
               "demo · Hello");
      QVERIFY(backend.notifications()->ingest(bus.baseService(), 2, "demo", 0,
                                              "Again", "Body"));
      QCOMPARE(backend.activity()->notice()["title"].toString(),
               "demo · Again");
    }
    bus.unregisterObject("/ScreenSaver");
    bus.unregisterObject("/org/freedesktop/Notifications");
    bus.unregisterService("org.freedesktop.ScreenSaver");
    bus.unregisterService("org.freedesktop.Notifications");
  }
  void noticeNodsCompactSurface() {
    Backend backend(nullptr, false);
    QQmlApplicationEngine engine;
    engine.rootContext()->setContextProperty("island", &backend);
    engine.rootContext()->setContextProperty("previewMode", true);
    engine.rootContext()->setContextProperty("startExpanded", false);
    engine.load(QUrl("qrc:/qml/Main.qml"));
    QVERIFY(!engine.rootObjects().isEmpty());
    auto *root = qobject_cast<QQuickWindow *>(engine.rootObjects().first());
    QVERIFY(root);
    root->show();
    QVERIFY(QTest::qWaitForWindowExposed(root));
    auto *surface = root->findChild<QQuickItem *>("islandSurface");
    QVERIFY(surface);
    QCOMPARE(surface->height(), 40.0);
    // Card notices (language, notifications) swap the compact contents but
    // keep the exact idle frame.
    backend.language()->report("Thai");
    QTRY_VERIFY(root->property("noticeCard").toBool());
    QTest::qWait(60);
    QCOMPARE(surface->height(), 40.0);
    QCOMPARE(surface->width(), 288.0);
    QTRY_COMPARE_WITH_TIMEOUT(root->property("noticeCard").toBool(), false,
                              4000);
    QTRY_COMPARE(surface->height(), 40.0);
    // Ordinary notices keep the smaller nod.
    backend.activity()->show("task:x", "Build done", 30, 2000);
    QTRY_VERIFY(root->property("noticeBump").toBool());
    QTRY_COMPARE(surface->height(), 54.0);
    QTRY_COMPARE_WITH_TIMEOUT(root->property("noticeBump").toBool(), false,
                              3000);
    QTRY_COMPARE(surface->height(), 40.0);
    // Expanded notices cross-fade the compact title without growing.
    root->setProperty("expanded", true);
    QTest::qWait(60);
    backend.language()->report("English");
    QTest::qWait(60);
    QVERIFY(!root->property("noticeCard").toBool());
    QVERIFY(!root->property("noticeBump").toBool());
    QVERIFY(QMetaObject::invokeMethod(root, "collapse"));
    backend.setPreferredPage(0);
  }
  void allPagesKeepNavigationAndFooterInsideShape() {
    Backend backend(nullptr, false);
    QQmlApplicationEngine engine;
    engine.rootContext()->setContextProperty("island", &backend);
    engine.rootContext()->setContextProperty("previewMode", true);
    engine.rootContext()->setContextProperty("startExpanded", true);
    engine.load(QUrl("qrc:/qml/Main.qml"));
    QVERIFY(!engine.rootObjects().isEmpty());
    auto *root = qobject_cast<QQuickWindow *>(engine.rootObjects().first());
    root->show();
    QVERIFY(QTest::qWaitForWindowExposed(root));
    auto *surface = root->findChild<QQuickItem *>("islandSurface");
    auto *navigation = root->findChild<QQuickItem *>("islandNavigation");
    auto *tabs = root->findChild<QQuickItem *>("primaryTabs");
    auto *actions = root->findChild<QQuickItem *>("navigationActions");
    auto *footer = root->findChild<QQuickItem *>("audioFooter");
    auto *slider = root->findChild<QQuickItem *>("volumeSlider");
    QVERIFY(surface && navigation && tabs && actions && footer && slider);
    const QList<int> heights{216, 384, 432, 400, 336, 384, 384, 288};
    for (int page = 0; page < 8; ++page) {
      root->setProperty("page", page);
      QTRY_COMPARE(surface->height(), double(heights[page]));
      QTest::qWait(30);
      const auto top = navigation->mapToItem(surface, QPointF());
      QVERIFY2(top.y() >= 0,
               qPrintable(QString("Navigation clipped on page %1").arg(page)));
      QCOMPARE(footer->isVisible(), page == 0);
      QCOMPARE(slider->isVisible(), page == 0);
      const auto tabsLeft = tabs->mapToItem(navigation, QPointF()).x();
      QVERIFY2(qAbs(tabsLeft) < 1.0,
               qPrintable(QString("Primary tabs do not start at the content "
                                  "edge on page %1")
                              .arg(page)));
      const auto actionsRight =
          actions->mapToItem(navigation, QPointF(actions->width(), 0)).x();
      QVERIFY2(qAbs(actionsRight - navigation->width()) < 1.0,
               qPrintable(QString("Navigation actions are not right-aligned on page %1")
                              .arg(page)));
      if (page == 0) {
        const auto bottom = footer->mapToItem(
            surface, QPointF(footer->width(), footer->height()));
        QVERIFY2(bottom.y() <= surface->height() - 8,
                 "Music footer is clipped");
      }
    }
    root->setProperty("page", 3);
    QTest::qWait(30);
    auto *shortcuts = findVisualItem(root->contentItem(), "settingsShortcuts");
    QVERIFY(shortcuts);
    QList<QQuickItem *> shortcutButtons;
    for (auto *child : shortcuts->childItems())
      if (child->isVisible())
        shortcutButtons.append(child);
    QCOMPARE(shortcutButtons.size(), 3);
    for (auto *button : shortcutButtons)
      QVERIFY(qAbs(button->width() - shortcutButtons.first()->width()) < 1.0);
    root->setProperty("page", 7);
    QTest::qWait(40);
    // Repeater delegates are visual children, so locate them by walking the
    // page's item tree rather than QObject children.
    auto *tools = findVisualItem(root->contentItem(), "toolsPage");
    QVERIFY(tools);
    QList<QQuickItem *> cards;
    for (auto *child : tools->childItems())
      if (child->objectName() == "toolCard")
        cards.append(child);
    QCOMPARE(cards.size(), 3);
    for (auto *card : cards) {
      const auto bottom =
          card->mapToItem(surface, QPointF(0, card->height())).y();
      QVERIFY2(bottom <= surface->height() - 6,
               "A Tools card extends beyond the page content");
    }
    root->setProperty("page", 2);
    QTest::qWait(30);
    auto *grid = root->findChild<QQuickItem *>("systemTemperatures");
    QVERIFY(grid);
    QList<QQuickItem *> metricCards;
    for (auto *child : grid->childItems())
      if (child->objectName() == "metricCard")
        metricCards.append(child);
    if (metricCards.size() % 2 == 1) {
      auto *last = metricCards.last();
      QVERIFY2(qAbs(last->width() - grid->width()) < 1.0,
               "The odd final System card should span the centered row");
    }
    // Opening System details grows the surface back to the detailed size.
    root->setProperty("page", 2);
    auto *system = root->findChild<QQuickItem *>("systemTemperatures");
    QVERIFY(system);
    auto *systemPage = system;
    while (systemPage && !systemPage->property("details").isValid())
      systemPage = systemPage->parentItem();
    QVERIFY(systemPage);
    systemPage->setProperty("details", true);
    QTRY_COMPARE(surface->height(), 568.0);
    QTRY_COMPARE(surface->width(), 560.0);
    QTest::qWait(30);
    QVERIFY2(
        footer->mapToItem(surface, QPointF(footer->width(), footer->height()))
                .y() <= surface->height() - 8,
        "Footer clipped on the expanded-details System page");
    systemPage->setProperty("details", false);
    QTRY_COMPARE(surface->height(), 432.0);
    root->setProperty("page", 4);
    QTest::qWait(30);
    auto *awakeChoices =
        findVisualItem(root->contentItem(), "awakeDurationChoices");
    QVERIFY(awakeChoices);
    const auto awakeCenter =
        awakeChoices->mapToItem(awakeChoices->parentItem(),
                                QPointF(awakeChoices->width() / 2, 0))
            .x();
    QVERIFY(qAbs(awakeCenter - awakeChoices->parentItem()->width() / 2) <
            1.0);
    for (const auto &name : {"awakePage", "tasksPage", "notificationsPage"}) {
      auto *page = root->findChild<QObject *>(name);
      QVERIFY(page);
      QVERIFY(QMetaObject::invokeMethod(page, "backRequested"));
      QCOMPARE(root->property("page").toInt(), 7);
    }
    root->setProperty("page", 1);
    QTest::qWait(30);
    auto *modeOptions =
        findVisualItem(root->contentItem(), "focusModeOptions");
    QVERIFY(modeOptions);
    const auto modeCenter =
        modeOptions->mapToItem(modeOptions->parentItem(),
                               QPointF(modeOptions->width() / 2, 0))
            .x();
    QVERIFY(qAbs(modeCenter - modeOptions->parentItem()->width() / 2) < 1.0);
    auto *durationControls =
        root->findChild<QQuickItem *>("focusDurationControls");
    QVERIFY(durationControls);
    const auto durationCenter =
        durationControls->mapToItem(durationControls->parentItem(),
                                    QPointF(durationControls->width() / 2, 0))
            .x();
    QVERIFY(qAbs(durationCenter -
                 durationControls->parentItem()->width() / 2) < 1.0);
    auto *mode = root->findChild<QObject *>("pomodoroMode");
    auto *start = root->findChild<QObject *>("startFocus");
    QVERIFY(mode && start);
    QVERIFY(QMetaObject::invokeMethod(mode, "clicked"));
    QVERIFY(mode->property("checked").toBool());
    QTRY_COMPARE(surface->height(), 384.0);
    QTest::qWait(30);
    auto *breakControls =
        root->findChild<QQuickItem *>("focusPomodoroControls");
    QVERIFY(breakControls);
    const auto breakCenter =
        breakControls->mapToItem(breakControls->parentItem(),
                                 QPointF(breakControls->width() / 2, 0))
            .x();
    QVERIFY(qAbs(breakCenter - breakControls->parentItem()->width() / 2) <
            1.0);
    auto *startItem = qobject_cast<QQuickItem *>(start);
    auto end = startItem->mapToItem(surface, QPointF(0, startItem->height()));
    QVERIFY2(end.y() <= surface->height() - 12,
             "Pomodoro controls overlap the bottom safe area");
    QVERIFY(QMetaObject::invokeMethod(start, "clicked"));
    QCOMPARE(backend.focus()->snapshot()["state"].toString(),
             QString("running"));
    QVERIFY(backend.focus()->snapshot()["pomodoro"].toBool());
    backend.cancelTimer();
    QVERIFY(QMetaObject::invokeMethod(root, "collapse"));
    QVERIFY(!root->property("expanded").toBool());
    backend.setPreferredPage(0);
  }
  void messageRowGrowsTheSurfaceInsteadOfClipping() {
    Backend backend(nullptr, false);
    QQmlApplicationEngine engine;
    engine.rootContext()->setContextProperty("island", &backend);
    engine.rootContext()->setContextProperty("previewMode", true);
    engine.rootContext()->setContextProperty("startExpanded", true);
    engine.load(QUrl("qrc:/qml/Main.qml"));
    QVERIFY(!engine.rootObjects().isEmpty());
    auto *root = qobject_cast<QQuickWindow *>(engine.rootObjects().first());
    root->show();
    QVERIFY(QTest::qWaitForWindowExposed(root));
    auto *surface = root->findChild<QQuickItem *>("islandSurface");
    auto *footer = root->findChild<QQuickItem *>("audioFooter");
    QVERIFY(surface && footer);
    QTRY_COMPARE(surface->height(), 216.0);
    const auto before =
        footer->mapToItem(surface, QPointF(0, footer->height())).y();
    backend.setMessage("The player could not complete this action.");
    QTRY_VERIFY(surface->height() > 216.0);
    QTRY_VERIFY(surface->height() < 264.0);
    // The error grows the island; the player keeps its exact bands.
    QTRY_COMPARE(footer->mapToItem(surface, QPointF(0, footer->height())).y(),
                 before);
  }
  void escapeStepsBackToToolsBeforeCollapsing() {
    Backend backend(nullptr, false);
    QQmlApplicationEngine engine;
    engine.rootContext()->setContextProperty("island", &backend);
    engine.rootContext()->setContextProperty("previewMode", true);
    engine.rootContext()->setContextProperty("startExpanded", true);
    engine.load(QUrl("qrc:/qml/Main.qml"));
    QVERIFY(!engine.rootObjects().isEmpty());
    auto *root = qobject_cast<QQuickWindow *>(engine.rootObjects().first());
    root->show();
    QVERIFY(QTest::qWaitForWindowExposed(root));
    QTest::qWait(350);
    for (int page = 4; page <= 6; ++page) {
      root->setProperty("page", page);
      root->setProperty("expanded", true);
      QTest::qWait(30);
      QTest::keyClick(root, Qt::Key_Escape);
      QTRY_COMPARE(root->property("page").toInt(), 7);
      QVERIFY2(root->property("expanded").toBool(),
               qPrintable(QString("Escape on page %1 collapsed instead of "
                                  "returning to Tools")
                              .arg(page)));
    }
    QTest::keyClick(root, Qt::Key_Escape);
    QTRY_VERIFY(!root->property("expanded").toBool());
    backend.setPreferredPage(0);
  }
  void animationsOffMeansNoTransitions() {
    const auto oldOrganization = QCoreApplication::organizationName();
    const auto oldApplication = QCoreApplication::applicationName();
    QTemporaryDir config;
    const auto restore = qScopeGuard([&] {
      QCoreApplication::setOrganizationName(oldOrganization);
      QCoreApplication::setApplicationName(oldApplication);
    });
    QCoreApplication::setOrganizationName("KestrelRegressionTests");
    QCoreApplication::setApplicationName("NoMotion");
    QSettings::setPath(QSettings::NativeFormat, QSettings::UserScope,
                       config.path());
    QSettings().setValue("animations", false);
    Backend backend(nullptr, false);
    QVERIFY(!backend.animations());
    QQmlApplicationEngine engine;
    engine.rootContext()->setContextProperty("island", &backend);
    engine.rootContext()->setContextProperty("previewMode", true);
    engine.rootContext()->setContextProperty("startExpanded", true);
    engine.load(QUrl("qrc:/qml/Main.qml"));
    QVERIFY(!engine.rootObjects().isEmpty());
    auto *root = qobject_cast<QQuickWindow *>(engine.rootObjects().first());
    root->setProperty("page", 0);
    root->show();
    QVERIFY(QTest::qWaitForWindowExposed(root));
    QTest::qWait(60);
    // Repeater delegates are visual children only, so walk the nested tabs
    // row rather than the QObject tree.
    QQuickItem *focusTab = nullptr;
    auto *nav = root->findChild<QQuickItem *>("islandNavigation");
    auto *tabs = root->findChild<QQuickItem *>("primaryTabs");
    QVERIFY(nav && tabs);
    for (auto *item : tabs->childItems())
      if (item->property("label").toString() == "Focus")
        focusTab = item;
    QVERIFY(focusTab);
    QVERIFY(!focusTab->property("selected").toBool());
    root->setProperty("page", 1);
    QTest::qWait(60);
    QVERIFY(focusTab->property("selected").toBool());
    const auto settled = focusTab->property("implicitWidth").toDouble();
    QVERIFY2(settled > 60,
             "The selected tab should already be showing its label.");
    QTest::qWait(300);
    QCOMPARE(focusTab->property("implicitWidth").toDouble(), settled);
    root->setProperty("page", 0);
    QTest::qWait(60);
    QCOMPARE(focusTab->property("implicitWidth").toDouble(), 34.0);
    backend.setPreferredPage(0);
  }
  void rejectsUnknownDisplayAndLoadsMoveTargets() {
    Backend backend(nullptr, false);
    const auto original = backend.display();
    backend.chooseDisplay("not-a-connected-display");
    QCOMPARE(backend.display(), original);
    backend.setTopGap(-10);
    QCOMPARE(backend.topGap(), 0);
    backend.setTopGap(200);
    QCOMPARE(backend.topGap(), 120);
    backend.setTopGap(0);
    QQmlEngine engine;
    engine.rootContext()->setContextProperty("island", &backend);
    QQmlComponent component(&engine, QUrl("qrc:/qml/MoveTarget.qml"));
    QScopedPointer<QObject> target(component.createWithInitialProperties(
        {{"displayName", "Test display"}}));
    QVERIFY2(target, qPrintable(component.errorString()));
    QVERIFY(target->findChild<QObject *>("moveDropArea"));
  }
  void tintedCoverColorsIsland() {
    auto bus = QDBusConnection::sessionBus();
    if (!bus.isConnected())
      QSKIP("A session bus is required; run with dbus-run-session.");
    QTemporaryDir directory;
    const auto coverPath = directory.filePath("cover.png");
    QImage cover(64, 64, QImage::Format_RGB32);
    cover.fill(QColor(235, 130, 90));
    QVERIFY(cover.save(coverPath));
    const QString service = "org.mpris.MediaPlayer2.KestrelTintTest";
    QVERIFY(bus.registerService(service));
    FakePlayer player;
    player.art = "file://" + coverPath;
    QVERIFY(bus.registerObject("/org/mpris/MediaPlayer2", &player,
                               QDBusConnection::ExportAllProperties |
                                   QDBusConnection::ExportAllSlots));
    const auto cleanup = qScopeGuard([&] {
      bus.unregisterObject("/org/mpris/MediaPlayer2");
      bus.unregisterService(service);
    });
    Backend backend;
    QTRY_COMPARE(backend.media()["service"].toString(), service);
    const auto tint = backend.media()["tint"].toString();
    QVERIFY2(tint.startsWith("#") && tint != "#000000",
             qPrintable("Expected a saturated cover tint, got " + tint));
    QQmlApplicationEngine engine;
    engine.rootContext()->setContextProperty("island", &backend);
    engine.rootContext()->setContextProperty("previewMode", true);
    engine.rootContext()->setContextProperty("startExpanded", true);
    engine.load(QUrl("qrc:/qml/Main.qml"));
    QVERIFY(!engine.rootObjects().isEmpty());
    auto *root = qobject_cast<QQuickWindow *>(engine.rootObjects().first());
    root->setProperty("page", 0);
    root->show();
    QVERIFY(QTest::qWaitForWindowExposed(root));
    auto *artwork = root->findChild<QQuickItem *>("compactArtwork");
    QVERIFY(artwork);
    QTRY_COMPARE(artwork->property("status").toInt(), 1); // Image.Ready
    QTest::qWait(400);
    const auto shot = QDir::temp().filePath("kestrel-ui/music-tinted.png");
    QDir().mkpath(QFileInfo(shot).absolutePath());
    QVERIFY2(root->grabWindow().save(shot), qPrintable(shot));
    qInfo() << "tinted screenshot:" << shot;
  }
  void emptyMedia() {
    auto result = Backend::normalizeMedia({}, {});
    QVERIFY(!result["available"].toBool());
    QVERIFY(!result["canPlay"].toBool());
    QCOMPARE(result["title"].toString(), "Nothing playing");
  }
  void metadataAndCapabilities() {
    QVariantMap meta{{"xesam:title", "Test song"},
                     {"xesam:artist", QStringList{"One", "Two"}}};
    QVariantMap properties{{"Metadata", meta},   {"PlaybackStatus", "Playing"},
                           {"CanControl", true}, {"CanPlay", true},
                           {"CanPause", false},  {"CanGoNext", true}};
    auto result = Backend::normalizeMedia(
        "org.mpris.MediaPlayer2.test.instance1", properties);
    QCOMPARE(result["title"].toString(), "Test song");
    QCOMPARE(result["artist"].toString(), "One, Two");
    QVERIFY(result["playing"].toBool());
    QVERIFY(result["canPlay"].toBool());
    QVERIFY(!result["canPause"].toBool());
    QVERIFY(result["canNext"].toBool());
    properties["CanControl"] = false;
    result = Backend::normalizeMedia("org.mpris.MediaPlayer2.test", properties);
    QVERIFY(!result["canPlay"].toBool());
    QVERIFY(!result["canNext"].toBool());
  }
  void wrappedMetadata() {
    QVariantMap meta{
        {"xesam:title", QVariant::fromValue(QDBusVariant("Wrapped song"))}};
    auto result = Backend::normalizeMedia(
        "org.mpris.MediaPlayer2.test",
        {{"Metadata", QVariant::fromValue(QDBusVariant(meta))}});
    QCOMPARE(result["title"].toString(), "Wrapped song");
  }
  void automaticallySelectsCurrentPlayback() {
    QVariantMap browser{
        {"service", "org.mpris.MediaPlayer2.chromium.instance1"},
        {"playing", false},
        {"activity", 99}};
    QVariantMap integration{
        {"service", "org.mpris.MediaPlayer2.plasma-browser-integration"},
        {"playing", true},
        {"activity", 1}};
    QCOMPARE(Backend::currentMedia({browser, integration},
                                   browser["service"].toString())["service"],
             integration["service"]);
    browser["playing"] = true;
    browser["activity"] = 1;
    QCOMPARE(Backend::currentMedia({integration, browser})["service"],
             integration["service"]);
    QCOMPARE(Backend::currentMedia({browser, integration})["service"],
             integration["service"]);
    QVariantMap music{{"service", "org.mpris.MediaPlayer2.spotify"},
                      {"playing", true},
                      {"activity", 2}};
    QCOMPARE(
        Backend::currentMedia({browser, integration, music},
                              integration["service"].toString())["service"],
        music["service"]);
    music["playing"] = false;
    QCOMPARE(Backend::currentMedia({music, integration},
                                   music["service"].toString())["service"],
             integration["service"]);
    QCOMPARE(Backend::currentMedia({browser})["service"], browser["service"]);
    QVERIFY(!Backend::currentMedia({})["available"].toBool());
  }
  void timerPauseResumeCancel() {
    Backend backend(nullptr, false);
    backend.startTimer(1);
    QVERIFY(backend.timerRunning());
    QCOMPARE(backend.remaining(), 60);
    QTest::qWait(1100);
    backend.pauseTimer();
    QVERIFY(!backend.timerRunning());
    QCOMPARE(backend.remaining(), 59);
    QTest::qWait(1100);
    QCOMPARE(backend.remaining(), 59);
    backend.pauseTimer();
    QVERIFY(backend.timerRunning());
    backend.cancelTimer();
    QCOMPARE(backend.remaining(), 0);
    QVERIFY(!backend.timerRunning());
  }
  void rejectsInvalidTimerDurations() {
    Backend backend(nullptr, false);
    backend.startTimer(0);
    QCOMPARE(backend.remaining(), 0);
    backend.startTimer(-1);
    QCOMPARE(backend.remaining(), 0);
    backend.startTimer(181);
    QCOMPARE(backend.remaining(), 0);
  }
  void realDbusMediaRoundTrip() {
    auto bus = QDBusConnection::sessionBus();
    if (!bus.isConnected())
      QSKIP("A session bus is required; run with dbus-run-session.");
    const QString service = "org.mpris.MediaPlayer2.KestrelIntegrationTest";
    QVERIFY(bus.registerService(service));
    FakePlayer player;
    QVERIFY(bus.registerObject("/org/mpris/MediaPlayer2", &player,
                               QDBusConnection::ExportAllProperties |
                                   QDBusConnection::ExportAllSlots));
    {
      Backend backend;
      QTRY_COMPARE_WITH_TIMEOUT(backend.media().value("service").toString(),
                                service, 5000);
      QCOMPARE(backend.media().value("title").toString(),
               "Integration test song");
      QCOMPARE(backend.media().value("artist").toString(), "Test artist");
      QVERIFY(backend.media().value("playing").toBool());
      backend.mediaAction("PlayPause");
      QTRY_VERIFY(!player.playing);
      QTRY_VERIFY(!backend.media().value("playing").toBool());
      backend.mediaAction("Next");
      QTRY_COMPARE(player.skips, 1);
      QTRY_COMPARE(backend.media()["durationUs"].toLongLong(),
                   qlonglong(180000000));
      const auto dragGeneration = backend.seekGeneration();
      backend.seek(90, service, "/track/one", dragGeneration);
      QTRY_COMPARE(player.seeks, 1);
      QCOMPARE(player.currentPosition, qlonglong(90000000));
      player.track = "/track/two";
      player.publish();
      QTRY_COMPARE_WITH_TIMEOUT(backend.media()["trackId"].toString(),
                                QString("/track/two"), 2000);
      backend.seek(120, service, "/track/one", dragGeneration);
      QTest::qWait(40);
      QCOMPARE(player.seeks, 1);
      const auto beforeReplacement = backend.seekGeneration();
      auto replacementBus = QDBusConnection::connectToBus(
          QDBusConnection::SessionBus, "replacement-player");
      FakePlayer replacement;
      replacement.track = "/track/two";
      QVERIFY(
          replacementBus.registerObject("/org/mpris/MediaPlayer2", &replacement,
                                        QDBusConnection::ExportAllProperties |
                                            QDBusConnection::ExportAllSlots));
      QVERIFY(bus.unregisterService(service));
      QVERIFY(replacementBus.registerService(service));
      QTRY_VERIFY(backend.seekGeneration() != beforeReplacement);
      QTRY_COMPARE(backend.media()["owner"].toString(),
                   replacementBus.baseService());
      backend.seek(120, service, "/track/two", beforeReplacement);
      QTest::qWait(40);
      QCOMPARE(replacement.seeks, 0);
      backend.seek(120, service, "/track/two", backend.seekGeneration());
      QTRY_COMPARE(replacement.seeks, 1);
      replacementBus.unregisterObject("/org/mpris/MediaPlayer2");
      replacementBus.unregisterService(service);
      QDBusConnection::disconnectFromBus("replacement-player");
      QVERIFY(backend.cpu() >= 0 && backend.cpu() <= 100);
      QVERIFY(backend.memory() > 0 && backend.memory() <= 100);
    }
    bus.unregisterObject("/org/mpris/MediaPlayer2");
    bus.unregisterService(service);
  }
  void shuffleAndRepeatRoundTrip() {
    auto bus = QDBusConnection::sessionBus();
    if (!bus.isConnected())
      QSKIP("A session bus is required; run with dbus-run-session.");
    const QString service = "org.mpris.MediaPlayer2.KestrelShuffleTest";
    QVERIFY(bus.registerService(service));
    FakePlayer player;
    QVERIFY(bus.registerObject("/org/mpris/MediaPlayer2", &player,
                               QDBusConnection::ExportAllProperties |
                                   QDBusConnection::ExportAllSlots));
    const auto cleanup = qScopeGuard([&] {
      bus.unregisterObject("/org/mpris/MediaPlayer2");
      bus.unregisterService(service);
    });
    Backend backend;
    QTRY_COMPARE_WITH_TIMEOUT(backend.media().value("service").toString(),
                              service, 5000);
    QVERIFY(backend.media().value("canShuffle").toBool());
    QVERIFY(backend.media().value("canLoop").toBool());
    backend.setShuffle(true);
    QTRY_VERIFY(player.shuffleOn);
    QTRY_VERIFY(backend.media().value("shuffle").toBool());
    backend.cycleLoop();
    QTRY_COMPARE(player.loop, QString("Playlist"));
    QTRY_COMPARE(backend.media().value("loop").toString(), QString("Playlist"));
    backend.cycleLoop();
    QTRY_COMPARE(player.loop, QString("Track"));
    backend.cycleLoop();
    QTRY_COMPARE(player.loop, QString("None"));
  }
  void pointerDragsReachMediaAndVolume() {
    QTemporaryDir directory;
    QFile script(directory.filePath("pactl"));
    QVERIFY(script.open(QIODevice::WriteOnly));
    script.write(
        "#!/bin/sh\nprintf '%s\\n' \"$*\" >> \"$KESTREL_TEST_AUDIO_LOG\"\ncase "
        "\"$*\" in\n*get-sink-volume*) echo "
        "'{\"volume\":{\"mono\":{\"value\":32768}}}';;\n*get-sink-mute*) echo "
        "'{\"mute\":false}';;\nsubscribe) sleep 1;;\nesac\n");
    script.close();
    QVERIFY(script.setPermissions(QFileDevice::ReadOwner |
                                  QFileDevice::WriteOwner |
                                  QFileDevice::ExeOwner));
    const auto oldPath = qgetenv("PATH"),
               oldLog = qgetenv("KESTREL_TEST_AUDIO_LOG");
    const auto restore = qScopeGuard([&] {
      qputenv("PATH", oldPath);
      qputenv("KESTREL_TEST_AUDIO_LOG", oldLog);
    });
    qputenv("PATH", directory.path().toUtf8() + ":" + oldPath);
    qputenv("KESTREL_TEST_AUDIO_LOG", directory.filePath("calls").toUtf8());
    auto bus = QDBusConnection::sessionBus();
    const QString service = "org.mpris.MediaPlayer2.KestrelPointerTest";
    QVERIFY(bus.registerService(service));
    QVERIFY(bus.registerService("org.freedesktop.portal.Desktop"));
    FakePlayer player;
    QVERIFY(bus.registerObject("/org/mpris/MediaPlayer2", &player,
                               QDBusConnection::ExportAllProperties |
                                   QDBusConnection::ExportAllSlots));
    const auto cleanup = qScopeGuard([&] {
      bus.unregisterObject("/org/mpris/MediaPlayer2");
      bus.unregisterService(service);
      bus.unregisterService("org.freedesktop.portal.Desktop");
    });
    Backend backend;
    QTRY_VERIFY(backend.audioAvailable());
    QTRY_COMPARE(backend.media()["service"].toString(), service);
    QQmlApplicationEngine engine;
    engine.rootContext()->setContextProperty("island", &backend);
    engine.rootContext()->setContextProperty("previewMode", true);
    engine.rootContext()->setContextProperty("startExpanded", true);
    engine.load(QUrl("qrc:/qml/Main.qml"));
    QVERIFY(!engine.rootObjects().isEmpty());
    auto *root = qobject_cast<QQuickWindow *>(engine.rootObjects().first());
    root->setProperty("page", 0);
    root->show();
    QVERIFY(QTest::qWaitForWindowExposed(root));
    QTest::qWait(350);
    int finalVolume = 0;
    for (const auto &name : {"mediaTimeline", "volumeSlider"}) {
      auto *slider = root->findChild<QQuickItem *>(name);
      QVERIFY(slider);
      QVERIFY(slider->isEnabled());
      QVERIFY2(slider->height() >= 24,
               "A visible slider needs a real pointer hit area");
      const auto start =
          slider
              ->mapToScene(QPointF(slider->width() * 0.4, slider->height() / 2))
              .toPoint();
      const auto end =
          slider
              ->mapToScene(QPointF(slider->width() * 0.8, slider->height() / 2))
              .toPoint();
      const auto click = slider
                             ->mapToScene(QPointF(slider->width() * 0.25,
                                                  slider->height() / 2))
                             .toPoint();
      QTest::mouseClick(root, Qt::LeftButton, Qt::NoModifier, click);
      if (QString(name) == "mediaTimeline")
        QTRY_VERIFY(player.seeks > 0);
      QTest::mousePress(root, Qt::LeftButton, Qt::NoModifier, start);
      QVERIFY2(slider->property("pressed").toBool(), name);
      QTest::mouseMove(root, end, 20);
      QTest::mouseRelease(root, Qt::LeftButton, Qt::NoModifier, end);
      if (QString(name) == "volumeSlider")
        finalVolume = slider->property("requestedVolume").toInt();
    }
    QVERIFY(finalVolume > 70 && finalVolume < 90);
    QTRY_VERIFY(player.seeks > 0);
    QVERIFY(player.currentPosition > 100000000);
    auto calls = [&] {
      QFile file(directory.filePath("calls"));
      if (!file.open(QIODevice::ReadOnly))
        return QByteArray();
      return file.readAll();
    };
    QTRY_VERIFY(calls().contains("set-sink-volume @DEFAULT_SINK@ " +
                                 QByteArray::number(finalVolume) + "%"));
  }
  void coalescesVolumeWithoutLosingFinalValue() {
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const auto executable = directory.filePath("pactl");
    const auto log = directory.filePath("calls");
    QFile script(executable);
    QVERIFY(script.open(QIODevice::WriteOnly));
    script.write("#!/bin/sh\nprintf '%s\\n' \"$*\" >> "
                 "\"$KESTREL_TEST_AUDIO_LOG\"\nsleep 0.25\n");
    script.close();
    QVERIFY(script.setPermissions(QFileDevice::ReadOwner |
                                  QFileDevice::WriteOwner |
                                  QFileDevice::ExeOwner));
    const auto oldPath = qgetenv("PATH");
    const auto oldLog = qgetenv("KESTREL_TEST_AUDIO_LOG");
    const auto restore = qScopeGuard([&] {
      qputenv("PATH", oldPath);
      qputenv("KESTREL_TEST_AUDIO_LOG", oldLog);
    });
    qputenv("PATH", directory.path().toUtf8() + ":" + oldPath);
    qputenv("KESTREL_TEST_AUDIO_LOG", log.toUtf8());
    Backend backend(nullptr, false);
    backend.setVolume(20);
    backend.setVolume(40);
    backend.setVolume(75);
    auto calls = [&] {
      QFile file(log);
      if (!file.open(QIODevice::ReadOnly))
        return QByteArray();
      return file.readAll();
    };
    QTRY_VERIFY_WITH_TIMEOUT(
        calls().contains("set-sink-volume @DEFAULT_SINK@ 75%"), 3000);
    QVERIFY(calls().contains("set-sink-volume @DEFAULT_SINK@ 20%"));
    QVERIFY(!calls().contains("set-sink-volume @DEFAULT_SINK@ 40%"));
  }
  void quickSliderReleasePreservesRequestedVolume() {
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    QFile script(directory.filePath("pactl"));
    QVERIFY(script.open(QIODevice::WriteOnly));
    script.write(
        "#!/bin/sh\nprintf '%s\\n' \"$*\" >> \"$KESTREL_TEST_AUDIO_LOG\"\n");
    script.close();
    QVERIFY(script.setPermissions(QFileDevice::ReadOwner |
                                  QFileDevice::WriteOwner |
                                  QFileDevice::ExeOwner));
    const auto oldPath = qgetenv("PATH");
    const auto oldLog = qgetenv("KESTREL_TEST_AUDIO_LOG");
    const auto restore = qScopeGuard([&] {
      qputenv("PATH", oldPath);
      qputenv("KESTREL_TEST_AUDIO_LOG", oldLog);
    });
    qputenv("PATH", directory.path().toUtf8() + ":" + oldPath);
    qputenv("KESTREL_TEST_AUDIO_LOG", directory.filePath("calls").toUtf8());
    Backend backend(nullptr, false);
    QQmlApplicationEngine engine;
    engine.rootContext()->setContextProperty("island", &backend);
    engine.rootContext()->setContextProperty("previewMode", true);
    engine.rootContext()->setContextProperty("startExpanded", true);
    engine.load(QUrl("qrc:/qml/Main.qml"));
    QVERIFY(!engine.rootObjects().isEmpty());
    auto *slider =
        engine.rootObjects().first()->findChild<QQuickItem *>("volumeSlider");
    QVERIFY(slider);
    slider->setProperty("pressed", true);
    slider->setProperty("value", 75);
    QVERIFY(QMetaObject::invokeMethod(slider, "moved"));
    QCOMPARE(slider->property("requestedVolume").toInt(), 75);
    slider->setProperty("pressed", false);
    emit backend.volumeChanged();
    auto calls = [&] {
      QFile file(directory.filePath("calls"));
      if (!file.open(QIODevice::ReadOnly))
        return QByteArray();
      return file.readAll();
    };
    QTRY_VERIFY_WITH_TIMEOUT(
        calls().contains("set-sink-volume @DEFAULT_SINK@ 75%"), 2000);
  }
  void microphoneConfirmedStateIsVisible() {
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    QFile script(directory.filePath("pactl"));
    QVERIFY(script.open(QIODevice::WriteOnly));
    script.write("#!/bin/sh\ncase \"$*\" in\n*get-source-mute*) echo "
                 "'{\"mute\":true}';;\n*get-sink-mute*) echo "
                 "'{\"mute\":false}';;\n*get-sink-volume*) echo "
                 "'{\"volume\":{\"mono\":{\"value\":32768}}}';;\nsubscribe) "
                 "sleep 2;;\nesac\n");
    script.close();
    QVERIFY(script.setPermissions(QFileDevice::ReadOwner |
                                  QFileDevice::WriteOwner |
                                  QFileDevice::ExeOwner));
    const auto path = qgetenv("PATH");
    const auto restore = qScopeGuard([&] { qputenv("PATH", path); });
    qputenv("PATH", directory.path().toUtf8() + ":" + path);
    Backend backend;
    backend.setAudioFeedback(true);
    QTRY_VERIFY(backend.micAvailable());
    QVERIFY(backend.micMuted());
    QQmlApplicationEngine engine;
    engine.rootContext()->setContextProperty("island", &backend);
    engine.rootContext()->setContextProperty("previewMode", true);
    engine.rootContext()->setContextProperty("startExpanded", true);
    engine.load(QUrl("qrc:/qml/Main.qml"));
    QVERIFY(!engine.rootObjects().isEmpty());
    auto *button = engine.rootObjects().first()->findChild<QObject *>(
        "microphoneMuteButton");
    QVERIFY(button);
    QCOMPARE(button->property("symbol").toString(), "microphone-muted");
    auto *icon = button->findChild<QObject *>("buttonIcon");
    QVERIFY(icon);
    QTRY_COMPARE(icon->property("status").toInt(), 1);
    backend.setAudioFeedback(false);
  }
};
QTEST_MAIN(BackendTest)
#include "test_backend.moc"
