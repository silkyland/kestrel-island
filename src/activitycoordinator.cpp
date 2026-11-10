#include "activitycoordinator.h"
#include <algorithm>
#include <chrono>

ActivityCoordinator::ActivityCoordinator(QObject *parent,
                                         std::function<qint64()> clock)
    : QObject(parent), m_now(std::move(clock)) {
  if (!m_now)
    m_now = [] {
      return std::chrono::duration_cast<std::chrono::milliseconds>(
                 std::chrono::steady_clock::now().time_since_epoch())
          .count();
    };
  m_expiry.setInterval(100);
  connect(&m_expiry, &QTimer::timeout, this, &ActivityCoordinator::expire);
}
QVariantMap ActivityCoordinator::selected() const {
  for (const auto &value : m_activities)
    if (value.toMap().value("key") == m_selection)
      return value.toMap();
  return m_activities.isEmpty() ? QVariantMap() : m_activities.first().toMap();
}
QVariantMap ActivityCoordinator::notice() const {
  return m_notices.isEmpty() ? QVariantMap() : m_notices.first().toMap();
}
void ActivityCoordinator::upsert(const QVariantMap &value, bool select) {
  if (value.value("key").toString().isEmpty())
    return;
  for (auto &entry : m_activities)
    if (entry.toMap().value("key") == value.value("key")) {
      if (select)
        m_selection = value.value("key").toString();
      if (entry.toMap() != value || select) {
        entry = value;
        emit changed();
      }
      return;
    }
  if (m_activities.size() >= 8)
    return;
  m_activities << value;
  if (select || m_selection.isEmpty())
    m_selection = value.value("key").toString();
  emit changed();
}
void ActivityCoordinator::remove(const QString &key) {
  const auto size = m_activities.size();
  m_activities.removeIf(
      [&](const QVariant &value) { return value.toMap().value("key") == key; });
  if (size != m_activities.size())
    emit changed();
}
void ActivityCoordinator::show(const QString &key, const QString &title,
                               int priority, int ttl, bool card) {
  m_notices.removeIf(
      [&](const QVariant &v) { return v.toMap().value("key") == key; });
  m_notices << QVariantMap{{"key", key},
                           {"title", title.left(160)},
                           {"priority", priority},
                           {"expires", m_now() + qBound(100, ttl, 60000)},
                           {"created", m_now()},
                           {"card", card},
                           {"seq", ++m_sequence}};
  std::stable_sort(m_notices.begin(), m_notices.end(),
                   [](const QVariant &a, const QVariant &b) {
                     auto x = a.toMap(), y = b.toMap();
                     return x["priority"].toInt() != y["priority"].toInt()
                                ? x["priority"].toInt() > y["priority"].toInt()
                                : x["seq"].toULongLong() >
                                      y["seq"].toULongLong();
                   });
  while (m_notices.size() > 8)
    m_notices.removeLast();
  m_expiry.start();
  emit changed();
}
void ActivityCoordinator::expire() {
  const auto size = m_notices.size();
  m_notices.removeIf([&](const QVariant &v) {
    return v.toMap()["expires"].toLongLong() <= m_now();
  });
  if (m_notices.isEmpty())
    m_expiry.stop();
  if (size != m_notices.size())
    emit changed();
}
void ActivityCoordinator::cycle() {
  for (int i = 0; i < m_activities.size(); ++i)
    if (m_activities[i].toMap()["key"] == selected()["key"]) {
      m_selection =
          m_activities[(i + 1) % m_activities.size()].toMap()["key"].toString();
      emit changed();
      return;
    }
}
void ActivityCoordinator::dismiss() {
  if (!m_notices.isEmpty()) {
    m_notices.removeFirst();
    emit changed();
  }
}

void ActivityCoordinator::select(const QString &key) {
  for (const auto &value : m_activities)
    if (value.toMap()["key"] == key) {
      m_selection = key;
      emit changed();
      return;
    }
}
