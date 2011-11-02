/*
 *   Copyright (C) 2011 Ivan Cukic <ivan.cukic(at)kde.org>
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

#ifndef STARTACTIVE_H_
#define STARTACTIVE_H_

#include <QApplication>
#include <QDBusConnection>

// #include <X11/Xlib.h>

class StartActive: public QApplication {
    Q_OBJECT

public:
    StartActive(/*Display * display,*/ int argc, char ** argv);
    virtual ~StartActive();

public Q_SLOTS:
    /**
     * Loads the specified modules
     * @param modules coma-separated list of modules
     */
    void load(const QString & modules);

    /**
     * Stops all modules and quits
     */
    void quit();

    void moduleStarted(const QString & module);

private:
    class Private;
    Private * const d;
};

#endif // STARTACTIVE_H_

