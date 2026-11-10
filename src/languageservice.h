#pragma once
#include <QDBusConnection>
#include <QObject>
#include <QString>
#include <QStringList>

// Watches KDE's keyboard-layout interface (org.kde.keyboard /Layouts,
// org.kde.KeyboardLayouts). Plasma 6 emits index-based layoutChanged plus an
// a(sss) layout list; the older kded daemon emits currentLayoutChanged plus
// aa{ss}. Both resolve to a human-readable layout name reported through
// switched(). Read-only: the service never calls setters.
class LanguageService : public QObject {
  Q_OBJECT
public:
  explicit LanguageService(
      QObject *parent = nullptr, bool live = true,
      QDBusConnection bus = QDBusConnection::sessionBus());
  QString layout() const { return m_layout; }
  bool available() const { return m_available; }
  // Public so tests can inject a resolved name without a full bus fixture.
  void report(const QString &layout);
signals:
  void switched(const QString &layout);
private slots:
  void layoutIndex(uint index);
  void layoutName(const QString &name);
  void layoutListChanged();

private:
  void requestLayouts();
  void seed();
  QString resolveName(const QString &shortName) const;
  QDBusConnection m_bus;
  QString m_layout;
  QList<QStringList> m_names;
  int m_pendingIndex = -1;
  quint64 m_generation = 0;
  bool m_seeding = true;
  bool m_available = false;
};
