#pragma once
#include <QDBusConnection>
#include <QDBusContext>
#include <QObject>
#include <QTimer>
#include <QVariantMap>

class SessionState : public QObject {
  Q_OBJECT
public:
  explicit SessionState(QObject *parent = nullptr, bool live = true,
                        QDBusConnection bus = QDBusConnection::sessionBus());
  bool safe() const {
    return m_lockedKnown && m_quietKnown && !m_locked && !m_quiet;
  }
  QVariantMap snapshot() const {
    return {{"lockedKnown", m_lockedKnown},
            {"locked", m_locked},
            {"quietKnown", m_quietKnown},
            {"quiet", m_quiet}};
  }
  void resync();
  void start();
  void setTestState(bool locked, bool quiet);
signals:
  void changed();
private slots:
  void locked(bool value);
  void aboutToLock();
  void properties(const QString &interface, const QVariantMap &changed,
                  const QStringList &invalidated);

private:
  QDBusConnection m_bus;
  bool m_locked = true, m_quiet = true, m_lockedKnown = false,
       m_quietKnown = false;
  quint64 m_generation = 0;
  bool m_started = false;
};
