#include "sessionstate.h"
#include <QDBusMessage>
#include <QDBusPendingCallWatcher>
#include <QDBusPendingReply>
#include <QDBusServiceWatcher>
#include <QDBusVariant>

SessionState::SessionState(QObject *parent, bool live, QDBusConnection bus)
    : QObject(parent), m_bus(bus) {
  if (live)
    start();
}
void SessionState::start() {
  if (m_started)
    return;
  m_started = true;
  auto bus = m_bus;
  bus.connect("org.freedesktop.ScreenSaver", "/ScreenSaver",
              "org.freedesktop.ScreenSaver", "ActiveChanged", this,
              SLOT(locked(bool)));
  bus.connect("org.freedesktop.ScreenSaver", "/ScreenSaver",
              "org.kde.screensaver", "AboutToLock", this, SLOT(aboutToLock()));
  bus.connect("org.freedesktop.Notifications", "/org/freedesktop/Notifications",
              "org.freedesktop.DBus.Properties", "PropertiesChanged", this,
              SLOT(properties(QString, QVariantMap, QStringList)));
  auto *watch = new QDBusServiceWatcher(this);
  watch->setConnection(bus);
  watch->setWatchMode(QDBusServiceWatcher::WatchForOwnerChange);
  watch->addWatchedService("org.freedesktop.ScreenSaver");
  watch->addWatchedService("org.freedesktop.Notifications");
  connect(watch, &QDBusServiceWatcher::serviceOwnerChanged, this,
          [this] { resync(); });
  QTimer::singleShot(0, this, &SessionState::resync);
}
void SessionState::resync() {
  const auto generation = ++m_generation;
  m_lockedKnown = m_quietKnown = false;
  emit changed();
  auto lock = QDBusMessage::createMethodCall(
      "org.freedesktop.ScreenSaver", "/ScreenSaver",
      "org.freedesktop.ScreenSaver", "GetActive");
  auto *pending =
      new QDBusPendingCallWatcher(m_bus.asyncCall(lock, 1200), this);
  connect(pending, &QDBusPendingCallWatcher::finished, this,
          [this, generation](auto *call) {
            QDBusPendingReply<bool> reply = *call;
            call->deleteLater();
            if (generation != m_generation)
              return;
            m_lockedKnown = !reply.isError();
            m_locked = reply.isError() || reply.value();
            emit changed();
          });
  auto quiet = QDBusMessage::createMethodCall(
      "org.freedesktop.Notifications", "/org/freedesktop/Notifications",
      "org.freedesktop.DBus.Properties", "Get");
  quiet << "org.freedesktop.Notifications" << "Inhibited";
  auto *request =
      new QDBusPendingCallWatcher(m_bus.asyncCall(quiet, 1200), this);
  connect(request, &QDBusPendingCallWatcher::finished, this,
          [this, generation](auto *call) {
            QDBusPendingReply<QDBusVariant> reply = *call;
            call->deleteLater();
            if (generation != m_generation)
              return;
            m_quietKnown = !reply.isError();
            m_quiet = reply.isError() || reply.value().variant().toBool();
            emit changed();
          });
}
void SessionState::locked(bool value) {
  ++m_generation;
  m_lockedKnown = true;
  m_locked = value;
  if (!value)
    resync();
  else
    emit this->changed();
}
void SessionState::aboutToLock() { locked(true); }
void SessionState::properties(const QString &interface,
                              const QVariantMap &changed,
                              const QStringList &invalidated) {
  if (interface != "org.freedesktop.Notifications")
    return;
  if (invalidated.contains("Inhibited")) {
    resync();
    return;
  }
  if (changed.contains("Inhibited")) {
    ++m_generation;
    m_quietKnown = true;
    m_quiet = changed["Inhibited"].toBool();
    if (!m_quiet)
      resync();
    else
      emit this->changed();
  }
}
void SessionState::setTestState(bool locked, bool quiet) {
  ++m_generation;
  m_lockedKnown = m_quietKnown = true;
  m_locked = locked;
  m_quiet = quiet;
  emit changed();
}
