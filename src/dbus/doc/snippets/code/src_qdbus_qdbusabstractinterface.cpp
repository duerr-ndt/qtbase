/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the documentation of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** BSD License Usage
** Alternatively, you may use this file under the terms of the BSD license
** as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of The Qt Company Ltd nor the names of its
**     contributors may be used to endorse or promote products derived
**     from this software without specific prior written permission.
**
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <QString>
#include <QDBusMessage>
#include <QDBusReply>
#include <QDBusInterface>

using namespace Qt::StringLiterals;

class Abstract_DBus_Interface : public QObject
{
    Q_OBJECT

public:
    Abstract_DBus_Interface(QObject *parent = nullptr)
    : QObject(parent) {
        interface = new QDBusInterface("org.example.Interface", "/Example/Methods");
    }

    ~Abstract_DBus_Interface() {  delete interface; }
    void interfaceMain();
    void asyncCall();
    QString retrieveValue() { return QString(); }

public slots:
    void callFinishedSlot();

private:
    QDBusInterface *interface;
};

void Abstract_DBus_Interface::interfaceMain()
{
//! [0]
QString value = retrieveValue();
QDBusMessage reply;

QDBusReply<int> api = interface->call("GetAPIVersion"_L1);
if (api >= 14)
  reply = interface->call("ProcessWorkUnicode"_L1, value);
else
  reply = interface->call("ProcessWork"_L1, "UTF-8"_L1, value.toUtf8());
//! [0]
}

void Abstract_DBus_Interface::asyncCall()
{
//! [1]
QString value = retrieveValue();
QDBusPendingCall pcall = interface->asyncCall("Process"_L1, value);

QDBusPendingCallWatcher *watcher = new QDBusPendingCallWatcher(pcall);

QObject::connect(watcher, SIGNAL(finished(QDBusPendingCallWatcher*)),
                this, SLOT(callFinishedSlot(QDBusPendingCallWatcher*)));
//! [1]
}
