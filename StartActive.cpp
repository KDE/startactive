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

#include <KConfig>
#include <KConfigGroup>

#include <QX11Info>
#include <X11/Xlib.h>
#include <X11/Xcursor/Xcursor.h>

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
    QTime time;
};

void StartActive::Private::printLevels()
{
    // debugging
    for (int level = 0; level < runOrder.size(); level++) {
        qDebug() << "StartActive:\t" << level << " - " << runOrder[level];
    }
}

StartActive::StartActive(/*Display * display,*/ int argc, char ** argv)
    : QApplication(/*display,*/ argc, argv),
      d(new Private(this))
{
    KConfig c("kcminputrc");
    KConfigGroup cg(&c, "Mouse");
    const QString theme = cg.readEntry("cursorTheme", "plasmamobilemouse");

    // Apply the KDE cursor theme to ourselves
    XcursorSetTheme(QX11Info::display(), theme.toLatin1() );

    // Load the default cursor from the theme and apply it to the root window.
    Cursor handle = XcursorLibraryLoadCursor(QX11Info::display(), "left_ptr");
    XDefineCursor(QX11Info::display(), QX11Info::appRootWindow(), handle);
    XFreeCursor(QX11Info::display(), handle); // Don't leak the cursor

    setenv("XCURSOR_THEME", theme.toLatin1(), 1);

    d->time.start();
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
    qDebug() << "StartActive:\t" << "The system wants us to quit.";

    // Ignoring at the moment

}

void StartActive::Private::initDBus()
{
    qDebug() << "\n\n--- StartActive -----------------------------------------";
    qDebug() << "StartActive:\t" << "Initializing DBus";

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

            qDebug() << "StartActive:\t" << "DBUS" << key << "=" << value;

            ::setenv(
                    key.toAscii(),
                    value.toAscii(),
                    1
                );

        }
    } else {
        qDebug() << "StartActive:\t" << "DBus already running at:" << ::getenv("DBUS_SESSION_BUS_ADDRESS");
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
    qDebug() << "\n\n--- StartActive -----------------------------------------";
    qDebug() << "StartActive:\t" << "Setting environment variables:";

    QSettings config(QString(STARTACTIVE_DATA_PATH) + "/env.conf", QSettings::IniFormat);

    foreach (const QString & key, config.allKeys()) if (key[0] != '#') {
        QString value = config.value(key).toString();

        qDebug() << "StartActive:\t" << key << "=" << value;

        ::setenv(
                key.toAscii(),
                value.toAscii(),
                1
            );

    }
}

void StartActive::load(const QString & modules)
{
    qDebug() << "StartActive:\t" << "Loading modules: " << modules;

    d->scheduled = modules.split(",");

    while (!d->scheduled.empty()) {
        d->readModuleData(d->scheduled.first());
    }

    // d->printLevels();

    // foreach (const QString & module, d->depends.keys()) {
    //     qDebug() << "StartActive:\t" << "module" << module << "is a prerequisite of" << d->depends[module];
    // }

    // foreach (const QString & module, d->requires.keys()) {
    //     qDebug() << "StartActive:\t" << "module" << module << "depends on" << d->requires[module];
    // }

    // qDebug() << "StartActive:\t" << "\n\n\nSTARTING, total:" << d->modules.keys() << d->modules.size();

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
    QSet < QString > starting = runOrder[0];
    runOrder[0].clear();

    qDebug() << "StartActive:\t" << "these are the currently running modules:" << running;
    qDebug() << "StartActive:\t" << "starting the following modules:" << starting;

    // Did we end or we are in a dead-lock
    if (starting.size() == 0 && running.size() == 0) {
        SplashWindow::close();

        int leftCount = 0;
        foreach (const QSet < QString > & left, runOrder) {
            leftCount += left.size();
        }

        if (leftCount > 0) {
            qDebug() << "StartActive:\t" << "ERROR:" << leftCount << "modules not started - dead-lock detected";
            printLevels();

        } else {
            qDebug() << "StartActive:\t" << "##### Starting finished. We are all live and well (" << time.elapsed() << "ms )";
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

    qDebug() << "StartActive:\t" << "starting module " << module;
    Module * data = modules[module];

    if (data->wait != DontWait) {
        running += module;
    }

    // Starting even if the exec is empty, to have the notification that
    // it has /finished/
    new ProcessStarter(
            module,
            data->exec,
            q, "moduleStarted",
            data->dbus
        );
}

void StartActive::moduleStarted(const QString & module)
{
    qDebug() << "StartActive:\t" << "module started" << module << d->runOrder.size() << "(" << d->time.elapsed() << "ms )";
    d->modulesFinished++;

    d->running -= module;

    int _stage = ceil((d->modulesFinished / (qreal) d->modules.size()) * STAGE_COUNT);
    if (d->stage != _stage) {
        // send the event to the splash
        d->stage = _stage;
        SplashWindow::setStage(d->stage);
    }

    d->printLevels();

    d->runOrder[0] -= module;

    qDebug() << "StartActive:\t" << "Modules that depend on the current one:" << d->depends[module];

    for (int level = 0; level < d->runOrder.size(); level++) {
        QSet < QString > intersection = d->runOrder[level];
        intersection.intersect(d->depends[module]);

        foreach (const QString & dep, intersection) {
            d->runOrder[level]     -= dep;
            d->runOrder[level - 1] += dep;
        }
    }

    d->startFreeModules();
}
