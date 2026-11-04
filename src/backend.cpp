#include "backend.h"
#include <QClipboard>

#include <LayerShellQt/Window>
#include <QApplication>
#include <QDBusArgument>
#include <QDBusConnection>
#include <QDBusConnectionInterface>
#include <QDBusObjectPath>
#include <QDBusPendingCallWatcher>
#include <QDBusPendingReply>
#include <QDBusVariant>
#include <QDir>
#include <QDrag>
#include <QFile>
#include <QGuiApplication>
#include <QImage>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMenu>
#include <QMimeData>
#include <QPainterPath>
#include <QProcess>
#include <QProcessEnvironment>
#include <QQmlComponent>
#include <QQmlEngine>
#include <QQuickItem>
#include <QQuickWindow>
#include <QSaveFile>
#include <QScreen>
#include <QStandardPaths>
#include <QSystemTrayIcon>
#include <QtMath>
#include <cmath>
#include <tuple>

namespace {
QDBusMessage mediaCall(const QString &service, const QString &interface,
                       const QString &method) {
  return QDBusMessage::createMethodCall(service, "/org/mpris/MediaPlayer2",
                                        interface, method);
}
QVariant unwrap(const QVariant &value) {
  return value.metaType() == QMetaType::fromType<QDBusVariant>()
             ? value.value<QDBusVariant>().variant()
             : value;
}
} // namespace

Backend::Backend(QObject *parent, bool live)
    : QObject(parent),
      m_notifications(
          this, live,
          m_settings.value("notifications/validated", false).toBool()),
      m_live(live) {
  // KDE's global animation speed, read once at startup. A factor of 0
  // disables motion; otherwise durations scale inside [0.25, 1].
  const QSettings kde(QStandardPaths::writableLocation(
                          QStandardPaths::GenericConfigLocation) +
                          "/kdeglobals",
                      QSettings::IniFormat);
  const double factor = kde.value("KDE/AnimationDurationFactor", 1).toDouble();
  m_animationFactor = factor <= 0 ? 0 : qBound(0.25, factor, 1.0);
  qInfo() << "KDE AnimationDurationFactor" << factor << "-> animation scale"
          << m_animationFactor;
  m_media = normalizeMedia({}, {});
  connect(&m_system, &SystemMonitor::changed, this, [this] {
    auto data = m_system.snapshot();
    m_cpu = data["cpu"].toMap()["percent"].toDouble();
    m_memory = data["memory"].toMap()["percent"].toDouble();
    emit statsChanged();
  });
  m_poll.setInterval(10000);
  connect(&m_poll, &QTimer::timeout, this, [this] {
    pollMedia();
    pollStats();
    pollVolume();
    if (audioFeedback())
      pollMic();
  });
  m_mediaRefresh.setSingleShot(true);
  m_mediaRefresh.setInterval(80);
  connect(&m_mediaRefresh, &QTimer::timeout, this, &Backend::pollMedia);
  m_playbackTimer.setInterval(1000);
  connect(&m_playbackTimer, &QTimer::timeout, this, [this] {
    emit playbackTick();
    if (m_expanded)
      pollStats();
  });
  m_messageExpiry.setSingleShot(true);
  m_messageExpiry.setInterval(7000);
  connect(&m_messageExpiry, &QTimer::timeout, this, [this] {
    m_message.clear();
    emit messageChanged();
  });
  connect(&m_focus, &FocusSession::changed, this, [this] {
    const auto state = m_focus.snapshot();
    if (state["state"] == "idle")
      m_activity.remove("focus");
    else
      m_activity.upsert(
          {{"key", "focus"},
           {"title", state["state"] == "completed"
                         ? "Focus complete"
                         : QString("Focus %1:%2")
                               .arg(remaining() / 60, 2, 10, QChar('0'))
                               .arg(remaining() % 60, 2, 10, QChar('0'))},
           {"page", 1}});
    emit timerChanged();
  });
  connect(&m_focus, &FocusSession::finished, this, [this] {
    m_activity.show("focus-complete", "Focus complete · take a breath", 40,
                    6000);
    emit timerFinished();
  });
  connect(&m_awake, &KeepAwakeService::changed, this, [this] {
    auto state = m_awake.snapshot();
    if (state["state"] == "active")
      m_activity.upsert(
          {{"key", "awake"}, {"title", "Keeping awake"}, {"page", 4}});
    else
      m_activity.remove("awake");
  });
  m_tasks.setEnabled(live && m_settings.value("tasksEnabled", false).toBool());
  connect(&m_tasks, &TaskActivityService::changed, this, [this] {
    auto rows = m_tasks.tasks();
    if (rows.isEmpty())
      m_activity.remove("tasks");
    else
      m_activity.upsert(
          {{"key", "tasks"},
           {"title", QString("%1 task(s) · %2")
                         .arg(rows.size())
                         .arg(rows.first().toMap()["label"].toString())},
           {"page", 5}});
  });
  connect(&m_tasks, &TaskActivityService::notice, this,
          [this](auto id, auto title, int priority) {
            m_activity.show("task:" + id, title, priority, 5000);
          });
  m_notifications.configure(
      live && m_settings.value("notifications/enabled", false).toBool(),
      m_settings.value("notifications/apps").toStringList());
  connect(&m_notifications, &NotificationsService::changed, this, [this] {
    auto n = m_notifications.entries().size();
    if (n)
      m_activity.upsert({{"key", "notifications"},
                         {"title", QString("%1 notification(s)").arg(n)},
                         {"page", 6}});
    else
      m_activity.remove("notifications");
  });
  connect(this, &Backend::mediaChanged, this, [this] {
    m_mediaSample.restart();
    if (m_media["available"].toBool())
      m_activity.upsert(
          {{"key", "music"}, {"title", m_media["title"]}, {"page", 0}});
    else
      m_activity.remove("music");
    emit playbackTick();
  });
  m_audioRefresh.setSingleShot(true);
  m_audioRefresh.setInterval(100);
  connect(&m_audioRefresh, &QTimer::timeout, this, [this] {
    pollVolume();
    if (audioFeedback())
      pollMic();
  });
  m_audioRestart.setSingleShot(true);
  connect(&m_audioRestart, &QTimer::timeout, this,
          &Backend::startAudioObserver);
  if (live) {
    m_poll.start();
    m_playbackTimer.start();
    auto bus = QDBusConnection::sessionBus();
    bus.connect({}, "/org/mpris/MediaPlayer2",
                "org.freedesktop.DBus.Properties", "PropertiesChanged", this,
                SLOT(mediaProperties(QString, QVariantMap, QStringList)));
    bus.connect({}, "/org/mpris/MediaPlayer2", "org.mpris.MediaPlayer2.Player",
                "Seeked", this, SLOT(mediaSeeked(qlonglong)));
    bus.connect("org.freedesktop.DBus", "/org/freedesktop/DBus",
                "org.freedesktop.DBus", "NameOwnerChanged", this,
                SLOT(mediaOwnerChanged(QString, QString, QString)));
    if (audioFeedback())
      startAudioObserver();
    QTimer::singleShot(0, this, [this] {
      pollMedia();
      pollStats();
      pollVolume();
      if (audioFeedback())
        pollMic();
    });
  }
}

