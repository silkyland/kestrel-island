#pragma once
#include <QObject>
#include <QTimer>
#include <QVariantMap>
#include <functional>

class ActivityCoordinator : public QObject {
  Q_OBJECT
  Q_PROPERTY(QVariantList activities READ activities NOTIFY changed)
  Q_PROPERTY(QVariantMap selected READ selected NOTIFY changed)
  Q_PROPERTY(QVariantMap notice READ notice NOTIFY changed)
public:
  explicit ActivityCoordinator(QObject *parent = nullptr,
                               std::function<qint64()> clock = {});
  QVariantList activities() const { return m_activities; }
  QVariantMap selected() const;
  QVariantMap notice() const;
  void upsert(const QVariantMap &activity, bool select = false);
  void remove(const QString &key);
  void show(const QString &key, const QString &title, int priority,
            int ttl = 1600, bool card = false);
  Q_INVOKABLE void cycle();
  Q_INVOKABLE void select(const QString &key);
  Q_INVOKABLE void dismiss();
  void expire();
signals:
  void changed();

private:
  QVariantList m_activities, m_notices;
  QString m_selection;
  quint64 m_sequence = 0;
  std::function<qint64()> m_now;
  QTimer m_expiry;
};
