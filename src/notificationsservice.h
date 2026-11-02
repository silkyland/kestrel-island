#pragma once
#include "sessionstate.h"
#include <QDBusContext>
#include <QTimer>
#include <QVariantList>

class NotificationsService : public QObject, public QDBusContext {
  Q_OBJECT
  Q_CLASSINFO("D-Bus Interface", "org.kde.NotificationWatcher")
  Q_PROPERTY(QVariantList entries READ entries NOTIFY changed)
  Q_PROPERTY(QString status READ status NOTIFY changed)
  Q_PROPERTY(bool enabled READ enabled NOTIFY changed)
  Q_PROPERTY(bool available READ available CONSTANT)
  Q_PROPERTY(QStringList allowlist READ allowlist NOTIFY changed)
public:
  explicit NotificationsService(
      QObject *parent = nullptr, bool live = true, bool validated = false,
      QDBusConnection bus = QDBusConnection::sessionBus());
  ~NotificationsService() override;
  QVariantList entries() const { return m_entries; }
  QString status() const;
  bool enabled() const { return m_enabled; }
  bool available() const { return m_validated; }
  QStringList allowlist() const { return m_allowlist; }
  void configure(bool enabled, const QStringList &allowlist);
  Q_INVOKABLE void clear();
  SessionState *session() { return &m_session; }
  bool ingest(const QString &sender, uint id, const QString &app, uint replaces,
              const QString &summary, const QString &body,
              const QVariantMap &hints = {});
public slots:
  Q_SCRIPTABLE void Notify(uint id, const QString &app, uint replaces,
                           const QString &icon, const QString &summary,
                           const QString &body, const QStringList &actions,
                           const QVariantMap &hints, int timeout);
  Q_SCRIPTABLE void CloseNotification(uint id);
signals:
  void changed();
private slots:
  void reconcile();
  void closed(uint id, uint reason);

private:
  void unregister();
  QDBusConnection m_bus;
  SessionState m_session;
  bool m_enabled = false, m_registered = false, m_registering = false,
       m_live = false, m_validated = false;
  quint64 m_generation = 0;
  quint64 m_ownerGeneration = 0;
  QString m_owner, m_error;
  QStringList m_allowlist;
  QVariantList m_entries;
  QTimer m_expiry;
};
