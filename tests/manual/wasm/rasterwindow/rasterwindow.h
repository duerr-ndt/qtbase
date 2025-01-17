/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the examples of the Qt Toolkit.
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
#ifndef RASTERWINDOW_H
#define RASTERWINDOW_H

#include <QtGui>

class RasterWindow : public QRasterWindow
{
public:
    RasterWindow();

    virtual void paintEvent(QPaintEvent * event);

    virtual void exposeEvent(QExposeEvent * ev);
    virtual void focusInEvent(QFocusEvent * ev);
    virtual void focusOutEvent(QFocusEvent * ev);
    virtual void hideEvent(QHideEvent * ev);
    virtual void keyPressEvent(QKeyEvent * ev);
    virtual void keyReleaseEvent(QKeyEvent * ev);
    virtual void mouseDoubleClickEvent(QMouseEvent * ev);
    virtual void mouseMoveEvent(QMouseEvent * ev);
    virtual void mousePressEvent(QMouseEvent * ev);
    virtual void mouseReleaseEvent(QMouseEvent * ev);
    virtual void moveEvent(QMoveEvent * ev);
//    virtual bool nativeEvent(const QByteArray & eventType, void * message, long * result);
    virtual void resizeEvent(QResizeEvent * ev);
    virtual void showEvent(QShowEvent * ev);
    virtual void tabletEvent(QTabletEvent * ev);
    virtual void touchEvent(QTouchEvent * ev);
    virtual void wheelEvent(QWheelEvent * ev);
private:
    void incrementEventCount();
    int m_eventCount;
    int m_timeoutCount;
    int m_frameCount;
    int m_fps;
    QPoint m_offset;
    QPoint m_lastPos;
    bool m_pressed;
};

#endif // RASTERWINDOW_H