Backend::~Backend() {
  for (auto *process : findChildren<QProcess *>()) {
    process->disconnect(this);
    if (process->state() != QProcess::NotRunning) {
      process->kill();
      process->waitForFinished(1000);
    }
  }
  delete m_menu;
}

QVariantMap Backend::decodeProperties(const QDBusMessage &reply) {
  if (reply.type() == QDBusMessage::ErrorMessage || reply.arguments().isEmpty())
    return {};
  const auto first = reply.arguments().first();
  auto result = first.metaType() == QMetaType::fromType<QDBusArgument>()
                    ? qdbus_cast<QVariantMap>(first.value<QDBusArgument>())
                    : first.toMap();
  for (auto it = result.begin(); it != result.end(); ++it)
    it.value() = unwrap(it.value());
  return result;
}

// Vorssaint-style cover tint: the dominant colour of the local artwork,
// clamped to stay legible on black. Neutral or unreadable covers return an
// invalid colour so the UI keeps its default accent.
static QColor mediaTint(const QString &art) {
  static QString lastArt;
  static QColor lastTint;
  if (art == lastArt)
    return lastTint;
  lastArt = art;
  lastTint = QColor();
  if (!art.startsWith("file://"))
    return lastTint;
  QImage image(art.mid(7));
  if (image.isNull())
    return lastTint;
  image = image.scaled(8, 8, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
  double r = 0, g = 0, b = 0;
  const auto count = image.width() * image.height();
  for (int y = 0; y < image.height(); ++y)
    for (int x = 0; x < image.width(); ++x) {
      const auto pixel = image.pixelColor(x, y);
      r += pixel.redF();
      g += pixel.greenF();
      b += pixel.blueF();
    }
  const auto hsv = QColor::fromRgbF(r / count, g / count, b / count).toHsv();
  if (hsv.saturationF() < 0.18 || hsv.valueF() < 0.2)
    return lastTint;
  lastTint = QColor::fromHsvF(hsv.hueF(),
                              qBound(0.45, hsv.saturationF(), 0.8),
                              qBound(0.65, hsv.valueF(), 0.95));
  return lastTint;
}

QVariantMap Backend::normalizeMedia(const QString &service,
                                    const QVariantMap &properties) {
  auto raw = unwrap(properties.value("Metadata"));
  const QVariantMap metadata =
      raw.metaType() == QMetaType::fromType<QDBusArgument>()
          ? qdbus_cast<QVariantMap>(raw.value<QDBusArgument>())
          : raw.toMap();
  QStringList artists = unwrap(metadata.value("xesam:artist")).toStringList();
  const bool exists = !service.isEmpty();
  return {
      {"available", exists},
      {"service", service},
      {"title", unwrap(metadata.value("xesam:title")).toString().isEmpty()
                    ? (exists ? "Untitled media" : "Nothing playing")
                    : unwrap(metadata.value("xesam:title")).toString()},
      {"artist", artists.join(", ")},
      {"art", unwrap(metadata.value("mpris:artUrl")).toString()},
      {"tint",
       mediaTint(unwrap(metadata.value("mpris:artUrl")).toString()).name()},
      {"playing", properties.value("PlaybackStatus").toString() == "Playing"},
      {"canPlay", properties.value("CanControl").toBool() &&
                      properties.value("CanPlay").toBool()},
      {"canPause", properties.value("CanControl").toBool() &&
                       properties.value("CanPause").toBool()},
      {"canNext", properties.value("CanControl").toBool() &&
                      properties.value("CanGoNext").toBool()},
      {"canPrevious", properties.value("CanControl").toBool() &&
                          properties.value("CanGoPrevious").toBool()},
      {"durationUs", unwrap(metadata.value("mpris:length")).toLongLong()},
      {"trackId",
       unwrap(metadata.value("mpris:trackid")).value<QDBusObjectPath>().path()},
      {"positionUs", properties.value("Position").toLongLong()},
      {"rate", properties.value("Rate", 1.0).toDouble()},
      {"canSeek", properties.value("CanControl").toBool() &&
                      properties.value("CanSeek").toBool()},
      {"player", service.section('.', 3).section('.', 0, 0)}};
}

void Backend::pollMedia() {
  if (m_mediaBusy) {
    m_mediaDirty = true;
    return;
  }
  m_mediaDirty = false;
  m_mediaBusy = true;
  auto bus = QDBusConnection::sessionBus();
  auto namesCall = QDBusMessage::createMethodCall(
      "org.freedesktop.DBus", "/org/freedesktop/DBus", "org.freedesktop.DBus",
      "ListNames");
  auto *namesWatcher =
      new QDBusPendingCallWatcher(bus.asyncCall(namesCall, 1200), this);
  connect(
      namesWatcher, &QDBusPendingCallWatcher::finished, this,
      [this](QDBusPendingCallWatcher *watcher) {
        QDBusPendingReply<QStringList> reply = *watcher;
        watcher->deleteLater();
        QStringList services;
        if (!reply.isError())
          for (const auto &name : reply.value())
            if (name.startsWith("org.mpris.MediaPlayer2."))
              services << name;
        if (services.isEmpty()) {
          m_media = normalizeMedia({}, {});
          m_previousMedia.clear();
          m_mediaBusy = false;
          emit mediaChanged();
          return;
        }
        struct Batch {
          int left;
          QVariantList players;
        };
        auto batch = std::make_shared<Batch>();
        batch->left = services.size();
        auto complete = [this, batch](const QString &service,
                                      const QString &owner,
                                      const QDBusMessage &response) {
          if (response.type() != QDBusMessage::ErrorMessage) {
            auto player = normalizeMedia(service, decodeProperties(response));
            player["owner"] = owner;
            batch->players << player;
          }
          if (--batch->left)
            return;
          ++m_mediaGeneration;
          QHash<QString, QVariantMap> next;
          for (auto &candidate : batch->players) {
            auto player = candidate.toMap();
            const auto service = player.value("service").toString();
            const auto before = m_previousMedia.value(service);
            auto activity = before.value("activity").toULongLong();
            if (player.value("playing").toBool() &&
                (!before.value("playing").toBool() ||
                 before.value("title") != player.value("title") ||
                 before.value("artist") != player.value("artist")))
              activity = m_mediaGeneration;
            player.insert("activity", activity);
            candidate = player;
            next.insert(service, player);
          }
          const auto selected =
              currentMedia(batch->players, m_media.value("service").toString());
          if (selected.value("service") != m_media.value("service") ||
              selected.value("trackId") != m_media.value("trackId") ||
              selected.value("owner") != m_media.value("owner")) {
            ++m_seekGeneration;
            emit seekContextChanged();
          }
          m_media = selected;
          m_previousMedia = next;
          m_mediaBusy = false;
          emit mediaChanged();
          if (m_mediaDirty)
            m_mediaRefresh.start();
        };
        for (const auto &service : services) {
          auto ownerRequest = QDBusMessage::createMethodCall(
              "org.freedesktop.DBus", "/org/freedesktop/DBus",
              "org.freedesktop.DBus", "GetNameOwner");
          ownerRequest << service;
          auto *ownerCall = new QDBusPendingCallWatcher(
              QDBusConnection::sessionBus().asyncCall(ownerRequest, 1200),
              this);
          connect(ownerCall, &QDBusPendingCallWatcher::finished, this,
                  [this, service, complete](auto *call) {
                    QDBusPendingReply<QString> ownerReply = *call;
                    const auto reply = call->reply();
                    call->deleteLater();
                    if (ownerReply.isError()) {
                      complete(service, {},
                               QDBusMessage::createError(ownerReply.error()));
                      return;
                    }
                    const auto owner = ownerReply.value();
                    auto message = mediaCall(
                        owner, "org.freedesktop.DBus.Properties", "GetAll");
                    message << "org.mpris.MediaPlayer2.Player";
                    auto *pending = new QDBusPendingCallWatcher(
                        QDBusConnection::sessionBus().asyncCall(message, 1200),
                        this);
                    connect(pending, &QDBusPendingCallWatcher::finished, this,
                            [service, owner, complete](auto *done) {
                              const auto response = done->reply();
                              done->deleteLater();
                              complete(service, owner, response);
                            });
                  });
        }
      });
}

QVariantMap Backend::currentMedia(const QVariantList &players,
                                  const QString &previous) {
  QVariantMap selected;
  auto rank = [&previous](const QVariantMap &player) {
    const auto service = player.value("service").toString();
    return std::make_tuple(
        player.value("playing").toBool(),
        player.value("activity").toULongLong(),
        service.contains("plasma-browser-integration"), service == previous,
        !player.value("artist").toString().isEmpty(), service);
  };
  for (const auto &candidate : players) {
    const auto player = candidate.toMap();
    if (selected.isEmpty() || rank(player) > rank(selected))
      selected = player;
  }
  return selected.isEmpty() ? normalizeMedia({}, {}) : selected;
}

void Backend::mediaAction(const QString &method) {
  const auto service = m_media.value("service").toString();
  if (service.isEmpty())
    return;
  const bool allowed =
      (method == "Next" && m_media.value("canNext").toBool()) ||
      (method == "Previous" && m_media.value("canPrevious").toBool()) ||
      (method == "PlayPause" &&
       m_media.value(m_media.value("playing").toBool() ? "canPause" : "canPlay")
           .toBool());
  if (!allowed)
    return;
  auto *watcher = new QDBusPendingCallWatcher(
      QDBusConnection::sessionBus().asyncCall(
          mediaCall(service, "org.mpris.MediaPlayer2.Player", method), 1500),
      this);
  connect(watcher, &QDBusPendingCallWatcher::finished, this,
          [this](QDBusPendingCallWatcher *call) {
            if (call->isError())
              setMessage("The player could not complete this action.");
            call->deleteLater();
            pollMedia();
          });
}

void Backend::pollStats() {
  m_system.refresh((m_expanded && m_currentPage == 2) || m_systemPreview);
}
void Backend::setPreferredPage(int page) {
  if (page < 0 || page > 7 || page == 3)
    return;
  m_settings.setValue("ui/lastPage", page);
  emit preferencesChanged();
}
void Backend::setCurrentPage(int page) {
  m_currentPage = page;
  if (m_live && m_expanded)
    pollStats();
}
void Backend::previewSystem() {
  m_systemPreview = true;
  pollStats();
  QTimer::singleShot(250, this, [this] { pollStats(); });
}

void Backend::pollVolume() {
  if (m_audioBusy || m_commandBusy ||
      QStandardPaths::findExecutable("pactl").isEmpty())
    return;
  m_audioBusy = true;
  auto *process = new QProcess(this);
  connect(
      process, &QProcess::finished, this,
      [this, process](int code, QProcess::ExitStatus status) {
        const auto object =
            QJsonDocument::fromJson(process->readAllStandardOutput()).object();
        m_audioAvailable =
            code == 0 && status == QProcess::NormalExit && !object.isEmpty();
        if (m_audioAvailable) {
          const auto channels = object.value("volume").toObject();
          double total = 0;
          for (const auto &channel : channels)
            total +=
                channel.toObject().value("value").toDouble() / 65536.0 * 100;
          const auto value =
              channels.isEmpty() ? 0 : qRound(total / channels.size());
          if (m_haveAudio && audioFeedback() && value != m_volume)
            m_activity.show("volume", QString("Volume %1%").arg(value), 30);
          m_volume = value;
          m_haveAudio = true;
        }
        emit volumeChanged();
        process->deleteLater();
        if (!m_audioAvailable) {
          m_audioBusy = false;
          return;
        }
        auto *mute = new QProcess(this);
        connect(mute, &QProcess::finished, this,
                [this, mute](int code, QProcess::ExitStatus status) {
                  if (code == 0 && status == QProcess::NormalExit) {
                    const auto value =
                        QJsonDocument::fromJson(mute->readAllStandardOutput())
                            .object()
                            .value("mute")
                            .toBool();
                    if (audioFeedback() && value != m_muted)
                      m_activity.show("volume",
                                      value ? "Output muted" : "Output unmuted",
                                      30);
                    m_muted = value;
                  }
                  m_audioBusy = false;
                  emit volumeChanged();
                  mute->deleteLater();
                });
        connect(mute, &QProcess::errorOccurred, this,
                [this, mute](QProcess::ProcessError error) {
                  if (error == QProcess::FailedToStart) {
                    m_audioBusy = false;
                    mute->deleteLater();
                  }
                });
        mute->start("pactl",
                    {"--format=json", "get-sink-mute", "@DEFAULT_SINK@"});
        QTimer::singleShot(3000, mute, [mute] {
          if (mute->state() != QProcess::NotRunning)
            mute->kill();
        });
      });
  connect(process, &QProcess::errorOccurred, this,
          [this, process](QProcess::ProcessError error) {
            if (error == QProcess::FailedToStart) {
              m_audioBusy = false;
              m_audioAvailable = false;
              emit volumeChanged();
              process->deleteLater();
            }
          });
  process->start("pactl",
                 {"--format=json", "get-sink-volume", "@DEFAULT_SINK@"});
  QTimer::singleShot(3000, process, [process] {
    if (process->state() != QProcess::NotRunning)
      process->kill();
  });
}

void Backend::audioCommand(const QStringList &args) {
  if (m_commandBusy) {
    if (!m_audioQueue.isEmpty() && args.first() == "set-sink-volume" &&
        m_audioQueue.last().first() == "set-sink-volume")
      m_audioQueue.last() = args;
    else
      m_audioQueue.enqueue(args);
    return;
  }
  m_commandBusy = true;
  auto *process = new QProcess(this);
  connect(process, &QProcess::finished, this,
          [this, process](int code, QProcess::ExitStatus status) {
            m_commandBusy = false;
            if (code || status != QProcess::NormalExit)
              setMessage(
                  "Audio control failed. Is PipeWire / PulseAudio running?");
            process->deleteLater();
            if (!m_audioQueue.isEmpty())
              audioCommand(m_audioQueue.dequeue());
            else {
              pollVolume();
              if (audioFeedback())
                pollMic();
            }
          });
  connect(process, &QProcess::errorOccurred, this,
          [this, process](QProcess::ProcessError error) {
            if (error == QProcess::FailedToStart) {
              m_commandBusy = false;
              m_audioQueue.clear();
              setMessage("Install pactl (libpulse) to control volume.");
              process->deleteLater();
            }
          });
  process->start("pactl", args);
  QTimer::singleShot(3000, process, [process] {
    if (process->state() != QProcess::NotRunning)
      process->kill();
  });
}

void Backend::setVolume(int volume) {
  volume = qBound(0, volume, 100);
  audioCommand(
      {"set-sink-volume", "@DEFAULT_SINK@", QString::number(volume) + "%"});
}
void Backend::toggleMute() {
  audioCommand({"set-sink-mute", "@DEFAULT_SINK@", "toggle"});
}

int Backend::remaining() const { return m_focus.remaining(); }
void Backend::startTimer(int minutes) {
  m_focus.start(minutes);
  if (m_focus.running())
    m_activity.select("focus");
}
void Backend::pauseTimer() { m_focus.pause(); }
void Backend::cancelTimer() { m_focus.cancel(); }
void Backend::setMessage(const QString &message) {
  m_message = message;
  m_messageExpiry.start();
  emit messageChanged();
}

void Backend::updateInputRegion(double x, double width, double height) {
  if (!m_window)
    return;
  QPainterPath p;
  const double s = 14, r = qMin(26.0, height / 2);
  p.moveTo(x, 0);
  p.quadTo(x + s, 0, x + s, s);
  p.lineTo(x + s, height - r);
  p.quadTo(x + s, height, x + s + r, height);
  p.lineTo(x + width - s - r, height);
  p.quadTo(x + width - s, height, x + width - s, height - r);
  p.lineTo(x + width - s, s);
  p.quadTo(x + width - s, 0, x + width, 0);
  p.closeSubpath();
  m_window->setMask(QRegion(p.toFillPolygon().toPolygon()));
}

void Backend::attach(QQuickWindow *window, QSystemTrayIcon *tray) {
  m_window = window;
  m_tray = tray;
  position();
  buildMenu();
  connect(qApp, &QGuiApplication::screenAdded, this, [this] {
    position();
    buildMenu();
  });
  connect(qApp, &QGuiApplication::screenRemoved, this, [this] {
    QTimer::singleShot(100, this, [this] {
      position();
      buildMenu();
    });
  });
}

void Backend::position() {
  if (!m_window)
    return;
  QScreen *screen = QGuiApplication::primaryScreen();
  const auto wanted = m_settings.value("screen").toString();
  for (auto *candidate : QGuiApplication::screens())
    if (candidate->name() == wanted)
      screen = candidate;
  if (!screen)
    return;
  const int gap = m_settings.value("topGap", 0).toInt();
  // wl_layer_surface's output is fixed at creation. Recreate the native
  // surface when changing monitors; setScreen alone only updates Qt state.
  const bool remap = m_window->isVisible() && m_window->screen() != screen &&
                     QGuiApplication::platformName().startsWith("wayland");
  if (remap) {
    m_window->hide();
    m_window->destroy();
  }
  m_window->setScreen(screen);
  if (QGuiApplication::platformName().startsWith("wayland")) {
    auto *layer = LayerShellQt::Window::get(m_window);
    layer->setScope("kestrel-island");
    // KWin maps Top to AboveLayer, below active fullscreen windows.
    // Overlay would cover games and fullscreen video regardless of focus.
    layer->setLayer(LayerShellQt::Window::LayerTop);
    layer->setAnchors(LayerShellQt::Window::AnchorTop);
    layer->setExclusiveZone(-1);
    layer->setKeyboardInteractivity(
        m_expanded ? LayerShellQt::Window::KeyboardInteractivityOnDemand
                   : LayerShellQt::Window::KeyboardInteractivityNone);
    layer->setActivateOnShow(false);
    layer->setScreen(screen);
    layer->setMargins(QMargins(0, gap, 0, 0));
  } else {
    m_window->setPosition(screen->geometry().center().x() -
                              m_window->width() / 2,
                          screen->geometry().top() + gap);
  }
  if (remap)
    m_window->show();
  emit preferencesChanged();
}

void Backend::buildMenu() {
  if (!m_tray)
    return;
  auto *old = m_tray->contextMenu();
  auto *menu = new QMenu;
  menu->addSection("Kestrel Island");
  connect(menu->addAction("Settings…"), &QAction::triggered, this,
          &Backend::settingsRequested);
  connect(menu->addAction("Expand / collapse"), &QAction::triggered, this,
          &Backend::toggleRequested);
  auto *screens = menu->addMenu("Display");
  for (auto *screen : QGuiApplication::screens()) {
    auto *action = screens->addAction(screen->name());
    action->setCheckable(true);
    action->setChecked(screen == m_window->screen());
    connect(action, &QAction::triggered, this,
            [this, name = screen->name()] { chooseDisplay(name); });
  }
  auto *gaps = menu->addMenu("Top offset (avoid your panel)");
  for (int gap : {0, 24, 32, 40, 48, 56, 64}) {
    auto *action = gaps->addAction(gap == 0 ? "0 px — attached notch"
                                            : QString::number(gap) + " px");
    action->setCheckable(true);
    action->setChecked(m_settings.value("topGap", 0).toInt() == gap);
    connect(action, &QAction::triggered, this, [this, gap] { setTopGap(gap); });
  }
  for (const auto &entry :
       {qMakePair(QString("Peek on hover"), QString("hoverExpand")),
        qMakePair(QString("Animate transitions"), QString("animations"))}) {
    auto *action = menu->addAction(entry.first);
    action->setCheckable(true);
    action->setChecked(m_settings.value(entry.second, true).toBool());
    connect(action, &QAction::toggled, this,
            [this, key = entry.second](bool on) {
              m_settings.setValue(key, on);
              emit preferencesChanged();
            });
  }
  const auto startup =
      QStandardPaths::writableLocation(QStandardPaths::ConfigLocation) +
      "/autostart/io.github.kestrel.Island.desktop";
  auto *autostart = menu->addAction("Start at login");
  autostart->setCheckable(true);
  autostart->setChecked(QFile::exists(startup));
  connect(autostart, &QAction::toggled, this, [this, startup](bool on) {
    if (!on) {
      if (!QFile::remove(startup))
        setMessage(
            "Could not disable startup. Check the autostart file permissions.");
      return;
    }
    QDir().mkpath(QFileInfo(startup).absolutePath());
    QSaveFile file(startup);
    auto executable = QCoreApplication::applicationFilePath();
    executable.replace('\\', "\\\\")
        .replace('"', "\\\"")
        .replace('`', "\\`")
        .replace('$', "\\$")
        .replace('%', "%%");
    const auto data =
        QString("[Desktop Entry]\nType=Application\nName=Kestrel "
                "Island\nExec=\"%1\"\nIcon=kestrel-island\nTerminal=false\n")
            .arg(executable)
            .toUtf8();
    if (!file.open(QIODevice::WriteOnly) || file.write(data) != data.size() ||
        !file.commit())
      setMessage(
          "Could not enable startup. Check your config directory permissions.");
  });
  menu->addSeparator();
  connect(menu->addAction("Quit Kestrel Island"), &QAction::triggered, qApp,
          &QCoreApplication::quit);
  m_tray->setContextMenu(menu);
  m_menu = menu;
  if (old)
    old->deleteLater();
}

void Backend::openMenu() {
  if (m_tray && m_tray->contextMenu())
    m_tray->contextMenu()->popup(QCursor::pos());
}

void Backend::setExpanded(bool expanded) {
  m_expanded = expanded;
  if (expanded && m_live)
    pollStats();
  if (m_window && QGuiApplication::platformName().startsWith("wayland")) {
    const bool releaseFocus = !expanded && m_window->isActive();
    LayerShellQt::Window::get(m_window)->setKeyboardInteractivity(
        expanded ? LayerShellQt::Window::KeyboardInteractivityOnDemand
                 : LayerShellQt::Window::KeyboardInteractivityNone);
    // KWin 6.8 leaves an active layer surface focused when interactivity
    // becomes None. Unmapping releases it through KWin's normal focus chain;
    // remapping with None and activateOnShow=false cannot reclaim focus.
    if (releaseFocus) {
      m_window->hide();
      m_window->show();
    }
  }
}

QStringList Backend::displays() const {
  QStringList names;
  for (auto *screen : QGuiApplication::screens())
    names << screen->name();
  return names;
}
QString Backend::display() const {
  const auto saved = m_settings.value("screen").toString();
  if (displays().contains(saved))
    return saved;
  return QGuiApplication::primaryScreen()
             ? QGuiApplication::primaryScreen()->name()
             : QString();
}
void Backend::chooseDisplay(const QString &name) {
  if (!displays().contains(name))
    return;
  m_settings.setValue("screen", name);
  position();
  buildMenu();
  emit preferencesChanged();
}
void Backend::setTopGap(int gap) {
  m_settings.setValue("topGap", qBound(0, gap, 120));
  position();
  buildMenu();
  emit preferencesChanged();
}
void Backend::setHoverExpand(bool enabled) {
  m_settings.setValue("hoverExpand", enabled);
  buildMenu();
  emit preferencesChanged();
}
void Backend::finishMove(const QString &name) {
  if (m_moving && displays().contains(name))
    m_dropDisplay = name;
}
void Backend::beginMove() {
  if (!m_window || m_moving)
    return;
  auto *engine = qmlEngine(m_window);
  if (!engine)
    return;
  QQmlComponent component(engine, QUrl("qrc:/qml/MoveTarget.qml"));
  if (component.isError()) {
    setMessage("Could not open display targets.");
    return;
  }
  m_moving = true;
  m_dropDisplay.clear();
  QList<QQuickWindow *> targets;
  for (auto *screen : QGuiApplication::screens()) {
    auto *target =
        qobject_cast<QQuickWindow *>(component.createWithInitialProperties(
            {{"displayName", screen->name()}}));
    if (!target)
      continue;
    target->setScreen(screen);
    target->resize(screen->geometry().size());
    if (QGuiApplication::platformName().startsWith("wayland")) {
      auto *layer = LayerShellQt::Window::get(target);
      layer->setScope("kestrel-island-move");
      layer->setScreen(screen);
      layer->setAnchors(
          LayerShellQt::Window::Anchors(LayerShellQt::Window::AnchorTop) |
          LayerShellQt::Window::AnchorBottom |
          LayerShellQt::Window::AnchorLeft | LayerShellQt::Window::AnchorRight);
      layer->setLayer(LayerShellQt::Window::LayerOverlay);
      layer->setExclusiveZone(-1);
      layer->setKeyboardInteractivity(
          LayerShellQt::Window::KeyboardInteractivityNone);
      layer->setActivateOnShow(false);
    } else
      target->setPosition(screen->geometry().topLeft());
    target->show();
    targets << target;
  }
  if (!targets.isEmpty()) {
    QDrag drag(m_window);
    auto *mime = new QMimeData;
    mime->setData("application/x-kestrel-island", "move");
    drag.setMimeData(mime);
    drag.setPixmap(QIcon(":/assets/island.svg").pixmap(64, 64));
    drag.setHotSpot(QPoint(32, 32));
    drag.exec(Qt::MoveAction);
  }
  for (auto *target : targets) {
    target->hide();
    target->deleteLater();
  }
  m_moving = false;
  if (!m_dropDisplay.isEmpty())
    chooseDisplay(m_dropDisplay);
}

void Backend::mediaProperties(const QString &interface, const QVariantMap &,
                              const QStringList &) {
  if (interface == "org.mpris.MediaPlayer2.Player")
    m_mediaRefresh.start();
}
void Backend::mediaSeeked(qlonglong) { m_mediaRefresh.start(); }
void Backend::mediaOwnerChanged(const QString &name, const QString &,
                                const QString &) {
  if (name == m_media.value("service").toString()) {
    ++m_seekGeneration;
    m_media["canSeek"] = false;
    emit seekContextChanged();
    emit mediaChanged();
  }
  if (name.startsWith("org.mpris.MediaPlayer2."))
    m_mediaRefresh.start();
}
double Backend::playbackPosition() const {
  double value = m_media.value("positionUs").toLongLong() / 1000000.0;
  const double rate = m_media.value("rate", 1.0).toDouble();
  if (m_media.value("playing").toBool() && m_mediaSample.isValid() &&
      std::isfinite(rate) && rate > 0)
    value += m_mediaSample.elapsed() / 1000.0 * rate;
  return qBound(
      0.0, value,
      qMax(0.0, m_media.value("durationUs").toLongLong() / 1000000.0));
}
void Backend::seek(double seconds, const QString &service,
                   const QString &trackId, qulonglong generation) {
  if (generation != m_seekGeneration ||
      m_media.value("owner").toString().isEmpty() || !std::isfinite(seconds) ||
      !m_media.value("canSeek").toBool() || trackId.isEmpty() ||
      !trackId.startsWith('/') ||
      service != m_media.value("service").toString() ||
      trackId != m_media.value("trackId").toString())
    return;
  auto msg = mediaCall(m_media.value("owner").toString(),
                       "org.mpris.MediaPlayer2.Player", "SetPosition");
  msg << QVariant::fromValue(QDBusObjectPath(trackId))
      << qint64(qBound(0.0, seconds,
                       m_media.value("durationUs").toLongLong() / 1000000.0) *
                1000000);
  auto *call = new QDBusPendingCallWatcher(
      QDBusConnection::sessionBus().asyncCall(msg, 1500), this);
  connect(call, &QDBusPendingCallWatcher::finished, this, [this](auto *done) {
    if (done->isError())
      setMessage(
          "The player could not seek. Returning to its current position.");
    done->deleteLater();
    pollMedia();
  });
}
void Backend::setAudioFeedback(bool value) {
  m_settings.setValue("audioFeedback", value);
  if (value && m_live) {
    startAudioObserver();
    pollMic();
  } else {
    m_audioRestart.stop();
    if (m_audioObserver)
      m_audioObserver->kill();
  }
  emit preferencesChanged();
}
void Backend::setTasksEnabled(bool value) {
  m_settings.setValue("tasksEnabled", value);
  m_tasks.setEnabled(value);
}
void Backend::configureNotifications(bool enabled, const QString &apps) {
  const auto list = apps.split(',', Qt::SkipEmptyParts);
  m_settings.setValue("notifications/enabled", enabled);
  m_settings.setValue("notifications/apps", list);
  m_notifications.configure(enabled, list);
}
void Backend::toggleMicMute() {
  if (!audioFeedback() || !m_micAvailable)
    return;
  audioCommand({"set-source-mute", "@DEFAULT_SOURCE@", m_micMuted ? "0" : "1"});
}
void Backend::pollMic() {
  if (m_micBusy || !audioFeedback() || m_commandBusy)
    return;
  m_micBusy = true;
  auto *process = new QProcess(this);
  connect(
      process, &QProcess::finished, this,
      [this, process](int code, QProcess::ExitStatus status) {
        auto object =
            QJsonDocument::fromJson(process->readAllStandardOutput()).object();
        m_micAvailable = code == 0 && status == QProcess::NormalExit &&
                         object.contains("mute");
        if (m_micAvailable) {
          auto value = object["mute"].toBool();
          if (value != m_micMuted)
            m_activity.show("microphone",
                            value ? "Default microphone muted"
                                  : "Default microphone unmuted",
                            30);
          m_micMuted = value;
        }
        m_micBusy = false;
        emit volumeChanged();
        process->deleteLater();
      });
  connect(process, &QProcess::errorOccurred, this, [this, process](auto error) {
    if (error == QProcess::FailedToStart) {
      m_micBusy = false;
      m_micAvailable = false;
      emit volumeChanged();
      process->deleteLater();
    }
  });
  process->start("pactl",
                 {"--format=json", "get-source-mute", "@DEFAULT_SOURCE@"});
  QTimer::singleShot(3000, process, [process] {
    if (process->state() != QProcess::NotRunning)
      process->kill();
  });
}
void Backend::startAudioObserver() {
  if (!m_live || !audioFeedback() || m_audioObserver ||
      QStandardPaths::findExecutable("pactl").isEmpty())
    return;
  auto *process = new QProcess(this);
  m_audioObserver = process;
  auto environment = QProcessEnvironment::systemEnvironment();
  environment.insert("LC_ALL", "C");
  process->setProcessEnvironment(environment);
  connect(process, &QProcess::readyReadStandardOutput, this, [this, process] {
    auto data = process->readAllStandardOutput();
    m_audioBackoff = 1000;
    if (data.size() > 65536) {
      process->kill();
      return;
    }
    if (data.contains("sink") || data.contains("source") ||
        data.contains("server"))
      m_audioRefresh.start();
  });
  connect(process, &QProcess::readyReadStandardError, process,
          [process] { process->readAllStandardError(); });

  auto cleanup = [this, process] {
    if (m_audioObserver != process)
      return;
    m_audioObserver = nullptr;
    process->deleteLater();
    if (audioFeedback()) {
      m_audioRestart.start(m_audioBackoff);
      m_audioBackoff = qMin(15000, m_audioBackoff * 2);
    }
  };
  connect(process, &QProcess::finished, this,
          [cleanup](int, QProcess::ExitStatus) { cleanup(); });
  connect(process, &QProcess::errorOccurred, this, [cleanup](auto error) {
    if (error == QProcess::FailedToStart)
      cleanup();
  });
  process->start("pactl", {"subscribe"});
}

void Backend::copyTaskExample() {
  QGuiApplication::clipboard()->setText(QStringLiteral(
      "kestrel-task --label \"Build\" -- cmake --build build -j 4"));
  setMessage("Copied task command. Run it in your project directory.");
}
