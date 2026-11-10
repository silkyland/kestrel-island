#include "languageservice.h"
#include <QDBusArgument>
#include <QDBusMessage>
#include <QDBusPendingCallWatcher>
#include <utility>

namespace {
const char kService[] = "org.kde.keyboard";
const char kPath[] = "/Layouts";
const char kInterface[] = "org.kde.KeyboardLayouts";

QDBusMessage layoutsCall(const char *method) {
  return QDBusMessage::createMethodCall(kService, kPath, kInterface, method);
}
// Entries are [short name, display name, long name]; the longest honest
// description wins and a bare short code is shown uppercased (US, TH).
QString displayName(const QStringList &entry) {
  if (!entry.value(2).isEmpty())
    return entry[2];
  if (!entry.value(1).isEmpty())
    return entry[1];
  return entry.value(0).toUpper();
}
QList<QStringList> parseLayoutList(const QVariant &value) {
  QList<QStringList> entries;
  if (value.metaType() != QMetaType::fromType<QDBusArgument>())
    return entries;
  const auto root = value.value<QDBusArgument>();
  if (root.currentType() != QDBusArgument::ArrayType)
    return entries;
  root.beginArray();
  while (!root.atEnd()) {
    QStringList entry;
    if (root.currentType() == QDBusArgument::StructureType) {
      root.beginStructure();
      while (!root.atEnd()) {
        QString part;
        root >> part;
        entry << part;
      }
      root.endStructure();
    } else if (root.currentType() == QDBusArgument::MapType) {
      QVariantMap map;
      root >> map;
      // kded entries are either {"layout","name"} pairs or a single
      // {shortName: longName} map.
      const auto keys = map.keys();
      entry << map.value("layout", keys.value(0)).toString() << QString()
            << map.value("name",
                         map.value("longName", map.value(keys.value(0))))
                   .toString();
    } else {
      break;
    }
    while (entry.size() < 3)
      entry << QString();
    entries << entry;
  }
  root.endArray();
  return entries;
}
} // namespace

LanguageService::LanguageService(QObject *parent, bool live,
                                 QDBusConnection bus)
    : QObject(parent), m_bus(bus) {
  if (!live)
    return;
  m_bus.connect(kService, kPath, kInterface, "layoutChanged", this,
                SLOT(layoutIndex(uint)));
  m_bus.connect(kService, kPath, kInterface, "currentLayoutChanged", this,
                SLOT(layoutName(QString)));
  m_bus.connect(kService, kPath, kInterface, "layoutListChanged", this,
                SLOT(layoutListChanged()));
  requestLayouts();
}

void LanguageService::report(const QString &layout) {
  const auto name = layout.trimmed();
  if (name.isEmpty() || name == m_layout)
    return;
  m_layout = name;
  emit switched(m_layout);
}

void LanguageService::layoutIndex(uint index) {
  if (index < uint(m_names.size()))
    report(displayName(m_names[index]));
  else {
    m_pendingIndex = int(index);
    requestLayouts();
  }
}

void LanguageService::layoutName(const QString &name) {
  report(resolveName(name));
}

void LanguageService::layoutListChanged() { requestLayouts(); }

void LanguageService::requestLayouts() {
  const auto generation = ++m_generation;
  auto *call = new QDBusPendingCallWatcher(
      m_bus.asyncCall(layoutsCall("getLayoutsList"), 1500), this);
  connect(call, &QDBusPendingCallWatcher::finished, this,
          [this, generation](QDBusPendingCallWatcher *done) {
            const auto message = done->reply();
            done->deleteLater();
            if (generation != m_generation)
              return;
            if (message.type() != QDBusMessage::ErrorMessage &&
                !message.arguments().isEmpty()) {
              m_names = parseLayoutList(message.arguments().first());
              m_available = !m_names.isEmpty();
            }
            if (m_pendingIndex >= 0) {
              const auto index = std::exchange(m_pendingIndex, -1);
              if (index >= 0 && index < m_names.size())
                report(displayName(m_names[index]));
            } else if (m_seeding) {
              m_seeding = false;
              seed();
            }
          });
}

void LanguageService::seed() {
  const auto generation = ++m_generation;
  auto *call = new QDBusPendingCallWatcher(
      m_bus.asyncCall(layoutsCall("getLayout"), 1500), this);
  connect(call, &QDBusPendingCallWatcher::finished, this,
          [this, generation](QDBusPendingCallWatcher *done) {
            const auto message = done->reply();
            done->deleteLater();
            if (generation != m_generation || message.arguments().isEmpty())
              return;
            if (message.type() != QDBusMessage::ErrorMessage) {
              const auto index = message.arguments().first().toUInt();
              if (index < uint(m_names.size()))
                m_layout = displayName(m_names[index]);
              return;
            }
            // Older daemons expose getCurrentLayout (string) instead.
            const auto gen = ++m_generation;
            auto *next = new QDBusPendingCallWatcher(
                m_bus.asyncCall(layoutsCall("getCurrentLayout"), 1500), this);
            connect(next, &QDBusPendingCallWatcher::finished, this,
                    [this, gen](QDBusPendingCallWatcher *fallback) {
                      const auto reply = fallback->reply();
                      fallback->deleteLater();
                      if (gen != m_generation ||
                          reply.type() == QDBusMessage::ErrorMessage ||
                          reply.arguments().isEmpty())
                        return;
                      m_layout = resolveName(reply.arguments().first().toString());
                    });
          });
}

QString LanguageService::resolveName(const QString &shortName) const {
  for (const auto &entry : m_names)
    if (entry.first() == shortName)
      return displayName(entry);
  return shortName.toUpper();
}
