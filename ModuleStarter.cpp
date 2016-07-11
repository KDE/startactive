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

#include "ModuleStarter.h"

#include <QProcess>
#include <QProcessEnvironment>
#include <QDBusServiceWatcher>
#include <QDBusConnection>
#include <QDBusConnectionInterface>
#include <QDebug>

ModuleStarter::ModuleStarter(const QString &id, const QString &exec,
                             QObject *target, const QString &slot,
                             const QString &dbus)
    : m_id(id)
    , m_dbus(dbus)
    , m_target(target)
    , m_slot(slot)
    , m_process(nullptr)
{
    if (exec.isEmpty()) {
        // Should we wait for some d-bus service, or do we send the
        // finished signal now?
        if (dbus.isEmpty()) {
            qDebug() << "ModuleStarter:\t" << m_id << "nothing to exec - meta-module";
            notifyListener();

        } else {
            listenForDBusRegistered();

        }

    } else {
        m_process = new QProcess(this);

        m_process->setProcessEnvironment(QProcessEnvironment::systemEnvironment());
        m_process->setProcessChannelMode(QProcess::ForwardedChannels);
        // m_process->closeReadChannel(QProcess::StandardOutput);
        // m_process->closeReadChannel(QProcess::StandardError);

        if (dbus.isEmpty()) {
            // If there is no dbus service to wait for, wait for the process
            // to be finished
            listenForProcessFinished();

        } else if (QDBusConnection::sessionBus().interface()->isServiceRegistered(dbus)) {
            // If the service is already present, no need to start this module
            notifyListener();
            return;

        } else {
            // If the service is not present, we are going to wait
            // until it starts
            listenForDBusRegistered();
        }

        connect(m_process, &QProcess::errorOccurred,
                this, [this] (QProcess::ProcessError error) {
                    qDebug() << "Error starting" << m_id << " " << error;
                    qDebug() << "Environment was:";
                    for (const auto& item: QProcessEnvironment::systemEnvironment().toStringList()) {
                        qDebug() << item;
                    }
                });

        qDebug() << "ModuleStarter:\t" << m_id << "exec process" << exec << "wait for" << dbus;
        m_process->start(exec);

    }
}

void ModuleStarter::listenForProcessFinished()
{
    qDebug() << "ModuleStarter:\t" << m_id << "will wait for the process to finish.";
    connect(m_process, (void (QProcess::*)(int,QProcess::ExitStatus))&QProcess::finished,
            this,      &ModuleStarter::processFinished);
}

void ModuleStarter::listenForDBusRegistered()
{
    qDebug() << "ModuleStarter:\t" << m_id << "will wait for the dbus service" << m_dbus;
    auto watcher = new QDBusServiceWatcher(
            m_dbus, QDBusConnection::sessionBus(),
            QDBusServiceWatcher::WatchForRegistration, this);

    connect(watcher, &QDBusServiceWatcher::serviceRegistered,
            this,    &ModuleStarter::dbusRegistered);
}

void ModuleStarter::processFinished(int exitCode,
                                    QProcess::ExitStatus exitStatus)
{
    // We don't have anything smart to do with these :/
    Q_UNUSED(exitCode);
    Q_UNUSED(exitStatus);

    notifyListener();
}

void ModuleStarter::dbusRegistered(const QString &service)
{
    Q_UNUSED(service);

    notifyListener();
}

void ModuleStarter::notifyListener()
{
    QMetaObject::invokeMethod(this, "notifyListenerImpl", Qt::QueuedConnection);
}

void ModuleStarter::notifyListenerImpl()
{
    qDebug() << "ModuleStarter:\t" << m_id << "process finished";

    if (m_process
        && m_process->processEnvironment()
               != QProcessEnvironment::systemEnvironment()) {
        qDebug() << "ModuleStarter:\t" << m_id << "process environment changed"
                 << m_process->processEnvironment().toStringList();
    }

    QMetaObject::invokeMethod(
            m_target, m_slot.toLatin1(),
            Qt::DirectConnection,
            Q_ARG(QString, m_id)
        );
    // deleteLater();
}

