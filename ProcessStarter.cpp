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

#include "ProcessStarter.h"

#include <QProcess>
#include <QProcessEnvironment>
#include <QDBusServiceWatcher>
#include <QDBusConnection>
#include <QDBusConnectionInterface>
#include <QDebug>

class ProcessStarter::Private {
public:
    QString id;
    QObject * target;
    QString slot;
    QProcess * process;


};

ProcessStarter::ProcessStarter(
        const QString & id, const QString & exec,
        QObject * target, const QString & slot,
        const QString & dbus)
    : d(new Private())
{
    d->id = id;
    d->target = target;
    d->slot = slot;
    d->process = NULL;

    if (exec.isEmpty()) {
        qDebug() << "Nothing to exec - meta-module";
        QMetaObject::invokeMethod(this, "processFinished", Qt::QueuedConnection);
        return;
    }

    d->process = new QProcess(this);

    d->process->setProcessEnvironment(QProcessEnvironment::systemEnvironment());

    if (dbus.isEmpty()) {
        qDebug() << "Waiting for the process to end...";
        connect(d->process, SIGNAL(finished(int, QProcess::ExitStatus)),
                this, SLOT(processFinished()));

    } else if (QDBusConnection::sessionBus().interface()->isServiceRegistered(dbus)) {
        processFinished();
        return;

    } else {
        qDebug() << "Wating for the dbus service...";
        QDBusServiceWatcher * watcher = new QDBusServiceWatcher(
                dbus, QDBusConnection::sessionBus(),
                QDBusServiceWatcher::WatchForRegistration, this);

        connect(watcher, SIGNAL(serviceRegistered(QString)),
                this, SLOT(processFinished()));
    }

    qDebug() << "exec process" << exec << "wait for" << dbus;
    d->process->start(exec);
}

ProcessStarter::~ProcessStarter()
{
    qDebug() << "Deleting ProcessStarter" << d->id;
    delete d;
}

void ProcessStarter::processFinished()
{
    qDebug() << "processFinished" << d->id;

    if (d->process)
    if (d->process->processEnvironment() != QProcessEnvironment::systemEnvironment()) {
        qDebug() << "process environment changed" <<
            d->process->processEnvironment().toStringList();

    }

    QMetaObject::invokeMethod(
            d->target, d->slot.toAscii(),
            Qt::DirectConnection,
            Q_ARG(QString, d->id)
        );
    // deleteLater();
}

