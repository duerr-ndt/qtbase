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

#include "qnetworkaccessdebugpipebackend_p.h"
#include "QtCore/qdatastream.h"
#include <QCoreApplication>
#include <QStringList>
#include <QUrlQuery>
#include "private/qnoncontiguousbytedevice_p.h"

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

#ifdef QT_BUILD_INTERNAL

enum {
    ReadBufferSize = 16384,
    WriteBufferSize = ReadBufferSize
};

QStringList QNetworkAccessDebugPipeBackendFactory::supportedSchemes() const
{
    return QStringList(QStringLiteral("debugpipe"));
}

QNetworkAccessBackend *
QNetworkAccessDebugPipeBackendFactory::create(QNetworkAccessManager::Operation op,
                                              const QNetworkRequest &request) const
{
    // is it an operation we know of?
    switch (op) {
    case QNetworkAccessManager::GetOperation:
    case QNetworkAccessManager::PutOperation:
        break;

    default:
        // no, we can't handle this operation
        return nullptr;
    }

    QUrl url = request.url();
    if (url.scheme() == "debugpipe"_L1)
        return new QNetworkAccessDebugPipeBackend;
    return nullptr;
}

QNetworkAccessDebugPipeBackend::QNetworkAccessDebugPipeBackend()
    : QNetworkAccessBackend(QNetworkAccessBackend::TargetType::Networked),
      bareProtocol(false),
      hasUploadFinished(false),
      hasDownloadFinished(false),
      hasEverythingFinished(false),
      bytesDownloaded(0),
      bytesUploaded(0)
{
}

QNetworkAccessDebugPipeBackend::~QNetworkAccessDebugPipeBackend()
{
    // this is signals disconnect, not network!
    socket.disconnect(this);    // we're not interested in the signals at this point
}

void QNetworkAccessDebugPipeBackend::open()
{
    socket.connectToHost(url().host(), url().port(12345));
    socket.setReadBufferSize(ReadBufferSize);

    // socket ready read -> we can push from socket to downstream
    connect(&socket, SIGNAL(readyRead()), SLOT(socketReadyRead()));
    connect(&socket, SIGNAL(errorOccurred(QAbstractSocket::SocketError)), SLOT(socketError()));
    connect(&socket, SIGNAL(disconnected()), SLOT(socketDisconnected()));
    connect(&socket, SIGNAL(connected()), SLOT(socketConnected()));
    // socket bytes written -> we can push more from upstream to socket
    connect(&socket, SIGNAL(bytesWritten(qint64)), SLOT(socketBytesWritten(qint64)));

    bareProtocol = QUrlQuery(url()).queryItemValue("bare"_L1) == "1"_L1;

    if (operation() == QNetworkAccessManager::PutOperation) {
        createUploadByteDevice();
        QObject::connect(uploadByteDevice(), SIGNAL(readyRead()), this,
                         SLOT(uploadReadyReadSlot()));
        QMetaObject::invokeMethod(this, "uploadReadyReadSlot", Qt::QueuedConnection);
    }
}

void QNetworkAccessDebugPipeBackend::socketReadyRead()
{
    readyRead();
}

qint64 QNetworkAccessDebugPipeBackend::read(char *data, qint64 maxlen)
{
    qint64 haveRead = socket.read(data, maxlen);

    if (haveRead == -1) {
        hasDownloadFinished = true;
        // this ensures a good last downloadProgress is emitted
        setHeader(QNetworkRequest::ContentLengthHeader, QVariant());
        possiblyFinish();
        return haveRead;
    }

    bytesDownloaded += haveRead;
    return haveRead;
}

qint64 QNetworkAccessDebugPipeBackend::bytesAvailable() const
{
    return socket.bytesAvailable();
}

void QNetworkAccessDebugPipeBackend::socketBytesWritten(qint64)
{
    pushFromUpstreamToSocket();
}

void QNetworkAccessDebugPipeBackend::uploadReadyReadSlot()
{
    pushFromUpstreamToSocket();
}

void QNetworkAccessDebugPipeBackend::pushFromUpstreamToSocket()
{
    // FIXME
    if (operation() == QNetworkAccessManager::PutOperation) {
        if (hasUploadFinished)
            return;

        forever {
            if (socket.bytesToWrite() >= WriteBufferSize)
                return;

            QByteArray data(WriteBufferSize, Qt::Uninitialized);
            qint64 haveRead = uploadByteDevice()->peek(data.data(), data.size());
            if (haveRead == -1) {
                // EOF
                hasUploadFinished = true;
                possiblyFinish();
                break;
            } else if (haveRead == 0) {
                // nothing to read right now, we will be called again later
                break;
            } else {
                qint64 haveWritten;
                data.truncate(haveRead);
                haveWritten = socket.write(std::move(data));

                if (haveWritten < 0) {
                    // write error!
                    QString msg = QCoreApplication::translate("QNetworkAccessDebugPipeBackend", "Write error writing to %1: %2")
                                  .arg(url().toString(), socket.errorString());
                    error(QNetworkReply::ProtocolFailure, msg);
                    finished();
                    return;
                } else {
                    uploadByteDevice()->skip(haveWritten);
                    bytesUploaded += haveWritten;
                }

                //QCoreApplication::processEvents();
            }
        }
    }
}

void QNetworkAccessDebugPipeBackend::possiblyFinish()
{
    if (hasEverythingFinished)
        return;
    hasEverythingFinished = true;

    if ((operation() == QNetworkAccessManager::GetOperation) && hasDownloadFinished) {
        socket.close();
        finished();
    } else if ((operation() == QNetworkAccessManager::PutOperation) && hasUploadFinished) {
        socket.close();
        finished();
    }


}

void QNetworkAccessDebugPipeBackend::close()
{
    qWarning("QNetworkAccessDebugPipeBackend::closeDownstreamChannel() %d",operation());;
    //if (operation() == QNetworkAccessManager::GetOperation)
    //    socket.disconnectFromHost();
}


void QNetworkAccessDebugPipeBackend::socketError()
{
    qWarning("QNetworkAccessDebugPipeBackend::socketError() %d",socket.error());
    QNetworkReply::NetworkError code;
    switch (socket.error()) {
    case QAbstractSocket::RemoteHostClosedError:
        return;                 // socketDisconnected will be called

    case QAbstractSocket::NetworkError:
        code = QNetworkReply::UnknownNetworkError;
        break;

    default:
        code = QNetworkReply::ProtocolFailure;
        break;
    }

    error(code, QNetworkAccessDebugPipeBackend::tr("Socket error on %1: %2")
          .arg(url().toString(), socket.errorString()));
    finished();
    disconnect(&socket, SIGNAL(disconnected()), this, SLOT(socketDisconnected()));

}

void QNetworkAccessDebugPipeBackend::socketDisconnected()
{
    if (socket.bytesToWrite() == 0) {
        // normal close
    } else {
        readyRead(); // @todo this is odd
        // abnormal close
        QString msg = QNetworkAccessDebugPipeBackend::tr("Remote host closed the connection prematurely on %1")
                             .arg(url().toString());
        error(QNetworkReply::RemoteHostClosedError, msg);
        finished();
    }
}

void QNetworkAccessDebugPipeBackend::socketConnected()
{
}


#endif

QT_END_NAMESPACE

#include "moc_qnetworkaccessdebugpipebackend_p.cpp"
