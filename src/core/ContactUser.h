/* Ricochet - https://ricochet.im/
 * Copyright (C) 2014, John Brooks <john.brooks@dereferenced.net>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *    * Redistributions of source code must retain the above copyright
 *      notice, this list of conditions and the following disclaimer.
 *
 *    * Redistributions in binary form must reproduce the above
 *      copyright notice, this list of conditions and the following disclaimer
 *      in the documentation and/or other materials provided with the
 *      distribution.
 *
 *    * Neither the names of the copyright owners nor the names of its
 *      contributors may be used to endorse or promote products derived from
 *      this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef CONTACTUSER_H
#define CONTACTUSER_H

#include <QObject>
#include <QHash>
#include <QMetaType>
#include <QVariant>
#include "utils/Settings.h"

class UserIdentity;
class OutgoingContactRequest;
class ConversationModel;

#ifdef PROTOCOL_NEW
namespace Protocol
{
    class OutboundConnector;
    class Connection;
}
#else
#include "protocol/ProtocolSocket.h"
class OutgoingContactSocket;
struct ChatMessageData;
class ChatMessageCommand;
#endif

/* Represents a user on the contact list.
 * All persistent uses of a ContactUser instance must either connect to the
 * contactDeleted() signal, or use a QWeakPointer to track deletion. A ContactUser
 * can be removed at essentially any time. */

class ContactUser : public QObject
{
    Q_OBJECT
    Q_DISABLE_COPY(ContactUser)
    Q_ENUMS(Status)

    Q_PROPERTY(int uniqueID READ getUniqueID CONSTANT)
    Q_PROPERTY(UserIdentity* identity READ getIdentity CONSTANT)
    Q_PROPERTY(QString nickname READ nickname WRITE setNickname NOTIFY nicknameChanged)
    Q_PROPERTY(QString contactID READ contactID CONSTANT)
    Q_PROPERTY(Status status READ status NOTIFY statusChanged)
    Q_PROPERTY(OutgoingContactRequest *contactRequest READ contactRequest NOTIFY statusChanged)
    Q_PROPERTY(SettingsObject *settings READ settings CONSTANT)
    Q_PROPERTY(ConversationModel *conversation READ conversation CONSTANT)

    friend class ContactsManager;
    friend class OutgoingContactRequest;
#ifndef PROTOCOL_NEW
    friend class ChatMessageCommand;
#endif

public:
    enum Status
    {
        Online,
        Offline,
        RequestPending,
        RequestRejected
    };

    UserIdentity * const identity;
    const int uniqueID;

    explicit ContactUser(UserIdentity *identity, int uniqueID, QObject *parent = 0);

#ifdef PROTOCOL_NEW
    Protocol::Connection *connection() { return m_connection; }
#else
    ProtocolSocket *conn() const { return m_conn; }
#endif
    bool isConnected() const { return status() == Online; }

    OutgoingContactRequest *contactRequest() { return m_contactRequest; }
    ConversationModel *conversation() { return m_conversation; }

    UserIdentity *getIdentity() const { return identity; }
    int getUniqueID() const { return uniqueID; }

    QString nickname() const;
    /* Hostname is in the onion hostname format, i.e. it ends with .onion */
    QString hostname() const;
    quint16 port() const;
    /* Contact ID in the ricochet: format */
    QString contactID() const;

    Status status() const { return m_status; }

    SettingsObject *settings();

    Q_INVOKABLE void deleteContact();

public slots:
#ifdef PROTOCOL_NEW
    /* Assign a connection to this user
     *
     * The connection must be connected, and the peer must be authenticated and
     * must match this user. ContactUser will assume ownership of the connection,
     * and it will be closed and deleted when it's no longer used.
     *
     * It is valid to pass an incoming or outgoing connection. If there is already
     * a connection, protocol-specific rules are applied and the new connection
     * may be closed to favor the older one.
     *
     * If the existing connection is replaced, that is equivalent to disconnecting
     * and reconnectng immediately - any ongoing operations will fail and need to
     * be retried at a higher level.
     */
    void assignConnection(Protocol::Connection *connection);
#else
    void incomingProtocolSocket(QTcpSocket *socket);
#endif

    void setNickname(const QString &nickname);
    void setHostname(const QString &hostname);

    void updateStatus();

signals:
    void statusChanged();
    void connected();
    void disconnected();

    void nicknameChanged();
    void contactDeleted(ContactUser *user);

#ifndef PROTOCOL_NEW
    /* Hack to allow creating models/windows/etc to handle other signals before they're
     * emitted; primarily, to allow UI to create models to handle incomingChatMessage */
    void prepareInteractiveHandler();
    void incomingChatMessage(const ChatMessageData &message);
#endif

private slots:
    void onConnected();
    void onDisconnected();
    void requestRemoved();
    void onSettingsModified(const QString &key, const QJsonValue &value);

private:
#ifdef PROTOCOL_NEW
    // XXX be paranoid about tracking deletion of m_connection
    Protocol::Connection *m_connection;
    Protocol::OutboundConnector *m_outgoingSocket;
#else
    ProtocolSocket *m_conn;
    OutgoingContactSocket *m_outgoingSocket;
#endif

    Status m_status;
    quint16 m_lastReceivedChatID;
    OutgoingContactRequest *m_contactRequest;
    SettingsObject *m_settings;
    ConversationModel *m_conversation;

    /* See ContactsManager::addContact */
    static ContactUser *addNewContact(UserIdentity *identity, int id);

    void loadContactRequest();
    void updateOutgoingSocket();

#ifdef PROTOCOL_NEW
    void clearConnection();
#endif
};

Q_DECLARE_METATYPE(ContactUser*)

#endif // CONTACTUSER_H
