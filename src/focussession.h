#pragma once
#include <QObject>
#include <QTimer>
#include <QVariantMap>
#include <functional>

class FocusSession : public QObject {
  Q_OBJECT
  Q_PROPERTY(QVariantMap snapshot READ snapshot NOTIFY changed)
public:
  explicit FocusSession(QObject *parent = nullptr,
                        std::function<qint64()> clock = {});
  QVariantMap snapshot() const;
  int remaining() const;
  bool running() const { return m_state == "running"; }
  void start(int minutes);
  Q_INVOKABLE void pomodoro(int work = 25, int shortBreak = 5,
                            int longBreak = 15);
  Q_INVOKABLE void pause();
  Q_INVOKABLE void next();
  Q_INVOKABLE void cancel();
  void tick();
signals:
  void changed();
  void finished();

private:
  void run(int minutes);
  QString m_state = "idle", m_phase = "timer";
  int m_work = 25, m_short = 5, m_long = 15, m_rounds = 0;
  qint64 m_remainingMs = 0;
  bool m_pomodoro = false;
  qint64 m_deadline = 0;
  std::function<qint64()> m_now;
  QTimer m_tick;
};
