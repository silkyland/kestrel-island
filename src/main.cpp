#include "backend.h"
#include "islandcontrol.h"
#include <LayerShellQt/Window>
#include <QApplication>
#include <QCommandLineParser>
#include <QDBusAbstractAdaptor>
#include <QDBusConnection>
#include <QDBusInterface>
#include <QDBusReply>
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QLockFile>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickItem>
#include <QQuickWindow>
#include <QScreen>
#include <QStandardPaths>
#include <QSystemTrayIcon>

int main(int argc, char **argv) {
  QApplication app(argc, argv);
  app.setApplicationName("Kestrel Island");
  app.setOrganizationName("Kestrel");
  app.setApplicationVersion("0.3.8");
  app.setDesktopFileName("io.github.kestrel.Island");
  app.setWindowIcon(QIcon(":/assets/island.svg"));
  app.setQuitOnLastWindowClosed(false);
  QCommandLineParser parser;
  parser.setApplicationDescription("A Dynamic Island for KDE Plasma");
  parser.addOption({"task-event-file",
                    "Send a task JSON event to an existing instance.", "path"});
  parser.addHelpOption();
  parser.addVersionOption();
  parser.addOption(
      {"screenshot",
       "Save an isolated preview PNG then exit (no desktop changes).",
       "directory"});
  parser.addOption({"expanded", "Open expanded for preview."});
  parser.addOption({"page",
                    "Start on music, focus, system, tools, settings, awake, "
                    "tasks or notifications.",
                    "page", "music"});
  parser.addOption({"check-session",
                    "Check the real desktop integration, print JSON, and "
                    "exit after four seconds."});
  parser.addOption(
      {"screen", "Place the island on the named display.", "name"});
  parser.addOption({"cycle-displays",
                    "With --check-session, remap across displays and "
                    "restore the selected display."});
  parser.addOption(
      {"check-keepawake",
       "With --check-session, briefly acquire and release power inhibition."});
  parser.process(app);
  if (parser.isSet("cycle-displays") && !parser.isSet("check-session")) {
    fprintf(stderr, "--cycle-displays requires --check-session.\n");
    return 1;
  }
  if (parser.isSet("check-keepawake") && !parser.isSet("check-session")) {
    fprintf(stderr, "--check-keepawake requires --check-session.\n");
    return 2;
  }
  const bool preview = parser.isSet("screenshot");
  auto bus = QDBusConnection::sessionBus();
  if (!preview && !bus.isConnected()) {
    fprintf(stderr, "Cannot connect to the session bus.\n");
    return 2;
  }
  if (parser.isSet("task-event-file")) {
    QFile file(parser.value("task-event-file"));
    if (!file.open(QIODevice::ReadOnly) || file.size() > 16384) {
      fprintf(stderr, "Cannot read task event (maximum 16 KiB).\n");
      return 2;
    }
    QJsonParseError error;
    auto document = QJsonDocument::fromJson(file.read(16385), &error);
    if (error.error != QJsonParseError::NoError || !document.isObject())
      return 2;
    QDBusInterface existing("io.github.kestrel.Island", "/Island",
                            "io.github.kestrel.Island", bus);
    existing.setTimeout(1500);
    QDBusReply<bool> accepted =
        existing.call("ReportTask", document.object().toVariantMap());
    if (!accepted.isValid() || !accepted.value()) {
      fprintf(stderr,
              "Task event rejected: start Kestrel and enable task hooks.\n");
      return 3;
    }
    return 0;
  }
  if (!preview && !bus.registerService("io.github.kestrel.Island")) {
    if (parser.isSet("check-session")) {
      fprintf(stderr,
              "Quit the running instance before checking the session.\n");
      return 2;
    }
    QDBusInterface existing("io.github.kestrel.Island", "/Island",
                            "io.github.kestrel.Island", bus);
    existing.setTimeout(1200);
    const auto reply = existing.call("Toggle");
    if (reply.type() == QDBusMessage::ErrorMessage) {
      fprintf(stderr, "Could not contact the existing instance.\n");
      return 2;
    }
    return 0;
  }
  Backend backend(nullptr, !preview);
  IslandControl control(&backend);
  if (!preview)
    bus.registerObject("/Island", &control,
                       QDBusConnection::ExportScriptableSlots);
  QSystemTrayIcon tray(QIcon(":/assets/island.svg"));
  tray.setToolTip("Kestrel Island — click to expand");
  QQmlApplicationEngine engine;
  engine.rootContext()->setContextProperty("island", &backend);
  engine.rootContext()->setContextProperty("previewMode", preview);
  engine.rootContext()->setContextProperty("startExpanded",
                                           parser.isSet("expanded"));
  engine.load(QUrl("qrc:/qml/Main.qml"));
  if (engine.rootObjects().isEmpty())
    return 1;
  auto *window = qobject_cast<QQuickWindow *>(engine.rootObjects().first());
  if (!window)
    return 1;
  const auto page = parser.value("page");
  if (page == "focus")
    window->setProperty("page", 1);
  else if (page == "system")
    window->setProperty("page", 2);
  else if (page == "settings")
    window->setProperty("page", 3);
  else if (page == "awake")
    window->setProperty("page", 4);
  else if (page == "tasks")
    window->setProperty("page", 5);
  else if (page == "notifications")
    window->setProperty("page", 6);
  else if (page == "tools")
    window->setProperty("page", 7);
  else if (page == "music" && (parser.isSet("page") || preview))
    window->setProperty("page", 0);
  else if (page != "music") {
    fprintf(stderr,
            "Unknown page; use music, focus, system, settings, awake, tasks, "
            "notifications or tools.\n");
    return 1;
  }
  if (preview && page == "system")
    backend.previewSystem();
  if (!preview) {
    backend.attach(window, &tray);
    if (parser.isSet("screen"))
      backend.chooseDisplay(parser.value("screen"));
    QObject::connect(&tray, &QSystemTrayIcon::activated, &backend,
                     [&backend](QSystemTrayIcon::ActivationReason reason) {
                       if (reason == QSystemTrayIcon::Trigger)
                         emit backend.toggleRequested();
                     });
    tray.show();
  }
  window->show();
  bool keepAwakeAcquired = false, keepAwakeReleased = false;
  if (parser.isSet("check-keepawake")) {
    QObject::connect(backend.awake(), &KeepAwakeService::changed, &app, [&] {
      const auto state = backend.awake()->snapshot().value("state").toString();
      if (state == "active" && !keepAwakeAcquired) {
        keepAwakeAcquired = true;
        QTimer::singleShot(200, backend.awake(), &KeepAwakeService::stop);
      }
      if (keepAwakeAcquired && state == "off")
        keepAwakeReleased = true;
    });
    QTimer::singleShot(300, &app, [&] { backend.awake()->start(1); });
  }
  if (parser.isSet("cycle-displays")) {
    const auto original = backend.display();
    const auto names = backend.displays();
    if (names.size() > 1) {
      QTimer::singleShot(700, &backend, [&backend, names] {
        backend.chooseDisplay(names[1]);
      });
      QTimer::singleShot(1600, &backend, [&backend, names] {
        backend.chooseDisplay(names[0]);
      });
      QTimer::singleShot(2500, &backend, [&backend, original] {
        backend.chooseDisplay(original);
      });
    }
  }
  if (parser.isSet("check-session")) {
    QTimer::singleShot(
        4000, &app,
        [&app, window, &backend, &parser, &keepAwakeAcquired,
         &keepAwakeReleased] {
          const auto *artwork = window->findChild<QObject *>("compactArtwork");
          QJsonObject status{
              {"platform", QGuiApplication::platformName()},
              {"screen", window->screen()->name()},
              {"exposed", window->isExposed()},
              {"page", window->property("page").toInt()},
              {"audioAvailable", backend.audioAvailable()},
              {"volume", backend.volume()},
              {"cpu", backend.cpu()},
              {"memory", backend.memory()},
              {"mediaAvailable", backend.media().value("available").toBool()},
              {"compactArtworkReady",
               artwork && artwork->property("status").toInt() == 1},
              {"statistics", QJsonObject::fromVariantMap(backend.statistics())},
              {"inputRegionWidth", window->mask().boundingRect().width()},
              {"inputRegionHeight", window->mask().boundingRect().height()}};
          if (QGuiApplication::platformName().startsWith("wayland")) {
            status.insert("keyboardInteractivity",
                          int(LayerShellQt::Window::get(window)
                                  ->keyboardInteractivity()));
            status.insert("presentationLayer",
                          int(LayerShellQt::Window::get(window)->layer()));
          }
          if (parser.isSet("check-keepawake")) {
            status.insert("keepAwakeAcquired", keepAwakeAcquired);
            status.insert("keepAwakeReleased", keepAwakeReleased);
          }
          const auto data =
              QJsonDocument(status).toJson(QJsonDocument::Compact);
          fprintf(stdout, "%s\n", data.constData());
          app.exit(window->isExposed() && !window->mask().isEmpty() &&
                           (!parser.isSet("check-keepawake") ||
                            (keepAwakeAcquired && keepAwakeReleased))
                       ? 0
                       : 3);
        });
  }
  if (preview) {
    const auto directory = parser.value("screenshot");
    QDir().mkpath(directory);
    QTimer::singleShot(1800, &app, [window, directory, &app] {
      auto frame = window->grabWindow();
      if (auto *surface = window->findChild<QQuickItem *>("islandSurface"))
        frame = frame.copy(0, 0, frame.width(),
                           qCeil(surface->height() * frame.devicePixelRatio()));
      const bool saved =
          frame.save(directory + "/" +
                     (window->property("expanded").toBool() ? "expanded.png"
                                                            : "compact.png"));
      app.exit(saved ? 0 : 2);
    });
  }
  return app.exec();
}
