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

#include "qnetworkproxy.h"

#include <QtCore/QByteArray>
#include <QtCore/QUrl>

#ifndef QT_NO_NETWORKPROXY

/*
 * Construct a proxy from the environment variables http_proxy and no_proxy.
 * Or no system proxy. Just return a list with NoProxy.
 */

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

static bool ignoreProxyFor(const QNetworkProxyQuery &query)
{
    const QByteArray noProxy = qgetenv("no_proxy").trimmed();
    if (noProxy.isEmpty())
        return false;

    const QString host = query.peerHostName();

    const QList<QByteArray> noProxyTokens = noProxy.split(',');

    for (const QByteArray &rawToken : noProxyTokens) {
        auto token = QLatin1StringView(rawToken).trimmed();

        // Since we use suffix matching, "*" is our 'default' behaviour
        if (token.startsWith(u'*'))
            token = token.mid(1);

        // Harmonize trailing dot notation
        if (token.endsWith(u'.') && !host.endsWith(u'.'))
            token = token.chopped(1);

        if (token.startsWith(u'.')) // leading dot is implied
            token = token.mid(1);

        if (host.endsWith(token)) {

            // Make sure that when we have a suffix match,
            // we don't match "donotmatch.com" with "match.com"

            if (host.size() == token.size())                  // iow: host == token
                return true;
            if (host[host.size() - token.size() - 1] == u'.') // match follows a dot
                return true;
        }
    }

    return false;
}

QList<QNetworkProxy> QNetworkProxyFactory::systemProxyForQuery(const QNetworkProxyQuery &query)
{
    QList<QNetworkProxy> proxyList;

    if (ignoreProxyFor(query))
        return proxyList << QNetworkProxy::NoProxy;

    // No need to care about casing here, QUrl lowercases values already
    const QString queryProtocol = query.protocolTag();
    QByteArray proxy_env;

    if (queryProtocol == "http"_L1)
        proxy_env = qgetenv("http_proxy");
    else if (queryProtocol == "https"_L1)
        proxy_env = qgetenv("https_proxy");
    else if (queryProtocol == "ftp"_L1)
        proxy_env = qgetenv("ftp_proxy");
    else
        proxy_env = qgetenv("all_proxy");

    // Fallback to http_proxy is no protocol specific proxy was found
    if (proxy_env.isEmpty())
        proxy_env = qgetenv("http_proxy");

    if (!proxy_env.isEmpty()) {
        QUrl url = QUrl(QString::fromLocal8Bit(proxy_env));
        const QString scheme = url.scheme();
        if (scheme == "socks5"_L1) {
            QNetworkProxy proxy(QNetworkProxy::Socks5Proxy, url.host(),
                    url.port() ? url.port() : 1080, url.userName(), url.password());
            proxyList << proxy;
        } else if (scheme == "socks5h"_L1) {
            QNetworkProxy proxy(QNetworkProxy::Socks5Proxy, url.host(),
                    url.port() ? url.port() : 1080, url.userName(), url.password());
            proxy.setCapabilities(QNetworkProxy::HostNameLookupCapability);
            proxyList << proxy;
        } else if ((scheme.isEmpty() || scheme == "http"_L1)
                  && query.queryType() != QNetworkProxyQuery::UdpSocket
                  && query.queryType() != QNetworkProxyQuery::TcpServer) {
            QNetworkProxy proxy(QNetworkProxy::HttpProxy, url.host(),
                    url.port() ? url.port() : 8080, url.userName(), url.password());
            proxyList << proxy;
        }
    }
    if (proxyList.isEmpty())
        proxyList << QNetworkProxy::NoProxy;

    return proxyList;
}

QT_END_NAMESPACE

#endif
