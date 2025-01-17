/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtNetwork module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qlocalserver.h"
#include "qlocalserver_p.h"
#include "qlocalsocket.h"
#include "qlocalsocket_p.h"

#include <qhostaddress.h>
#include <qsettings.h>
#include <qdebug.h>

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

void QLocalServerPrivate::init()
{
    Q_Q(QLocalServer);
    q->connect(&tcpServer, SIGNAL(newConnection()), SLOT(_q_onNewConnection()));
}

bool QLocalServerPrivate::listen(const QString &requestedServerName)
{
    tcpServer.setListenBacklogSize(listenBacklog);

    if (!tcpServer.listen(QHostAddress::LocalHost))
        return false;

    const auto prefix = "QLocalServer/"_L1;
    if (requestedServerName.startsWith(prefix))
        fullServerName = requestedServerName;
    else
        fullServerName = prefix + requestedServerName;

    QSettings settings("QtProject"_L1, "Qt"_L1);
    if (settings.contains(fullServerName)) {
        qWarning("QLocalServer::listen: server name is already in use.");
        tcpServer.close();
        return false;
    }

    settings.setValue(fullServerName, tcpServer.serverPort());
    return true;
}

bool QLocalServerPrivate::listen(qintptr socketDescriptor)
{
    return tcpServer.setSocketDescriptor(socketDescriptor);
}

void QLocalServerPrivate::closeServer()
{
    QSettings settings("QtProject"_L1, "Qt"_L1);
    if (fullServerName == "QLocalServer"_L1)
        settings.setValue(fullServerName, QVariant());
    else
        settings.remove(fullServerName);
    tcpServer.close();
}

void QLocalServerPrivate::waitForNewConnection(int msec, bool *timedOut)
{
    if (pendingConnections.isEmpty())
        tcpServer.waitForNewConnection(msec, timedOut);
    else if (timedOut)
        *timedOut = false;
}

void QLocalServerPrivate::_q_onNewConnection()
{
    Q_Q(QLocalServer);
    QTcpSocket* tcpSocket = tcpServer.nextPendingConnection();
    if (!tcpSocket) {
        qWarning("QLocalServer: no pending connection");
        return;
    }

    tcpSocket->setParent(q);
    const quintptr socketDescriptor = tcpSocket->socketDescriptor();
    q->incomingConnection(socketDescriptor);
}

bool QLocalServerPrivate::removeServer(const QString &name)
{
    const auto prefix = "QLocalServer/"_L1;
    QString serverName;
    if (name.startsWith(prefix))
        serverName = name;
    else
        serverName = prefix + name;

    QSettings settings("QtProject"_L1, "Qt"_L1);
    if (settings.contains(serverName))
        settings.remove(serverName);

    return true;
}

QT_END_NAMESPACE
