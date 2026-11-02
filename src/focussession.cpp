#include "focussession.h"
#include "sessionclock.h"

FocusSession::FocusSession(QObject *parent, std::function<qint64()> clock)
    : QObject(parent), m_now(std::move(clock)) {
  if (!m_now)
    m_now = sessionMilliseconds;
  m_tick.setInterval(250);
  connect(&m_tick, &QTimer::timeout, this, &FocusSession::tick);
}
int FocusSession::remaining() const {
  return running() ? qMax<qint64>(0, (m_deadline - m_now() + 999) / 1000)
                   : int((m_remainingMs + 999) / 1000);
}
QVariantMap FocusSession::snapshot() const {
  return {{"state", m_state},
          {"phase", m_phase},
          {"remaining", remaining()},
          {"pomodoro", m_pomodoro},
          {"round",
           qMax(1, m_rounds +
                       (m_phase == "work" && m_state != "completed" ? 1 : 0))}};
}
void FocusSession::run(int minutes) {
  m_remainingMs = qint64(minutes) * 60000;
  m_deadline = m_now() + m_remainingMs;
  m_state = "running";
  m_tick.start();
  emit changed();
}
void FocusSession::start(int minutes) {
  if (minutes < 1 || minutes > 180)
    return;
  m_pomodoro = false;
  m_phase = "timer";
  m_rounds = 0;
  run(minutes);
}
void FocusSession::pomodoro(int work, int shortBreak, int longBreak) {
  if (work < 1 || work > 180 || shortBreak < 1 || shortBreak > 180 ||
      longBreak < 1 || longBreak > 180)
    return;
  m_work = work;
  m_short = shortBreak;
  m_long = longBreak;
  m_pomodoro = true;
  m_phase = "work";
  m_rounds = 0;
  run(work);
}
void FocusSession::pause() {
  if (running()) {
    m_remainingMs = qMax<qint64>(0, m_deadline - m_now());
    m_state = "paused";
    m_tick.stop();
  } else if (m_state == "paused") {
    m_deadline = m_now() + m_remainingMs;
    m_state = "running";
    m_tick.start();
  } else
    return;
  emit changed();
}
void FocusSession::next() {
  if (!m_pomodoro || m_state != "completed")
    return;
  if (m_phase == "work") {
    m_phase = m_rounds % 4 == 0 ? "long break" : "short break";
    run(m_phase == "long break" ? m_long : m_short);
  } else {
    m_phase = "work";
    run(m_work);
  }
}
void FocusSession::cancel() {
  m_tick.stop();
  m_state = "idle";
  m_remainingMs = 0;
  m_phase = "timer";
  m_pomodoro = false;
  emit changed();
}
void FocusSession::tick() {
  if (running() && remaining() == 0) {
    m_tick.stop();
    m_remainingMs = 0;
    m_state = "completed";
    if (m_pomodoro && m_phase == "work")
      ++m_rounds;
    emit changed();
    emit finished();
  } else if (running())
    emit changed();
}
