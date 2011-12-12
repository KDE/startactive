/*
 *   Copyright (C) 2010 Ivan Cukic <ivan.cukic(at)kde.org>
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License version 2,
 *   or (at your option) any later version, as published by the Free
 *   Software Foundation
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details
 *
 *   You should have received a copy of the GNU General Public
 *   License along with this program; if not, write to the
 *   Free Software Foundation, Inc.,
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include "SplashWindow.h"

#include <QApplication>
#include <QDeclarativeContext>
#include <QDesktopWidget>
#include <QGraphicsObject>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QTimer>
#include <QDebug>

// #include <X11/Xlib.h>

#include "SystemInfo.h"

SplashWindow * SplashWindow::s_instance = NULL;

void SplashWindow::init()
{
    if (!s_instance) {
        // Display * display = XOpenDisplay(NULL);

        // s_instance = new SplashWindow();
        // s_instance->setStage(1);

        // int sw = WidthOfScreen(ScreenOfDisplay(display, DefaultScreen(display)));
        // int sh = HeightOfScreen(ScreenOfDisplay(display, DefaultScreen(display)));

        // qDebug() << "SPLASH: size " << sw << sh;

        // s_instance->setGeometry(0, 0, sw, sh);
        // s_instance->show();

        // XSelectInput(display, DefaultRootWindow(display), SubstructureNotifyMask);

        s_instance = new SplashWindow();
        s_instance->setStage(1);
        s_instance->setGeometry(QApplication::desktop()->screenGeometry());
        s_instance->setAttribute(Qt::WA_QuitOnClose, false);
        s_instance->setAttribute(Qt::WA_QuitOnClose, false);

    }
}

void SplashWindow::close()
{
    if (s_instance) s_instance->QDeclarativeView::close();
}

SplashWindow::SplashWindow()
    : QDeclarativeView(),
      m_stage(0)
{
    setWindowFlags(
            Qt::FramelessWindowHint |
            Qt::WindowStaysOnTopHint
        );

    setWindowFlags(Qt::X11BypassWindowManagerHint);

    rootContext()->setContextProperty("screenSize", size());

    int id = QApplication::arguments().indexOf("--splash");
    QString theme = "ActiveAir";

    if (id != -1 && id < QApplication::arguments().size() - 1) {
        theme = QApplication::arguments()[id + 1];
    }
    setSource(QUrl(themeDir(theme) + "/main.qml"));

    setStyleSheet("background: #000000; border: none");
    //be sure it will be eventually closed
    //FIXME: should never be stuck
    QTimer::singleShot(30000, this, SLOT(close()));
}

void SplashWindow::setStage(int stage)
{
    if (s_instance) {
        s_instance->m_stage = stage;

        s_instance->rootObject()->setProperty("stage", stage);

        if (stage < STAGE_COUNT) {
            s_instance->show();
        }
    }
}

void SplashWindow::resizeEvent(QResizeEvent *event)
{
    Q_UNUSED(event)
    rootContext()->setContextProperty("screenSize", size());
    centerOn(rootObject());
}

void SplashWindow::mousePressEvent(QMouseEvent *event)
{
    QDeclarativeView::mousePressEvent(event);
    //for mobile devices is better to not hide on click
    /*if (!event->isAccepted()) {
        close();
    }*/
}

void SplashWindow::closeEvent(QCloseEvent * event)
{
    s_instance = NULL;
    deleteLater();
}
