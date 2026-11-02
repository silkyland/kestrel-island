#include "systemmonitor.h"
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QProcess>
#include <QProcessEnvironment>
#include <QSet>
#include <QStandardPaths>
#include <QStorageInfo>
#include <QTimer>
#include <algorithm>
#include <cmath>

namespace {
QByteArray read(const QString &path) {
  QFile file(path);
  return file.open(QIODevice::ReadOnly) ? file.readAll() : QByteArray();
}
double number(const QString &path, double scale = 1) {
  bool ok = false;
  auto value = read(path).trimmed().toDouble(&ok);
  return ok && std::isfinite(value) && value >= 0 ? value / scale : -1;
}
double metric(const QString &text) {
  bool ok = false;
  auto value = text.trimmed().toDouble(&ok);
  return ok && std::isfinite(value) && value >= 0 ? value : -1;
}
QString pciName(const QString &vendor, const QString &device) {
  static const auto names = [] {
    QHash<QString, QString> result;
    QString vendor;
    for (const auto &line : read("/usr/share/hwdata/pci.ids").split('\n')) {
      if (line.isEmpty() || line.startsWith('#'))
        continue;
      if (line[0] != '\t' && line.size() > 6) {
        vendor = QString::fromLatin1(line.left(4)).toLower();
      } else if (line.startsWith('\t') && !line.startsWith("\t\t") &&
                 line.size() > 7)
        result[vendor + ":" + QString::fromLatin1(line.mid(1, 4)).toLower()] =
            QString::fromUtf8(line.mid(7)).trimmed();
    }
    return result;
  }();
  return names.value(vendor + ":" + device);
}
void history(QVariantList &values, double value) {
  values << value;
  while (values.size() > 60)
    values.removeFirst();
}
} // namespace

