#include "notificationsservice.h"
#include <QDBusMessage>
#include <QDBusPendingCallWatcher>
#include <QDBusPendingReply>
#include <QDBusServiceWatcher>
#include <QDateTime>

static QDBusMessage notificationCall(const QString &owner,
                                     const QString &method) {
  return QDBusMessage::createMethodCall(owner, "/org/freedesktop/Notifications",
                                        "org.kde.NotificationManager", method);
}
NotificationsService::NotificationsService(QObject *parent, bool live,
                                           bool validated, QDBusConnection bus)
    : QObject(parent), m_bus(bus), m_session(this, false, bus), m_live(live),
      m_validated(validated) {
  connect(&m_session, &SessionState::changed, this,
          &NotificationsService::reconcile);
  m_expiry.setInterval(30000);
  connect(&m_expiry, &QTimer::timeout, this, [this] {
    auto now = QDateTime::currentMSecsSinceEpoch();
    m_entries.removeIf([now](auto v) {
      return now - v.toMap()["received"].toLongLong() >= 600000;
    });
    emit changed();
  });
  if (!live)
    return;
  bus.registerObject("/NotificationWatcher", this,
                     QDBusConnection::ExportScriptableSlots);
  bus.connect("org.freedesktop.Notifications", "/org/freedesktop/Notifications",
              "org.freedesktop.Notifications", "NotificationClosed", this,
              SLOT(closed(uint, uint)));
  auto *watch =
      new QDBusServiceWatcher("org.freedesktop.Notifications", bus,
                              QDBusServiceWatcher::WatchForOwnerChange, this);
  connect(watch, &QDBusServiceWatcher::serviceOwnerChanged, this,
          [this](auto, auto, auto owner) {
            unregister();
            ++m_ownerGeneration;
            m_owner = owner;
            m_registered = m_registering = false;
            clear();
            if (m_enabled && m_validated && !m_allowlist.isEmpty())
              m_session.resync();
          });
  auto msg = QDBusMessage::createMethodCall(
      "org.freedesktop.DBus", "/org/freedesktop/DBus", "org.freedesktop.DBus",
      "GetNameOwner");
  msg << "org.freedesktop.Notifications";
  const auto generation = m_ownerGeneration;
  auto *call = new QDBusPendingCallWatcher(bus.asyncCall(msg, 1200), this);
  connect(call, &QDBusPendingCallWatcher::finished, this,
          [this, generation](auto *done) {
            QDBusPendingReply<QString> reply = *done;
            done->deleteLater();
            if (generation != m_ownerGeneration)
              return;
            if (!reply.isError())
              m_owner = reply.value();
            reconcile();
          });
}
NotificationsService::~NotificationsService() {
  unregister();
  if (m_live)
    m_bus.unregisterObject("/NotificationWatcher");
}
QString NotificationsService::status() const {
  if (!m_enabled)
    return "Off — notifications stay with Plasma";
  if (!m_validated)
    return "Unavailable until KDE lock / Do Not Disturb validation";
  if (m_allowlist.isEmpty())
    return "Choose application IDs before receiving notifications";
  if (!m_session.safe())
    return "Paused — locked, quiet mode, or session state unknown";
  if (!m_error.isEmpty())
    return m_error;
  return m_registered ? "Watching allowed applications"
                      : "Connecting to Plasma";
}
void NotificationsService::configure(bool enabled,
                                     const QStringList &allowlist) {
  unregister();
  m_enabled = enabled;
  m_allowlist.clear();
  for (auto app : allowlist) {
    app = app.trimmed();
    if (!app.isEmpty() && app.size() <= 160 && !m_allowlist.contains(app) &&
        m_allowlist.size() < 32)
      m_allowlist << app;
  }
  m_error.clear();
  clear();
  if (m_live && m_enabled && m_validated && !m_allowlist.isEmpty())
    m_session.start();
  reconcile();
}
void NotificationsService::clear() {
  m_entries.clear();
  m_expiry.stop();
  emit changed();
}
void NotificationsService::unregister() {
  ++m_generation;
  if ((m_registered || m_registering) && !m_owner.isEmpty())
    m_bus.asyncCall(notificationCall(m_owner, "UnRegisterWatcher"), 1200);
  m_registered = m_registering = false;
}
void NotificationsService::reconcile() {
  if (!m_live || !m_enabled || !m_validated || m_allowlist.isEmpty() ||
      !m_session.safe() || m_owner.isEmpty()) {
    unregister();
    clear();
    return;
  }
  if (m_registered || m_registering) {
    emit changed();
    return;
  }
  m_registering = true;
  const auto generation = ++m_generation;
  const auto owner = m_owner;
  auto *call = new QDBusPendingCallWatcher(
      m_bus.asyncCall(notificationCall(owner, "RegisterWatcher"), 1200), this);
  connect(call, &QDBusPendingCallWatcher::finished, this,
          [this, generation, owner](auto *done) {
            const bool ok = !done->isError();
            done->deleteLater();
            if (generation != m_generation) {
              // Cancellation already queued UnRegisterWatcher before any
              // replacement registration on this connection. Do not remove
              // the replacement after its acknowledgement arrives.
              return;
            }
            m_registering = false;
            m_registered = ok;
            if (!ok)
              m_error = "Plasma notification watcher is unavailable";
            emit changed();
          });
}
bool NotificationsService::ingest(const QString &sender, uint id,
                                  const QString &app, uint replaces,
                                  const QString &summary, const QString &body,
                                  const QVariantMap &hints) {
  if (!m_registered || !m_enabled || !m_validated || !m_session.safe() ||
      sender != m_owner)
    return false;
  const auto desktop = hints.value("desktop-entry").toString();
  if (!m_allowlist.contains(app) && !m_allowlist.contains(desktop))
    return false;
  if ((app.size() + summary.size() + body.size()) * 2 > 65536)
    return false;
  m_entries.removeIf([=](auto v) {
    auto existing = v.toMap()["id"].toUInt();
    return existing == id || (replaces && existing == replaces);
  });
  m_entries.prepend(
      QVariantMap{{"id", id},
                  {"app", app.left(160)},
                  {"summary", summary.left(160)},
                  {"body", body.left(512)},
                  {"received", QDateTime::currentMSecsSinceEpoch()}});
  while (m_entries.size() > 20)
    m_entries.removeLast();
  m_expiry.start();
  emit changed();
  return true;
}
void NotificationsService::Notify(uint id, const QString &app, uint replaces,
                                  const QString &, const QString &summary,
                                  const QString &body, const QStringList &,
                                  const QVariantMap &hints, int) {
  if (calledFromDBus())
    ingest(message().service(), id, app, replaces, summary, body, hints);
}
void NotificationsService::CloseNotification(uint id) {
  if (calledFromDBus() && message().service() == m_owner)
    closed(id, 0);
}
void NotificationsService::closed(uint id, uint) {
  m_entries.removeIf([id](auto v) { return v.toMap()["id"].toUInt() == id; });
  emit changed();
}
