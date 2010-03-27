#include "ContactUser.h"
#include <QSettings>
#include <QPixmapCache>
#include <QtDebug>

ContactUser::ContactUser(const QString &id, QObject *parent)
	: QObject(parent), uniqueID(id)
{
	Q_ASSERT(!uniqueID.isEmpty());

	loadSettings();

	QString host = readSetting("hostname").toString();
	quint16 port = (quint16)readSetting("port", 13535).toUInt();
	pConn = new ProtocolManager(host, port, this);
}

void ContactUser::loadSettings()
{
	QSettings settings;
	settings.beginGroup(QString("contacts/").append(uniqueID));

	pNickname = settings.value(QString("nickname"), uniqueID).toString();
	pSecret = settings.value(QString("secret")).toByteArray();
}

QVariant ContactUser::readSetting(const QString &key, const QVariant &defaultValue)
{
	QSettings settings;
	return settings.value(QString("contacts/%1/%2").arg(uniqueID, key), defaultValue);
}

void ContactUser::setNickname(const QString &nickname)
{
	if (pNickname == nickname)
		return;

	pNickname = nickname;

	QSettings settings;
	settings.setValue(QString("contacts/%1/nickname").arg(uniqueID), nickname);
}

void ContactUser::setSecret(const QByteArray &secret)
{
	if (pSecret == secret)
		return;

	Q_ASSERT(secret.size() == 16);
	pSecret = secret;

	QSettings settings;
	settings.setValue(QString("contacts/%1/secret").arg(uniqueID), secret);
}

QPixmap ContactUser::avatar(AvatarSize size)
{
	QPixmap re;
	if (QPixmapCache::find(cachedAvatar[size], &re))
		return re;

	QSettings settings;
	QString settingsKey = QString("contacts/%1/avatar").arg(uniqueID);
	if (size == TinyAvatar)
		settingsKey.append("-tiny");

	re = QPixmap::fromImage(settings.value(settingsKey).value<QImage>());

	cachedAvatar[size] = QPixmapCache::insert(re);
	return re;
}

void ContactUser::setAvatar(QImage image)
{
	if (image.width() > 160 || image.height() > 160)
		image = image.scaled(QSize(160, 160), Qt::KeepAspectRatio, Qt::SmoothTransformation);

	QSettings settings;
	QString key = QString("contacts/%1/avatar").arg(uniqueID);

	if (image.isNull())
	{
		settings.remove(key);
		settings.remove(key + "-tiny");
	}
	else
	{
		settings.setValue(key, image);

		QImage tiny = image.scaled(QSize(35, 35), Qt::KeepAspectRatio, Qt::SmoothTransformation);
		settings.setValue(key + "-tiny", tiny);
	}

	for (int i = 0; i < 2; ++i)
		QPixmapCache::remove(cachedAvatar[i]);
}

QString ContactUser::notesText() const
{
	QSettings settings;
	return settings.value(QString("contacts/%1/notes").arg(uniqueID)).toString();
}

void ContactUser::setNotesText(const QString &text)
{
	QSettings settings;
	QString key = QString("contacts/%1/notes").arg(uniqueID);

	if (text.isEmpty())
		settings.remove(key);
	else
		settings.setValue(key, text);
}
