#include "keepawakeservice.h"
#include "sessionclock.h"
#include <QDBusConnectionInterface>
#include <QDBusMessage>
#include <QDBusPendingCallWatcher>
#include <QDBusPendingReply>
#include <QDBusReply>
#include <QDBusServiceWatcher>

static QDBusMessage inhibition(int which, const QString &method,
                               const QString &owner = {}) {
  return QDBusMessage::createMethodCall(
      !owner.isEmpty() ? owner
      : which == 0     ? "org.freedesktop.ScreenSaver"
                       : "org.freedesktop.PowerManagement",
      which == 0 ? "/ScreenSaver" : "/org/freedesktop/PowerManagement/Inhibit",
      which == 0 ? "org.freedesktop.ScreenSaver"
                 : "org.freedesktop.PowerManagement.Inhibit",
      method);
}
KeepAwakeService::KeepAwakeService(QObject *parent, QDBusConnection bus,
                                   std::function<qint64()> clock)
    : QObject(parent), m_bus(bus), m_now(std::move(clock)) {
  if (!m_now)
    m_now = sessionMilliseconds;
  m_tick.setInterval(1000);
  connect(&m_tick, &QTimer::timeout, this, [this] {
    if (m_minutes && m_now() - m_startedAt >= m_minutes * 60000)
      stop();
    else
      emit changed();
  });
  auto *watch = new QDBusServiceWatcher(this);
  watch->setConnection(bus);
  watch->setWatchMode(QDBusServiceWatcher::WatchForOwnerChange);
  watch->addWatchedService("org.freedesktop.ScreenSaver");
  watch->addWatchedService("org.freedesktop.PowerManagement");
  connect(watch, &QDBusServiceWatcher::serviceOwnerChanged, this,
          [this](auto, auto oldOwner, auto) {
            if (!oldOwner.isEmpty() && m_state != "off") {
              stop();
              m_state = "error";
              m_error = "Power service changed. Start again to confirm.";
              emit changed();
            }
          });
}
KeepAwakeService::~KeepAwakeService() {
  for (auto lease : m_cookies) {
    auto msg = inhibition(lease.which, "UnInhibit", lease.owner);
    msg << lease.cookie;
    m_bus.call(msg, QDBus::Block, 300);
  }
}
QVariantMap KeepAwakeService::snapshot() const {
  return {{"state", m_state},
          {"error", m_error},
          {"remaining",
           m_minutes
               ? qMax(0, m_minutes * 60 - int((m_now() - m_startedAt) / 1000))
               : 0},
          {"timed", m_minutes > 0}};
}
void KeepAwakeService::start(int minutes) {
  if (minutes < 0 || minutes > 180 || m_state == "requesting" || m_releases ||
      !m_cookies.isEmpty())
    return;
  m_error.clear();
  m_minutes = minutes;
  m_state = "requesting";
  emit changed();
  acquire(0, ++m_generation);
}
void KeepAwakeService::acquire(int which, quint64 generation) {
  auto msg = inhibition(which, "Inhibit");
  msg << "Kestrel Island" << "Keep awake requested by the user";
  m_bus.interface()->setTimeout(1200);
  const QDBusReply<QString> ownerReply =
      m_bus.interface()->serviceOwner(msg.service());
  if (!ownerReply.isValid()) {
    m_error = "Power service unavailable";
    stop();
    m_state = "error";
    emit changed();
    return;
  }
  const auto owner = ownerReply.value();
  msg = inhibition(which, "Inhibit", owner);
  msg << "Kestrel Island" << "Keep awake requested by the user";
  auto *watch = new QDBusPendingCallWatcher(m_bus.asyncCall(msg, 1200), this);
  connect(watch, &QDBusPendingCallWatcher::finished, this,
          [this, which, generation, owner](auto *call) {
            QDBusPendingReply<uint> reply = *call;
            call->deleteLater();
            if (generation != m_generation) {
              if (!reply.isError())
                release(which, reply.value(), owner);
              return;
            }
            if (reply.isError()) {
              m_error = reply.error().message();
              stop();
              m_state = "error";
              emit changed();
              return;
            }
            m_cookies.append({which, reply.value(), owner});
            if (which == 0)
              acquire(1, generation);
            else {
              m_state = "active";
              m_startedAt = m_now();
              m_tick.start();
              emit changed();
            }
          });
}
void KeepAwakeService::release(int which, uint cookie, const QString &owner,
                               int attempt) {
  if (attempt == 0)
    ++m_releases;
  auto msg = inhibition(which, "UnInhibit", owner);
  msg << cookie;
  auto *call = new QDBusPendingCallWatcher(m_bus.asyncCall(msg, 1200), this);
  connect(call, &QDBusPendingCallWatcher::finished, this,
          [this, which, cookie, owner, attempt](auto *done) {
            auto error = done->error();
            done->deleteLater();
            if (error.isValid() && error.type() != QDBusError::ServiceUnknown &&
                error.name() != "org.freedesktop.DBus.Error.NameHasNoOwner") {
              if (attempt < 2) {
                QTimer::singleShot(200, this, [=, this] {
                  release(which, cookie, owner, attempt + 1);
                });
                return;
              }
              m_state = "error";
              m_error = "Could not release keep awake. Quit Kestrel to release "
                        "the connection.";
              m_cookies.append({which, cookie, owner});
            }
            --m_releases;
            if (!m_releases && m_state == "releasing")
              m_state = "off";
            emit changed();
          });
}
void KeepAwakeService::stop() {
  ++m_generation;
  m_tick.stop();
  auto cookies = m_cookies;
  m_cookies.clear();
  for (auto lease : cookies)
    release(lease.which, lease.cookie, lease.owner);
  m_state = m_releases ? "releasing" : "off";
  emit changed();
}
