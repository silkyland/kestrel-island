#include "islandcontrol.h"
#include "backend.h"
#include <QDBusMessage>
IslandControl::IslandControl(Backend *backend)
    : QObject(backend), m_backend(backend) {}
void IslandControl::Toggle() { emit m_backend->toggleRequested(); }
QVariantMap IslandControl::State() {
  return {{"expanded", m_backend->expanded()},
          {"display", m_backend->display()}};
}
void IslandControl::SetExpanded(bool expanded) {
  emit m_backend->expansionRequested(expanded);
}
bool IslandControl::ReportTask(const QVariantMap &event) {
  return m_backend->tasks()->report(
      event, calledFromDBus() ? message().service() : QString());
}
