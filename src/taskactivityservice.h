#pragma once
#include <QElapsedTimer>
#include <QObject>
#include <QTimer>
#include <QVariantMap>
#include <functional>

class TaskActivityService : public QObject {
  Q_OBJECT
  Q_PROPERTY(QVariantList tasks READ tasks NOTIFY changed)
  Q_PROPERTY(bool enabled READ enabled WRITE setEnabled NOTIFY changed)
public:
  explicit TaskActivityService(QObject *parent = nullptr,
                               std::function<qint64()> clock = {});
  QVariantList tasks() const;
  bool enabled() const { return m_enabled; }
  void setEnabled(bool enabled);
  bool report(const QVariantMap &event, const QString &sender = {});
  Q_INVOKABLE void dismiss(const QString &id);
  void expire();
signals:
  void changed();
  void notice(const QString &id, const QString &title, int priority);

private:
  bool m_enabled = false;
  QVariantMap m_tasks;
  QHash<QString, QPair<qint64, int>> m_rates;
  std::function<qint64()> m_now;
  QTimer m_tick;
};
