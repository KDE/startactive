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

#ifndef PROCESSSTARTER_H_
#define PROCESSSTARTER_H_

#include <QObject>

/**
 * ProcessStarter
 */
class ProcessStarter: public QObject {
    Q_OBJECT

public:
    ProcessStarter(
            const QString & id,
            const QString & exec,
            QObject * target,
            const QString & slot,
            const QString & dbus = QString()
            );
    virtual ~ProcessStarter();

private Q_SLOTS:
    void processFinished();

private:
    class Private;
    Private * const d;
};


#endif // PROCESSSTARTER_H_