SystemMonitor::SystemMonitor(QObject *parent) : QObject(parent) {
  m_snapshot = {{"ready", false},
                {"cpu", QVariantMap{{"ready", false}, {"percent", 0}}},
                {"memory", QVariantMap()},
                {"gpus", QVariantList()},
                {"disks", QVariantList()},
                {"nvme", QVariantList()},
                {"cpuHistory", QVariantList()},
                {"memoryHistory", QVariantList()}};
}
SystemMonitor::~SystemMonitor() {
  if (m_process) {
    m_process->disconnect(this);
    m_process->kill();
    m_process->waitForFinished(1000);
  }
}
QVariantMap SystemMonitor::parseMemory(const QByteArray &data) {
  QHash<QByteArray, qulonglong> fields;
  for (const auto &line : data.split('\n')) {
    auto pieces = line.simplified().split(' ');
    if (pieces.size() >= 2)
      fields[pieces[0]] = pieces[1].toULongLong() * 1024;
  }
  const auto total = fields.value("MemTotal:"),
             available = qMin(total, fields.value("MemAvailable:")),
             used = total - available;
  return {{"total", total},
          {"available", available},
          {"used", used},
          {"percent", total ? 100.0 * used / total : 0.0},
          {"ready", total > 0 && fields.contains("MemAvailable:")},
          {"swapTotal", fields.value("SwapTotal:")},
          {"swapUsed",
           fields.value("SwapTotal:") -
               qMin(fields.value("SwapTotal:"), fields.value("SwapFree:"))}};
}
QVariantMap SystemMonitor::disk(const QString &path, const QString &label) {
  QStorageInfo volume(path);
  volume.refresh();
  if (!volume.isValid() || !volume.isReady() || volume.bytesTotal() <= 0)
    return {};
  const auto total = volume.bytesTotal(),
             free = qBound<qint64>(0, volume.bytesFree(), total),
             available = qBound<qint64>(0, volume.bytesAvailable(), free);
  return {{"label", label},
          {"path", volume.rootPath()},
          {"device", QString::fromLocal8Bit(volume.device())},
          {"filesystem", QString::fromLatin1(volume.fileSystemType())},
          {"total", total},
          {"used", total - free},
          {"available", available},
          {"percent", 100.0 * (total - free) / total}};
}
QVariantList SystemMonitor::parseNvidia(const QByteArray &data) {
  QVariantList result;
  for (auto line : QString::fromUtf8(data).split('\n', Qt::SkipEmptyParts)) {
    const auto fields = line.split(',');
    if (fields.size() != 8 || fields[1].trimmed().isEmpty() ||
        !fields[2].contains(':'))
      continue;
    const auto used = metric(fields[4]), total = metric(fields[5]);
    result << QVariantMap{{"id", fields[2].trimmed().toLower().right(12)},
                          {"name", fields[1].trimmed()},
                          {"driver", "nvidia"},
                          {"percent", metric(fields[3])},
                          {"memoryUsed", used >= 0 ? used * 1048576 : -1},
                          {"memoryTotal", total >= 0 ? total * 1048576 : -1},
                          {"temperature", metric(fields[6])},
                          {"power", metric(fields[7])},
                          {"source", "nvidia-smi"},
                          {"error", QString()}};
  }
  return result;
}
QVariantList SystemMonitor::nvmeTemperatures(const QString &root) {
  QVariantList result;
  for (const auto &controller : QDir(root).entryList(
           {"nvme*"}, QDir::Dirs | QDir::NoDotAndDotDot, QDir::Name)) {
    bool numeric = false;
    controller.mid(4).toUInt(&numeric);
    if (!numeric)
      continue;
    const auto base = root + "/" + controller;
    double temperature = -1;
    for (const auto &hw : QDir(base).entryList(
             {"hwmon*"}, QDir::Dirs | QDir::NoDotAndDotDot)) {
      // NVMe hwmon channel 1 is the controller's composite temperature.
      temperature = number(base + "/" + hw + "/temp1_input", 1000);
      if (temperature >= 0)
        break;
    }
    const auto model = QString::fromUtf8(read(base + "/model")).simplified();
    result << QVariantMap{{"id", controller},
                          {"name", model.isEmpty() ? controller : model},
                          {"temperature", temperature}};
  }
  return result;
}
void SystemMonitor::refresh(bool includeGraphics) {
  const auto stat =
      read("/proc/stat").split('\n').value(0).simplified().split(' ');
  quint64 total = 0;
  for (int i = 1; i < qMin(9, int(stat.size())); ++i)
    total += stat[i].toULongLong();
  const auto idle = stat.value(4).toULongLong() + stat.value(5).toULongLong();
  const bool ready = m_lastTotal && total > m_lastTotal && idle >= m_lastIdle;
  const auto percent =
      ready ? qBound(0.0,
                     100.0 * (1.0 - double(idle - m_lastIdle) /
                                        double(total - m_lastTotal)),
                     100.0)
            : m_snapshot.value("cpu").toMap().value("percent").toDouble();
  m_lastTotal = total;
  m_lastIdle = idle;
  QString model, physical, core;
  QSet<QString> cores;
  int threads = 0;
  double mhz = 0;
  int frequencies = 0;
  for (auto line : QString::fromUtf8(read("/proc/cpuinfo")).split('\n')) {
    const auto split = line.indexOf(':');
    if (split < 0)
      continue;
    const auto key = line.left(split).trimmed(),
               value = line.mid(split + 1).trimmed();
    if (key == "model name" && model.isEmpty())
      model = value;
    if (key == "processor")
      ++threads;
    if (key == "physical id")
      physical = value;
    if (key == "core id") {
      core = value;
      cores.insert(physical + ":" + core);
    }
    if (key == "cpu MHz") {
      bool ok = false;
      auto n = value.toDouble(&ok);
      if (ok) {
        mhz += n;
        ++frequencies;
      }
    }
  }
  double temperature = -1;
  for (const auto &entry :
       QDir("/sys/class/hwmon").entryList(QDir::Dirs | QDir::NoDotAndDotDot)) {
    const auto base = "/sys/class/hwmon/" + entry;
    const auto name = read(base + "/name").trimmed();
    if (name == "k10temp" || name == "coretemp" || name == "zenpower") {
      temperature = number(base + "/temp1_input", 1000);
      if (temperature >= 0)
        break;
    }
  }
  auto memory = parseMemory(read("/proc/meminfo"));
  if (ready) {
    history(m_cpuHistory, percent);
    if (memory["ready"].toBool())
      history(m_memoryHistory, memory["percent"].toDouble());
  }
  m_snapshot["cpu"] = QVariantMap{
      {"ready", ready || m_snapshot["cpu"].toMap()["ready"].toBool()},
      {"percent", percent},
      {"name", model},
      {"cores", cores.size()},
      {"threads", threads},
      {"mhz", frequencies ? mhz / frequencies : -1},
      {"temperature", temperature}};
  m_snapshot["memory"] = memory;
  m_snapshot["cpuHistory"] = m_cpuHistory;
  m_snapshot["memoryHistory"] = m_memoryHistory;
  if (!m_diskClock.isValid() || m_diskClock.elapsed() >= 10000) {
    m_disks.clear();
    auto root = disk("/", "System");
    auto home = disk(QDir::homePath(), "Home");
    if (!root.isEmpty() && !home.isEmpty() &&
        root["device"] == home["device"] && root["total"] == home["total"]) {
      root["label"] = "System / Home";
      root["paths"] = "/ · " + home["path"].toString();
      m_disks << root;
    } else {
      if (!root.isEmpty())
        m_disks << root;
      if (!home.isEmpty())
        m_disks << home;
    }
    for (const auto &volume : QStorageInfo::mountedVolumes()) {
      const auto path = volume.rootPath();
      if (!path.startsWith("/run/media/") && !path.startsWith("/media/") &&
          !path.startsWith("/mnt/"))
        continue;
      const auto type = volume.fileSystemType();
      if (!QList<QByteArray>{"ext4", "btrfs", "xfs", "vfat", "exfat", "ntfs",
                             "fuseblk", "f2fs"}
               .contains(type))
        continue;
      bool duplicate = false;
      for (const auto &value : m_disks)
        if (value.toMap()["device"] == QString::fromLocal8Bit(volume.device()))
          duplicate = true;
      if (!duplicate) {
        auto entry = disk(
            path, volume.displayName().isEmpty() ? path : volume.displayName());
        if (!entry.isEmpty())
          m_disks << entry;
      }
    }
    m_diskClock.restart();
  }
  m_snapshot["disks"] = m_disks;
  m_snapshot["ready"] = true;
  m_snapshot["sampledAt"] = QDateTime::currentMSecsSinceEpoch();
  if (includeGraphics) {
    m_snapshot["nvme"] = nvmeTemperatures();
    graphics();
  }
  emit changed();
}
void SystemMonitor::graphics() {
  m_sysGpus.clear();
  for (const auto &card :
       QDir("/sys/class/drm").entryList(QDir::Dirs | QDir::NoDotAndDotDot)) {
    if (!card.startsWith("card") || card.mid(4).isEmpty() ||
        card.mid(4).toInt() < 0 || card.contains('-'))
      continue;
    bool numeric = false;
    card.mid(4).toInt(&numeric);
    if (!numeric)
      continue;
    const auto base = "/sys/class/drm/" + card + "/device";
    auto vendor = QString::fromLatin1(read(base + "/vendor").trimmed())
                      .mid(2)
                      .toLower(),
         device = QString::fromLatin1(read(base + "/device").trimmed())
                      .mid(2)
                      .toLower();
    if (vendor.isEmpty())
      continue;
    QString id, driver;
    for (const auto &line : read(base + "/uevent").split('\n')) {
      if (line.startsWith("PCI_SLOT_NAME="))
        id = QString::fromLatin1(line.mid(14)).toLower();
      if (line.startsWith("DRIVER="))
        driver = QString::fromLatin1(line.mid(7));
    }
    auto name = pciName(vendor, device);
    if (name.isEmpty())
      name = (vendor == "1002"   ? "AMD GPU"
              : vendor == "10de" ? "NVIDIA GPU"
                                 : "Graphics adapter") +
             QString(" · %1").arg(card);
    double temperature = -1;
    for (const auto &hw :
         QDir(base + "/hwmon").entryList(QDir::Dirs | QDir::NoDotAndDotDot)) {
      temperature = number(base + "/hwmon/" + hw + "/temp1_input", 1000);
      if (temperature >= 0)
        break;
    }
    m_sysGpus << QVariantMap{
        {"id", id},
        {"name", name},
        {"driver", driver},
        {"percent", number(base + "/gpu_busy_percent")},
        {"memoryUsed", number(base + "/mem_info_vram_used")},
        {"memoryTotal", number(base + "/mem_info_vram_total")},
        {"temperature", temperature},
        {"power", -1},
        {"source", driver == "nvidia" ? "nvidia-smi" : "sysfs"},
        {"error", QString()}};
  }
  publishGraphics();
  queryNvidia();
}
void SystemMonitor::publishGraphics() {
  auto merged = m_sysGpus;
  for (const auto &value : m_nvidia) {
    const auto gpu = value.toMap();
    bool found = false;
    for (auto &entry : merged)
      if (entry.toMap()["id"] == gpu["id"]) {
        entry = gpu;
        found = true;
        break;
      }
    if (!found)
      merged << gpu;
  }
  std::stable_sort(merged.begin(), merged.end(), [](auto a, auto b) {
    return a.toMap()["driver"] == "nvidia" && b.toMap()["driver"] != "nvidia";
  });
  m_snapshot["gpus"] = merged;
  m_snapshot["gpuError"] = m_gpuError;
}
void SystemMonitor::queryNvidia() {
  bool nvidia = false;
  for (const auto &value : m_sysGpus)
    if (value.toMap()["driver"] == "nvidia")
      nvidia = true;
  if (!nvidia || m_process ||
      (m_gpuClock.isValid() && m_gpuClock.elapsed() < 2000))
    return;
  m_gpuClock.restart();
  if (QStandardPaths::findExecutable("nvidia-smi").isEmpty()) {
    m_gpuError = "NVIDIA telemetry unavailable: nvidia-smi is not installed";
    publishGraphics();
    return;
  }
  auto *process = new QProcess(this);
  m_process = process;
  auto environment = QProcessEnvironment::systemEnvironment();
  environment.insert("LC_ALL", "C");
  process->setProcessEnvironment(environment);
  connect(process, &QProcess::finished, this,
          [this, process](int code, QProcess::ExitStatus status) {
            const auto values = parseNvidia(process->readAllStandardOutput());
            if (code == 0 && status == QProcess::NormalExit &&
                !values.isEmpty()) {
              m_nvidia = values;
              m_gpuError.clear();
            } else {
              m_nvidia.clear();
              m_gpuError = "NVIDIA driver did not return telemetry";
            }
            m_process = nullptr;
            process->deleteLater();
            publishGraphics();
            emit changed();
          });
  connect(process, &QProcess::errorOccurred, this, [this, process](auto error) {
    if (error == QProcess::FailedToStart) {
      m_gpuError = "NVIDIA telemetry could not start";
      m_nvidia.clear();
      m_process = nullptr;
      process->deleteLater();
      publishGraphics();
      emit changed();
    }
  });
  process->start("nvidia-smi",
                 {"--query-gpu=index,name,pci.bus_id,utilization.gpu,memory."
                  "used,memory.total,temperature.gpu,power.draw",
                  "--format=csv,noheader,nounits"});
  QTimer::singleShot(1500, process, [process] {
    if (process->state() != QProcess::NotRunning)
      process->kill();
  });
}
