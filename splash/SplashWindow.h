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

#ifndef SPLASH_WINDOW_H_
#define SPLASH_WINDOW_H_

#include <KQuickAddons/QuickViewSharedEngine>

class QResizeEvent;
class QMouseEvent;
class QKeyEvent;

class SplashWindow: public KQuickAddons::QuickViewSharedEngine
{
public:
    SplashWindow(bool testing = false, bool window = false);

    void setStage(int stage);
    virtual void setGeometry(const QRect &rect);

protected:
    void keyPressEvent(QKeyEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;

private:
    int m_stage;
    bool m_testing;
    bool m_window;
};

#endif // SPLASH_WINDOW_H_
