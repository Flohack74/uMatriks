#include <QApplication>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonArray>
#include <QSqlDatabase>
#include <QStringList>
#include <QSqlQuery>
#include <QSqlRecord>
#include <QTextStream>

#include "i18n.h"
#include "pushhelper.h"

PushHelper::PushHelper(QString appId, QString infile, QString outfile,
                       QObject *parent) : QObject(parent) {
    setlocale(LC_ALL, "");
    textdomain(GETTEXT_DOMAIN.toStdString().c_str());

    connect(&mPushClient, SIGNAL(persistentCleared()),
                    this, SLOT(notificationDismissed()));

    mPushClient.setAppId(appId);
    mPushClient.registerApp(appId);
    mInfile = infile;
    mOutfile = outfile;
}

PushHelper::~PushHelper() {
}

void PushHelper::process() {
    QString tag = "";

    QJsonObject pushMessage = readPushMessage(mInfile);
    mPostalMessage = pushToPostalMessage(pushMessage, tag);
    if (!tag.isEmpty()) {
        dismissNotification(tag);
    }

    // persistentCleared not called!
    notificationDismissed();
}

void PushHelper::notificationDismissed() {
    writePostalMessage(mPostalMessage, mOutfile);
    Q_EMIT done(); // Why does this not work?
}

QJsonObject PushHelper::readPushMessage(const QString &filename) {
    QFile file(filename);
    file.open(QIODevice::ReadOnly | QIODevice::Text);

    QString val = file.readAll();
    file.close();
    return QJsonDocument::fromJson(val.toUtf8()).object();
}

void PushHelper::writePostalMessage(const QJsonObject &postalMessage, const QString &filename) {
    QFile out;
    out.setFileName(filename);
    out.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate);

    QTextStream(&out) << QJsonDocument(postalMessage).toJson();
    out.close();
}

void PushHelper::dismissNotification(const QString &tag) {
    QStringList tags;
    tags << tag;
    mPushClient.clearPersistent(tags);
}

QJsonObject PushHelper::pushToPostalMessage(const QJsonObject &push, QString &tag) {
    QString summary = "";
    QString body = "";
    qint32 count = 0;

    QJsonObject message = push["message"].toObject();
    QJsonObject custom = message["custom"].toObject();

    QString key = "";
    if (message.keys().contains("loc_key")) {
        key = message["loc_key"].toString();    // no-i18n
    }

    QJsonArray args;
    if (message.keys().contains("loc_args")) {
        args = message["loc_args"].toArray();   // no-i18n
    }

	//BEGIN to customize here!
	
	summary = tg;
	body = N_("You have a new message");

	//Notification pop-up parameters

	//Actions
	//Clicking on the notifications can trigger an action (opening the App)
    QJsonArray actions = QJsonArray();
    QString actionUri = QString("matriks://chat/%1").arg(chatId);
    actions.append(actionUri);

	//Card
    QJsonObject card;
    card["summary"] = summary;  // no-i18n
    card["body"]    = body;     // no-i18n
    card["actions"] = actions;  // no-i18n
    card["popup"]   = true;     // no-i18n // TODO make setting
    card["persist"] = true;     // no-i18n // TODO make setting

	//Show count of unread messages on top of the app symbol?
	auto unreadCount = 0;
    QJsonObject emblem;
    emblem["count"] = unreadCount;                    // no-i18n
    emblem["visible"] = unreadCount > 0;              // no-i18n

	//The final object to be passed to Postal
    QJsonObject notification;
    notification["tag"] = tag;                  // no-i18n
    notification["card"] = card;                // no-i18n
    notification["emblem-counter"] = emblem;    // no-i18n
    notification["sound"] = true;               // no-i18n // TODO make setting
    notification["vibrate"] = true;             // no-i18n // TODO make setting

    QJsonObject postalMessage = QJsonObject();
    postalMessage["notification"] = notification; // no-i18n

    return postalMessage;
}
