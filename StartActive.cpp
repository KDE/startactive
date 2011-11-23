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

#include <QTextStream>
#include "StartActive.h"

#include "startactiveadaptor.h"

#include <signal.h>
#include <stdlib.h>

#include <QSettings>

#include "SignalListener.h"
#include "config-startactive.h"
#include "ProcessStarter.h"

#include "splash/SplashWindow.h"

/**
 *
 */
class StartActive::Private {
public:
    Private(StartActive * parent)
        : q(parent)
    {
    }

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
    QHash < QString, QSet < QString > > depends;
    QList < QSet < QString > > runOrder;

    QString modulePattern;

    void readModuleData(const QString module);
    void startFreeModules();
    void startModule(const QString & module);
    void printLevels();

    void initEnvironment();
    void initDBus();

    StartActive * const q;

    int modulesFinished;
    int stage;
};

void StartActive::Private::printLevels()
{
    // debugging
    for (int level = 0; level < runOrder.size(); level++) {
        qDebug() << level << " - " << runOrder[level];
    }
}

StartActive::StartActive(/*Display * display,*/ int argc, char ** argv)
    : QApplication(/*display,*/ argc, argv),
      d(new Private(this))
{
    d->initEnvironment();
    d->initDBus();

    // We have to deinitialize before quitting. So, we need to process
    // SIGHUP our way
    SignalListener::self()->registerSignal(SIGHUP);
    connect(SignalListener::self(), SIGNAL(signalReceived(int)),
            this, SLOT(quit()));

    d->modulePattern = QString(MODULE_PATH) + "/%1.module";

    int id = arguments().indexOf("--modules");
    if (id != -1 && id < arguments().size() - 1) {
        load(arguments()[id + 1]);
    } else {
        load("active");
    }

    SplashWindow::init();
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

void StartActive::Private::initDBus()
{
    qDebug() << "Initializing DBus";

    // Check whether dbus process is running
    if (! ::getenv("DBUS_SESSION_BUS_ADDRESS")) {
        QProcess app;
        app.start("dbus-launch");

        app.waitForFinished();

        char buf[1024];

        qint64 lineLength;

        while ((lineLength = app.readLine(buf, sizeof(buf))) != -1) {
            QString line = QString(buf).trimmed();

            int pos = line.indexOf('=');

            const QString & key = line.left(pos);
            const QString & value = line.mid(pos + 1);

            qDebug() << "DBUS" << key << "=" << value;

            ::setenv(
                    key.toAscii(),
                    value.toAscii(),
                    1
                );

        }
    } else {
        qDebug() << "DBus already running at:" << ::getenv("DBUS_SESSION_BUS_ADDRESS");
    }

    QDBusConnection dbus = QDBusConnection::sessionBus();

    new StartActiveAdaptor(q);

    // Proper object name
    dbus.registerObject("/StartActive", q);

    // For the compatibility with kquitapp
    dbus.registerObject("/MainApplication", q);
}

void StartActive::Private::initEnvironment()
{
    QSettings config(QString(STARTACTIVE_DATA_PATH) + "/env.conf", QSettings::IniFormat);

    qDebug() << "Setting environment variables:";

    foreach (const QString & key, config.allKeys()) if (key[0] != '#') {
        QString value = config.value(key).toString();

        qDebug() << key << "=" << value;

        ::setenv(
                key.toAscii(),
                value.toAscii(),
                1
            );

    }
}

void StartActive::load(const QString & modules)
{
    qDebug() << "Loading modules: " << modules;

    d->scheduled = modules.split(",");

    while (!d->scheduled.empty()) {
        d->readModuleData(d->scheduled.first());
    }

    // d->printLevels();

    foreach (const QString & module, d->depends.keys()) {
        qDebug() << "module" << module << "is a prerequisite of" << d->depends[module];
    }

    foreach (const QString & module, d->requires.keys()) {
        qDebug() << "module" << module << "depends on" << d->requires[module];
    }

    qDebug() << "\n\n\nSTARTING, total:" << d->modules.keys() << d->modules.size();

    d->modulesFinished = 0;
    d->stage = 0;

    d->startFreeModules();
}

void StartActive::Private::readModuleData(const QString module)
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

    modules[module] = data;

    const QSet < QString > & dependsOn =
        config.value("Module/depends", QStringList()).toStringList().toSet();

    int order = dependsOn.count();
    if (order == 1 && dependsOn.contains(QString())) {
        order = 0;
    }

    while (runOrder.size() <= order) {
        runOrder << QSet < QString > ();
    }

    runOrder[order] << module;

    if (order) {
        requires[module] += dependsOn;

        foreach (const QString & dep, dependsOn) {
            depends[dep] << module;

            if (!running.contains(dep)
                    && !scheduled.contains(dep)
                    && !modules.contains(dep)
            ) {
                scheduled << dep;
            }
        }
    }
}

void StartActive::Private::startFreeModules()
{
    // printLevels();

    QSet < QString > starting = runOrder[0];
    runOrder[0].clear();

    qDebug() << "Free modules" << starting << running;

    if (starting.size() == 0 && running.size() == 0) {
        SplashWindow::close();

        int leftCount = 0;
        foreach (const QSet < QString > & left, runOrder) {
            leftCount += left.size();
        }

        if (leftCount > 0) {
            qDebug() << "ERROR:" << leftCount << "modules not started - dead-lock detected";
        } else {
            qDebug() << "##### Starting finished. We are all live and well";
        }

    } else {
        foreach (const QString & module, starting) {
            startModule(module);
        }
    }

}

void StartActive::Private::startModule(const QString & module)
{
    if (module.isEmpty()) return;

    qDebug() << "Starting " << module;
    Module * data = modules[module];

    if (data->wait != DontWait) {
        running += module;
    }
    qDebug() << "Running modules" << running;

    new ProcessStarter(
            module,
            data->exec,
            q,
            "moduleStarted",
            data->dbus
        );
}

void StartActive::moduleStarted(const QString & module)
{
    qDebug() << "module started" << module << d->runOrder.size();
    d->modulesFinished++;

    d->running -= module;
    qDebug() << "Running modules" << d->running;

    int _stage = ceil((d->modulesFinished / (qreal) d->modules.size()) * STAGE_COUNT);
    if (d->stage != _stage) {
        // send the event to the splash
        d->stage = _stage;
        qDebug() << "SPLASH STAGE" << d->stage;

        SplashWindow::setStage(d->stage);
    }

    d->printLevels();

    d->runOrder[0] -= module;

    qDebug() << "Modules that depend on the current one:" << d->depends[module];

    for (int level = 0; level < d->runOrder.size(); level++) {
        QSet < QString > intersection = d->runOrder[level];
        intersection.intersect(d->depends[module]);

        foreach (const QString & dep, intersection) {
            qDebug() << "module" << dep << "found in" << level << "moved";
            d->runOrder[level]     -= dep;
            d->runOrder[level - 1] += dep;
        }
    }

    d->startFreeModules();
}
