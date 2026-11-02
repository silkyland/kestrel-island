#include "taskactivityservice.h"
#include <QJsonDocument>
#include <QRegularExpression>
#include <chrono>

TaskActivityService::TaskActivityService(QObject *parent,
                                         std::function<qint64()> clock)
    : QObject(parent), m_now(std::move(clock)) {
  if (!m_now)
    m_now = [] {
      return std::chrono::duration_cast<std::chrono::milliseconds>(
                 std::chrono::steady_clock::now().time_since_epoch())
          .count();
    };
  m_tick.setInterval(1000);
  connect(&m_tick, &QTimer::timeout, this, &TaskActivityService::expire);
}
void TaskActivityService::setEnabled(bool enabled) {
  m_enabled = enabled;
  if (!enabled) {
    m_tasks.clear();
    m_rates.clear();
    m_tick.stop();
  }
  emit changed();
}
QVariantList TaskActivityService::tasks() const {
  QVariantList result;
  for (auto it = m_tasks.begin(); it != m_tasks.end(); ++it) {
    auto row = it.value().toMap();
    row["elapsed"] = qMax<qint64>(
        0, ((row["state"] == "completed" || row["state"] == "failed"
                 ? row["updated"].toLongLong()
                 : m_now()) -
            row["started"].toLongLong()) /
               1000);
    result << row;
  }
  return result;
}
bool TaskActivityService::report(const QVariantMap &event,
                                 const QString &sender) {
  if (!m_enabled ||
      QJsonDocument::fromVariant(event).toJson(QJsonDocument::Compact).size() >
          16384)
    return false;
  static const QSet<QString> fields = {"version", "id",      "state",
                                       "label",   "project", "seq"};
  for (auto it = event.begin(); it != event.end(); ++it)
    if (!fields.contains(it.key()))
      return false;
  const auto id = event.value("id").toString(),
             state = event.value("state").toString(),
             label = event.value("label").toString(),
             project = event.value("project").toString();
  if (event.value("version").metaType() == QMetaType::fromType<QString>() ||
      event.value("version").toDouble() != 1.0 ||
      event.value("id").metaType() != QMetaType::fromType<QString>() ||
      event.value("state").metaType() != QMetaType::fromType<QString>() ||
      event.value("label").metaType() != QMetaType::fromType<QString>() ||
      !QRegularExpression("^[A-Za-z0-9._:-]{1,128}$").match(id).hasMatch() ||
      !QStringList{"running", "completed", "failed", "attention"}.contains(
          state) ||
      label.isEmpty() || label.size() > 160 || project.size() > 80 ||
      project.contains('/') || project.contains('\\'))
    return false;
  for (auto c : label + project)
    if (c.category() == QChar::Other_Control)
      return false;
  if (m_rates.size() >= 128 && !m_rates.contains(sender))
    return false;
  auto &rate = m_rates[sender];
  if (m_now() - rate.first >= 1000)
    rate = {m_now(), 0};
  if (++rate.second > 20)
    return false;
  const auto old = m_tasks.value(id).toMap();
  if (old.isEmpty() && m_tasks.size() >= 8)
    return false;
  if (event.contains("seq")) {
    bool ok = false;
    const auto seq = event["seq"].toULongLong(&ok);
    if (!ok || (old.contains("seq") && seq <= old["seq"].toULongLong()))
      return false;
  } else if (old.contains("seq"))
    return false;
  auto row = event;
  row["started"] = old.isEmpty() ? m_now() : old["started"];
  row["updated"] = m_now();
  m_tasks[id] = row;
  m_tick.start();
  emit changed();
  if (state == "attention" && old.value("state") != state)
    emit notice(id, label + " · needs attention", 40);
  if ((state == "completed" || state == "failed") &&
      old.value("state") != state &&
      m_now() - row["started"].toLongLong() >= 60000)
    emit notice(
        id, label + (state == "completed" ? " · complete" : " · failed"), 20);
  return true;
}
void TaskActivityService::dismiss(const QString &id) {
  if (m_tasks.remove(id))
    emit changed();
  if (m_tasks.isEmpty())
    m_tick.stop();
}
void TaskActivityService::expire() {
  const auto now = m_now();
  for (auto it = m_tasks.begin(); it != m_tasks.end();) {
    auto row = it.value().toMap();
    auto state = row["state"].toString();
    if ((state == "completed" || state == "failed") &&
        now - row["updated"].toLongLong() >= 60000) {
      it = m_tasks.erase(it);
      continue;
    }
    if (state == "running" && now - row["updated"].toLongLong() >= 120000) {
      row["state"] = "stale";
      it.value() = row;
    }
    ++it;
  }
  for (auto it = m_rates.begin(); it != m_rates.end();)
    if (now - it->first >= 10000)
      it = m_rates.erase(it);
    else
      ++it;
  if (m_tasks.isEmpty())
    m_tick.stop();
  emit changed();
}
