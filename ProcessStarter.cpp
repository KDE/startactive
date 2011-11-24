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
        qDebug() << "ProcessStarter:\t" << d->id << "nothing to exec - meta-module";
        QMetaObject::invokeMethod(this, "processFinished", Qt::QueuedConnection);
        return;
    }

    d->process = new QProcess(this);

    d->process->setProcessEnvironment(QProcessEnvironment::systemEnvironment());
    d->process->setProcessChannelMode(QProcess::ForwardedChannels);
    // d->process->closeReadChannel(QProcess::StandardOutput);
    // d->process->closeReadChannel(QProcess::StandardError);

    if (dbus.isEmpty()) {
        qDebug() << "ProcessStarter:\t" << d->id << "will wait for the process to finish.";
        connect(d->process, SIGNAL(finished(int, QProcess::ExitStatus)),
                this, SLOT(processFinished()));

    } else if (QDBusConnection::sessionBus().interface()->isServiceRegistered(dbus)) {
        processFinished();
        return;

    } else {
        qDebug() << "ProcessStarter:\t" << d->id << "will wait for the dbus service" << dbus;
        QDBusServiceWatcher * watcher = new QDBusServiceWatcher(
                dbus, QDBusConnection::sessionBus(),
                QDBusServiceWatcher::WatchForRegistration, this);

        connect(watcher, SIGNAL(serviceRegistered(QString)),
                this, SLOT(processFinished()));
    }

    qDebug() << "ProcessStarter:\t" << d->id << "exec process" << exec << "wait for" << dbus;
    d->process->start(exec);
}

ProcessStarter::~ProcessStarter()
{
    delete d;
}

void ProcessStarter::processFinished()
{
    qDebug() << "ProcessStarter:\t" << d->id << "process finished";

    if (d->process)
    if (d->process->processEnvironment() != QProcessEnvironment::systemEnvironment()) {
        qDebug() << "ProcessStarter:\t" << d->id << "process environment changed" <<
            d->process->processEnvironment().toStringList();

    }

    QMetaObject::invokeMethod(
            d->target, d->slot.toAscii(),
            Qt::DirectConnection,
            Q_ARG(QString, d->id)
        );
    // deleteLater();
}

