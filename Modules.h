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

#ifndef MODULES_H
#define MODULES_H

#include <QStringList>
#include <QHash>
#include <QMultiHash>

class Modules {
public:
    Modules(const QStringList &finalModules);

    QStringList freeModules() const;

    void removeModule(const QString &module);

    int count() const;
    int remaining() const;


    enum WaitMode {
        DontWait = 0,
        WaitForDbus = 1,
        WaitForEnd = 2
    };

    class ModuleData {
    public:
        QString exec;
        WaitMode wait;
        QString dbus;
    };

    inline
    ModuleData data(const QString &id) const
    {
        return modules[id];
    }


private:
    QHash<QString, ModuleData> modules;
    QMultiHash<QString, QString> deps;
    QHash<QString, int> depsCount;

};


#endif // include guard end

