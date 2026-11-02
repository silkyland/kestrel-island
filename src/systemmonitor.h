#pragma once
#include <QElapsedTimer>
#include <QObject>
#include <QPointer>
#include <QVariantMap>

class QProcess;
class SystemMonitor : public QObject {
  Q_OBJECT
public:
  explicit SystemMonitor(QObject *parent = nullptr);
  ~SystemMonitor() override;
  QVariantMap snapshot() const { return m_snapshot; }
  void refresh(bool graphics = false);
  static QVariantMap parseMemory(const QByteArray &data);
  static QVariantList parseNvidia(const QByteArray &data);
  static QVariantList nvmeTemperatures(const QString &root = "/sys/class/nvme");
  static QVariantMap disk(const QString &path, const QString &label);
signals:
  void changed();

private:
  void graphics();
  void queryNvidia();
  void publishGraphics();
  QVariantMap m_snapshot;
  QVariantList m_cpuHistory, m_memoryHistory, m_disks, m_sysGpus, m_nvidia;
  quint64 m_lastTotal = 0, m_lastIdle = 0;
  QElapsedTimer m_diskClock, m_gpuClock;
  QPointer<QProcess> m_process;
  QString m_gpuError;
};
