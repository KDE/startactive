/*
 *   Copyright (C) 2016 Ivan Čukić <ivan.cukic(at)kde.org>
 *
 *   This library is free software; you can redistribute it and/or
 *   modify it under the terms of the GNU Lesser General Public
 *   License as published by the Free Software Foundation; either
 *   version 2.1 of the License, or (at your option) version 3, or any
 *   later version accepted by the membership of KDE e.V. (or its
 *   successor approved by the membership of KDE e.V.), which shall
 *   act as a proxy defined in Section 6 of version 3 of the license.
 *
 *   This library is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *   Lesser General Public License for more details.
 *
 *   You should have received a copy of the GNU Lesser General Public
 *   License along with this library.
 *   If not, see <http://www.gnu.org/licenses/>.
 */

#include "Modules.h"

#include <QDebug>
#include <QSettings>

#include "config-startplasma.h"

const QString modulePattern = QString(STARTPLASMA_MODULE_DIR) + "/%1.module";

Modules::Modules(const QStringList &finalModules)
{
    QStringList infoLoadQueue = finalModules;

    while (!infoLoadQueue.isEmpty()) {
        auto currentModule = infoLoadQueue.takeFirst();

        QSettings config(modulePattern.arg(currentModule), QSettings::IniFormat);

        ModuleData data;
        data.exec = config.value("Module/exec", QString()).toString();

        const auto waitFor = config.value("Module/wait").toString();
        if (waitFor == "dbus") {
            data.wait = WaitForDbus;
            data.dbus = config.value("Module/dbus", QString()).toString();

        } else if (waitFor == "end") {
            data.wait = WaitForEnd;

        } else {
            data.wait = DontWait;

        }

        const auto envMode = config.value("Module/envMode", "inherit").toString();
        if (envMode == "inherit") {
            data.envMode = Modules::EnvironmentMode::Inherit;

        } else if (envMode == "append") {
            data.envMode = Modules::EnvironmentMode::Append;
            data.env = config.value("Module/env").toHash();
        }


        modules[currentModule] = data;

        for (const auto &dep :
             config.value("Module/depends", QStringList()).toStringList()) {

            if (dep.isEmpty()) continue;

            // Do we already have this module loaded or queued for loading?
            if (!modules.contains(dep) && !infoLoadQueue.contains(dep)) {
                infoLoadQueue << dep;
            }

            deps.insert(currentModule, dep);
        }

        depsCount[currentModule] = deps.count(currentModule);
    }

    qDebug() << "--- Modules ---";
    for (const auto& module: modules.keys()) {
        qDebug() << "Module: " << module;

        const auto &data = modules[module];
        qDebug() << "\t Exec:" << data.exec
                 << "| Wait:" << data.wait
                 << "| DBus:" << data.dbus
                 ;

        qDebug() << "\t Depends on:  " << deps.values(module);
        qDebug() << "\t Required by: " << deps.keys(module);
        qDebug() << "\t Degree:      " << depsCount[module];
    }
}

void Modules::removeModule(const QString &module)
{
    for (const auto &dep: deps.keys(module)) {
        depsCount[dep]--;
    }

    depsCount.remove(module);
}

QStringList Modules::freeModules() const
{
    return depsCount.keys(0);
}

int Modules::count() const
{
    return modules.count();
}

int Modules::remaining() const
{
    return depsCount.count();
}


