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

#include "SignalListener.h"

#include <signal.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>

#include <QSocketNotifier>
#include <QDebug>

namespace Private {
    int fd[2];
    void signalArrived(int signal)
    {
        write(fd[0], &signal, sizeof(signal));
    }
} // namespace Private

SignalListener &SignalListener::self()
{
    static SignalListener instance;
    return instance;
}

SignalListener::SignalListener()
{
    if (socketpair(AF_UNIX, SOCK_STREAM, 0, Private::fd) ||
        socketpair(AF_UNIX, SOCK_STREAM, 0, Private::fd)
    ) {
        qDebug() << "Couldn't create HUP socketpair";
        return;
    }

    auto sg = new QSocketNotifier(Private::fd[1], QSocketNotifier::Read, this);
    connect(sg, SIGNAL(activated(int)), this, SLOT(handleSignal()));
}

int SignalListener::registerSignal(int signal)
{
    struct sigaction action;

    action.sa_handler = Private::signalArrived;
    sigemptyset(&action.sa_mask);
    action.sa_flags = 0;
    action.sa_flags |= SA_RESTART;

    return (sigaction(SIGHUP, &action, 0) > 0);
}

void SignalListener::handleSignal()
{
    int signal;
    ::read(Private::fd[1], &signal, sizeof(signal));

    emit signalReceived(signal);
}

