#ifndef QTCOMPAT_H
#define QTCOMPAT_H

#include <QtGlobal>
#include <QString>
#include <QTextStream>
#include <QVariant>
#include <QMetaType>
#include <QMouseEvent>
#include <QPointF>

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
#include <QStringConverter>
#endif

namespace QtCompat {

// String split behavior (Qt::SkipEmptyParts was added in Qt 5.14)
#if QT_VERSION >= QT_VERSION_CHECK(5, 14, 0)
inline constexpr auto SkipEmptyParts = Qt::SkipEmptyParts;
#else
inline constexpr auto SkipEmptyParts = QString::SkipEmptyParts;
#endif

// UTF-8 stream encoding across Qt 5 and Qt 6
inline void setUtf8Encoding(QTextStream &stream)
{
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    stream.setEncoding(QStringConverter::Utf8);
#else
    stream.setCodec("UTF-8");
#endif
}

// QVariant type inspection without deprecated warnings
inline int variantTypeId(const QVariant &value)
{
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    return value.typeId();
#else
    return static_cast<int>(value.type());
#endif
}

// Mouse event local position across Qt 5.13 and Qt 6
inline QPointF mouseLocalPos(const QMouseEvent *event)
{
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    return event->position();
#else
    return event->localPos();
#endif
}

} // namespace QtCompat

#endif // QTCOMPAT_H
