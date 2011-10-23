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

#include "StartActive.h"
#include "startactiveadaptor.h"

#include <signal.h>

#include <QSettings>

#include "SignalListener.h"
#include "config-startactive.h"

/**
 *
 */
class StartActive::Private {
public:
    enum WaitMode {
        DontWait = 0,
        WaitForDbus = 1,
        WaitForEnd = 2
    };

    class Module {
    public:
        QString exec;
        WaitMode wait;
        QString dbus;

    };

    QStringList scheduled;
    QSet < QString > running;

    QHash < QString, Module * > modules;
    QHash < QString, QSet < QString > > requires;
    QHash < int, QSet < QString > > runOrder;

    QString modulePattern;

    void readModuleData(const QString & module);
    void startFreeModules();
    void startModule(const QString & module);

};

StartActive::StartActive(int argc, char ** argv)
    : QCoreApplication(argc, argv),
      d(new Private())
{
    QDBusConnection dbus = QDBusConnection::sessionBus();

    new StartActiveAdaptor(this);

    // Proper object name
    dbus.registerObject("/StartActive", this);

    // For the compatibility with kquitapp
    dbus.registerObject("/MainApplication", this);

    // We have to deinitialize before quitting. So, we need to process
    // SIGHUP our way
    SignalListener::self()->registerSignal(SIGHUP);
    connect(SignalListener::self(), SIGNAL(signalReceived(int)),
            this, SLOT(quit()));

    d->modulePattern = QString(MODULE_PATH) + "/%1.module";

    int id = arguments().indexOf("--modules");
    if (id != -1 && id < arguments().size() - 1) {
        load(arguments()[id + 1]);
    }
}

StartActive::~StartActive()
{
    delete d;
}

void StartActive::quit()
{
    qDebug() << "The system wants us to quit.";

    // Ignoring at the moment

}

void StartActive::load(const QString & modules)
{
    qDebug() << "Loading modules: " << modules;

    d->scheduled = modules.split(",");

    while (!d->scheduled.empty()) {
        d->readModuleData(d->scheduled.first());
    }

    // debugging
    foreach(int level, d->runOrder.keys()) {
        qDebug() << level << " - " << d->runOrder[level];
    }

    qDebug() << "STARTING";

    d->startFreeModules();
}

void StartActive::Private::readModuleData(const QString & module)
{
    if (module.isEmpty() || modules.contains(module) || running.contains(module)) return;

    QSettings config(modulePattern.arg(module), QSettings::IniFormat);

    scheduled.removeAll(module);

    Module * data = new Module();
    data->exec = config.value("Module/exec", QString()).toString();

    const QString & waitFor = config.value("Module/wait").toString();
    if (waitFor == "dbus") {
        data->wait = WaitForDbus;
        data->dbus = config.value("Module/dbus", QString()).toString();

    } else if (waitFor == "end") {
        data->wait = WaitForEnd;

    } else {
        data->wait = DontWait;

    }

    // qDebug() << data->exec << data->wait << data->dbus
    //     << config.contains("Module/id")
    //     << config.value("Module/id")
    //     << config.value("Module/exec")
    //     << config.value("Module/depends")
    //     << config.value("Module/id")
    //     ;

    modules[module] = data;

    const QSet < QString > & dependsOn =
        config.value("Module/depends", QStringList()).toStringList().toSet();
    runOrder[dependsOn.count()] << module;
    requires[module] += dependsOn;

    foreach (const QString & dep, dependsOn) {
        if (!running.contains(dep)
                && !scheduled.contains(dep)
                && !modules.contains(dep)
        ) {
            qDebug() << "adding" << dep;
            scheduled << dep;
        }
    }
    qDebug() << scheduled;
}

void StartActive::Private::startFreeModules()
{
    QSet < QString > starting = runOrder[0];
    runOrder[0].clear();

    foreach (const QString & module, starting) {
        startModule(module);
    }

}

void StartActive::Private::startModule(const QString & module)
{
    qDebug() << "Starting " << module;

}
