#pragma once
#include <QDBusContext>
#include <QObject>
#include <QVariantMap>
class Backend;
class IslandControl : public QObject, public QDBusContext {
  Q_OBJECT
  Q_CLASSINFO("D-Bus Interface", "io.github.kestrel.Island")
public:
  explicit IslandControl(Backend *backend);
public slots:
  Q_SCRIPTABLE void Toggle();
  Q_SCRIPTABLE QVariantMap State();
  Q_SCRIPTABLE void SetExpanded(bool expanded);
  Q_SCRIPTABLE bool ReportTask(const QVariantMap &event);

private:
  Backend *m_backend;
};
