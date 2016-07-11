/*
 *   Copyright (C) 2011-2016 Ivan Cukic <ivan.cukic(at)kde.org>
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

#ifndef MODULE_STARTER_H_
#define MODULE_STARTER_H_

#include <QObject>
#include <QProcess>

/**
 * ModuleStarter
 */
class ModuleStarter: public QObject {
    Q_OBJECT

public:
    ModuleStarter(const QString &id, const QString &exec, QObject *target,
                   const QString &slot, const QString &dbus = QString());

    void listenForProcessFinished();
    void listenForDBusRegistered();

private Q_SLOTS:
    void processFinished(int exitCode, QProcess::ExitStatus exitStatus);
    void dbusRegistered(const QString &service);

    void notifyListenerImpl();

private:
    void notifyListener();

    QString   m_id;
    QString   m_dbus;
    QObject  *m_target;
    QString   m_slot;
    QProcess *m_process;
};


#endif // include guard

