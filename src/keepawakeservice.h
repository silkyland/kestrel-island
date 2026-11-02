#pragma once
#include <QDBusConnection>
#include <QElapsedTimer>
#include <QObject>
#include <QTimer>
#include <QVariantMap>
#include <functional>

class KeepAwakeService : public QObject {
  Q_OBJECT
  Q_PROPERTY(QVariantMap snapshot READ snapshot NOTIFY changed)
public:
  explicit KeepAwakeService(QObject *parent = nullptr,
                            QDBusConnection bus = QDBusConnection::sessionBus(),
                            std::function<qint64()> clock = {});
  ~KeepAwakeService() override;
  QVariantMap snapshot() const;
  Q_INVOKABLE void start(int minutes);
  Q_INVOKABLE void stop();
signals:
  void changed();

private:
  void acquire(int which, quint64 generation);
  void release(int which, uint cookie, const QString &owner, int attempt = 0);
  QDBusConnection m_bus;
  struct Lease {
    int which;
    uint cookie;
    QString owner;
  };
  QList<Lease> m_cookies;
  int m_releases = 0;
  quint64 m_generation = 0;
  QString m_state = "off", m_error;
  int m_minutes = 0;
  std::function<qint64()> m_now;
  qint64 m_startedAt = 0;
  QTimer m_tick;
};
