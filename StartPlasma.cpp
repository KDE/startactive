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
#include "StartPlasma.h"

#include "startplasmaadaptor.h"

#include <signal.h>
#include <stdlib.h>

#include <QSettings>

#include "SignalListener.h"
#include "config-startplasma.h"
#include "ModuleStarter.h"
#include "Modules.h"

#include "splash/SplashWindow.h"

#include <KConfig>
#include <KConfigGroup>

#include <memory>

// #include <QX11Info>
// #include <X11/Xlib.h>
// #include <X11/Xcursor/Xcursor.h>

class StartPlasma::Private {
public:
    Private(StartPlasma * parent)
        : q(parent)
    {
    }

    StartPlasma * const q;

    QSet<QString> running;

    void startFreeModules();
    void startModule(const QString &module);
    void printLevels();

    void initEnvironment();
    void initDBus();

    // std::unique_ptr<SplashWindow> splash;
    std::unique_ptr<Modules> modules;

    int stage;
    QTime time;
};

void StartPlasma::Private::printLevels()
{
    // debugging
    // for (int level = 0; level < runOrder.size(); level++) {
    //     qDebug() << "StartPlasma:\t" << level << " - " << runOrder[level];
    // }
}

StartPlasma::StartPlasma(int argc, char ** argv)
    : QCoreApplication(argc, argv),
      d(new Private(this))
{
    arguments();
    QMetaObject::invokeMethod(this, "init", Qt::QueuedConnection);
}

void StartPlasma::init()
{
    qDebug() << "StartPlasma:\tStarting...";

    KConfig c("kcminputrc");
    KConfigGroup cg(&c, "Mouse");
    const QString theme = cg.readEntry("cursorTheme", "breeze_cursors");

    // Apply the KDE cursor theme to ourselves
    // XcursorSetTheme(QX11Info::display(), theme.toLatin1() );

    // Load the default cursor from the theme and apply it to the root window.
    // Cursor handle = XcursorLibraryLoadCursor(QX11Info::display(), "left_ptr");
    // XDefineCursor(QX11Info::display(), QX11Info::appRootWindow(), handle);
    // XFreeCursor(QX11Info::display(), handle); // Don't leak the cursor

    setenv("XCURSOR_THEME", theme.toLatin1(), 1);

    d->time.start();
    d->initEnvironment();
    d->initDBus();

    // We have to deinitialize before quitting. So, we need to process
    // SIGHUP our way
    SignalListener::self().registerSignal(SIGHUP);
    connect(&SignalListener::self(), SIGNAL(signalReceived(int)),
            this, SLOT(quit()));

    if (QCoreApplication::arguments().size() > 1) {
        load(QCoreApplication::arguments().last());
    } else {
        load("desktop-x11");
    }

    // d->splash.reset(new SplashWindow());
}

StartPlasma::~StartPlasma()
{
    delete d;
}

void StartPlasma::quit()
{
    qDebug() << "StartPlasma:\t" << "The system wants us to quit.";

    // Ignoring at the moment

}

void StartPlasma::Private::initDBus()
{
    qDebug() << "\n\n--- StartPlasma -----------------------------------------";
    qDebug() << "StartPlasma:\t" << "Initializing DBus";

    // Check whether dbus process is running
    if (!getenv("DBUS_SESSION_BUS_ADDRESS")) {
        QProcess app;
        app.start("dbus-launch");

        app.waitForFinished();

        char buf[1024];

        qint64 lineLength;

        while ((lineLength = app.readLine(buf, sizeof(buf))) != -1) {
            QString line = QString(buf).trimmed();

            int pos = line.indexOf('=');

            const QString &key = line.left(pos);
            const QString &value = line.mid(pos + 1);

            qDebug() << "StartPlasma:\t" << "DBUS" << key << "=" << value;

            setenv(
                    key.toLatin1(),
                    value.toLatin1(),
                    1
                );

        }
    } else {
        qDebug() << "StartPlasma:\t" << "DBus already running at:" << getenv("DBUS_SESSION_BUS_ADDRESS");
    }

    auto dbus = QDBusConnection::sessionBus();

    new StartPlasmaAdaptor(q);

    if (!dbus.registerService("org.kde.PlasmaStarter")) {
        qDebug() << "we are already running, quitting";
        QCoreApplication::quit();
    }

    // Proper object name
    dbus.registerObject("/StartPlasma", q);

    // For the compatibility with kquitapp
    dbus.registerObject("/MainApplication", q);
}

void StartPlasma::Private::initEnvironment()
{
    qDebug() << "\n\n--- StartPlasma -----------------------------------------";
    qDebug() << "StartPlasma:\t" << "Setting environment variables:";

    QSettings config(QString(STARTPLASMA_DATA_DIR) + "/env.conf", QSettings::IniFormat);

    foreach (const QString &key, config.allKeys()) if (key[0] != '#') {
        QString value = config.value(key).toString();

        qDebug() << "StartPlasma:\t" << key << "=" << value;

        setenv(
                key.toLatin1(),
                value.toLatin1(),
                1
            );

    }
}

void StartPlasma::load(const QString &modules)
{
    qDebug() << "StartPlasma:\t" << "Loading modules: " << modules;

    d->modules.reset(new Modules(modules.split(",")));

    d->stage = 0;

    d->startFreeModules();
}

void StartPlasma::Private::startFreeModules()
{
    for (const auto& module: modules->freeModules()) {
        startModule(module);
    }

    // QSet<QString> starting = runOrder[0];
    // runOrder[0].clear();
    //
    // qDebug() << "StartPlasma:\t" << "these are the currently running modules:" << running;
    // qDebug() << "StartPlasma:\t" << "starting the following modules:" << starting;
    //
    // // Did we end or we are in a dead-lock
    // if (starting.size() == 0 && running.size() == 0) {
    //     // splash.reset();
    //
    //     int leftCount = 0;
    //     foreach (const QSet<QString> & left, runOrder) {
    //         leftCount += left.size();
    //     }
    //
    //     if (leftCount > 0) {
    //         qDebug() << "StartPlasma:\t" << "ERROR:" << leftCount << "modules not started - dead-lock detected";
    //         printLevels();
    //
    //     } else {
    //         qDebug() << "StartPlasma:\t" << "##### Starting finished. We are all live and well (" << time.elapsed() << "ms )";
    //     }
    //
    // } else {
    //     foreach (const QString &module, starting) {
    //         startModule(module);
    //     }
    // }
    //
}

void StartPlasma::Private::startModule(const QString &module)
{
    if (module.isEmpty()) return;

    if (running.contains(module)) return;

    running << module;

    qDebug() << "StartPlasma:\t" << "starting module " << module;
    const auto &data = modules->data(module);

    // Starting even if the exec is empty, to have the notification that
    // it has /finished/
    new ModuleStarter(
            module,
            data.exec,
            q, "moduleStarted",
            data.dbus,
            data.envMode,
            data.env
        );
}

void StartPlasma::moduleStarted(const QString &module)
{
    d->modules->removeModule(module);

    int _stage = ceil((d->modules->remaining() / (qreal) d->modules->count()) * STAGE_COUNT);

    if (d->stage != _stage) {
        // send the event to the splash
        d->stage = _stage;
        // if (d->splash) {
        //    d->splash->setStage(d->stage);
        // }
    }

    d->startFreeModules();
}
