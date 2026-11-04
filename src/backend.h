#pragma once

#include "activitycoordinator.h"
#include "focussession.h"
#include "keepawakeservice.h"
#include "notificationsservice.h"
#include "systemmonitor.h"
#include "taskactivityservice.h"
#include <QDBusMessage>
#include <QElapsedTimer>
#include <QObject>
#include <QPointer>
#include <QQueue>
#include <QSettings>
#include <QTimer>
#include <QVariantMap>

class QProcess;
class QQuickWindow;
class QSystemTrayIcon;
class QMenu;

class Backend : public QObject {
  Q_OBJECT
  Q_PROPERTY(QVariantMap media READ media NOTIFY mediaChanged)
  Q_PROPERTY(double cpu READ cpu NOTIFY statsChanged)
  Q_PROPERTY(double memory READ memory NOTIFY statsChanged)
  Q_PROPERTY(QVariantMap statistics READ statistics NOTIFY statsChanged)
  Q_PROPERTY(int preferredPage READ preferredPage NOTIFY preferencesChanged)
  Q_PROPERTY(int volume READ volume NOTIFY volumeChanged)
  Q_PROPERTY(bool muted READ muted NOTIFY volumeChanged)
  Q_PROPERTY(bool audioAvailable READ audioAvailable NOTIFY volumeChanged)
  Q_PROPERTY(int remaining READ remaining NOTIFY timerChanged)
  Q_PROPERTY(bool timerRunning READ timerRunning NOTIFY timerChanged)
  Q_PROPERTY(QString message READ message NOTIFY messageChanged)
  Q_PROPERTY(bool hoverExpand READ hoverExpand NOTIFY preferencesChanged)
  Q_PROPERTY(bool animations READ animations NOTIFY preferencesChanged)
  Q_PROPERTY(
      double animationScale READ animationScale NOTIFY preferencesChanged)
  Q_PROPERTY(QStringList displays READ displays NOTIFY preferencesChanged)
  Q_PROPERTY(QString display READ display NOTIFY preferencesChanged)
  Q_PROPERTY(int topGap READ topGap NOTIFY preferencesChanged)
  Q_PROPERTY(ActivityCoordinator *activity READ activity CONSTANT)
  Q_PROPERTY(FocusSession *focus READ focus CONSTANT)
  Q_PROPERTY(KeepAwakeService *awake READ awake CONSTANT)
  Q_PROPERTY(TaskActivityService *tasks READ tasks CONSTANT)
  Q_PROPERTY(NotificationsService *notifications READ notifications CONSTANT)
  Q_PROPERTY(bool audioFeedback READ audioFeedback NOTIFY preferencesChanged)
  Q_PROPERTY(bool micMuted READ micMuted NOTIFY volumeChanged)
  Q_PROPERTY(bool micAvailable READ micAvailable NOTIFY volumeChanged)
  Q_PROPERTY(double playbackPosition READ playbackPosition NOTIFY playbackTick)
  Q_PROPERTY(
      qulonglong seekGeneration READ seekGeneration NOTIFY seekContextChanged)
public:
  explicit Backend(QObject *parent = nullptr, bool live = true);
  ~Backend() override;
  QVariantMap media() const { return m_media; }
  double cpu() const { return m_cpu; }
  double memory() const { return m_memory; }
  QVariantMap statistics() const { return m_system.snapshot(); }
  int preferredPage() const {
    return m_settings.value("ui/lastPage", 0).toInt();
  }
  Q_INVOKABLE void setPreferredPage(int page);
  Q_INVOKABLE void setCurrentPage(int page);
  void previewSystem();
  int volume() const { return m_volume; }
  bool muted() const { return m_muted; }
  bool audioAvailable() const { return m_audioAvailable; }
  int remaining() const;
  bool expanded() const { return m_expanded; }
  bool timerRunning() const { return m_focus.running(); }
  QString message() const { return m_message; }
  ActivityCoordinator *activity() { return &m_activity; }
  FocusSession *focus() { return &m_focus; }
  KeepAwakeService *awake() { return &m_awake; }
  TaskActivityService *tasks() { return &m_tasks; }
  NotificationsService *notifications() { return &m_notifications; }
  bool audioFeedback() const {
    return m_settings.value("audioFeedback", false).toBool();
  }
  bool micMuted() const { return m_micMuted; }
  bool micAvailable() const { return m_micAvailable; }
  double playbackPosition() const;
  qulonglong seekGeneration() const { return m_seekGeneration; }
  Q_INVOKABLE void seek(double seconds, const QString &service,
                        const QString &trackId, qulonglong generation);
  Q_INVOKABLE void setAudioFeedback(bool value);
  Q_INVOKABLE void setTasksEnabled(bool value);
  Q_INVOKABLE void copyTaskExample();
  Q_INVOKABLE void configureNotifications(bool enabled, const QString &apps);
  Q_INVOKABLE void toggleMicMute();
  bool hoverExpand() const {
    return m_settings.value("hoverExpand", true).toBool();
  }
  bool animations() const {
    return m_settings.value("animations", true).toBool();
  }
  double animationScale() const {
    return animations() ? m_animationFactor : 0;
  }
  QStringList displays() const;
  QString display() const;
  int topGap() const { return m_settings.value("topGap", 0).toInt(); }
  Q_INVOKABLE void chooseDisplay(const QString &name);
  Q_INVOKABLE void setTopGap(int gap);
  Q_INVOKABLE void setHoverExpand(bool enabled);
  Q_INVOKABLE void beginMove();
  Q_INVOKABLE void finishMove(const QString &name);
  void attach(QQuickWindow *window, QSystemTrayIcon *tray);
  void position();
  Q_INVOKABLE void mediaAction(const QString &method);
  Q_INVOKABLE void setVolume(int volume);
  Q_INVOKABLE void toggleMute();
  Q_INVOKABLE void startTimer(int minutes);
  Q_INVOKABLE void pauseTimer();
  Q_INVOKABLE void cancelTimer();
  Q_INVOKABLE void updateInputRegion(double x, double width, double height);
  Q_INVOKABLE void openMenu();
  Q_INVOKABLE void setExpanded(bool expanded);
  void setMessage(const QString &message);
  static QVariantMap decodeProperties(const QDBusMessage &reply);
  static QVariantMap normalizeMedia(const QString &service,
                                    const QVariantMap &properties);
  static QVariantMap currentMedia(const QVariantList &players,
                                  const QString &previous = {});
signals:
  void mediaChanged();
  void statsChanged();
  void volumeChanged();
  void timerChanged();
  void timerFinished();
  void messageChanged();
  void preferencesChanged();
  void toggleRequested();
  void expansionRequested(bool expanded);
  void settingsRequested();
  void playbackTick();
  void seekContextChanged();
private slots:
  void mediaProperties(const QString &interface, const QVariantMap &,
                       const QStringList &);
  void mediaSeeked(qlonglong);
  void mediaOwnerChanged(const QString &name, const QString &, const QString &);

private:
  void pollMedia();
  void pollStats();
  void pollVolume();
  void audioCommand(const QStringList &args);
  void buildMenu();
  void pollMic();
  void startAudioObserver();
  QSettings m_settings;
  SystemMonitor m_system{this};
  int m_currentPage = 0;
  bool m_systemPreview = false;
  ActivityCoordinator m_activity{this};
  FocusSession m_focus{this};
  KeepAwakeService m_awake{this};
  TaskActivityService m_tasks{this};
  NotificationsService m_notifications;
  QPointer<QQuickWindow> m_window;
  QSystemTrayIcon *m_tray = nullptr;
  QPointer<QMenu> m_menu;
  QTimer m_poll;
  QTimer m_mediaRefresh, m_playbackTimer, m_messageExpiry, m_audioRefresh,
      m_audioRestart;
  QProcess *m_audioObserver = nullptr;
  bool m_live = false, m_micMuted = false, m_micAvailable = false,
       m_micBusy = false, m_haveAudio = false;
  int m_audioBackoff = 1000;
  QElapsedTimer m_mediaSample;
  QVariantMap m_media;
  QString m_message;
  QHash<QString, QVariantMap> m_previousMedia;
  quint64 m_mediaGeneration = 0;
  quint64 m_seekGeneration = 1;
  double m_cpu = 0, m_memory = 0;
  int m_volume = 0;
  bool m_muted = false, m_audioAvailable = false;
  bool m_mediaBusy = false, m_audioBusy = false, m_commandBusy = false;
  bool m_mediaDirty = false;
  bool m_expanded = false;
  QQueue<QStringList> m_audioQueue;
  bool m_moving = false;
  QString m_dropDisplay;
  double m_animationFactor = 1.0;
};
