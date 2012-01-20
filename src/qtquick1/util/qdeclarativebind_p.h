/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: http://www.qt-project.org/
**
** This file is part of the QtDeclarative module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QDECLARATIVEBIND_H
#define QDECLARATIVEBIND_H

#include <QtDeclarative/qdeclarative.h>

#include <QtCore/qobject.h>

QT_BEGIN_HEADER

QT_BEGIN_NAMESPACE


class QDeclarative1BindPrivate;
class Q_AUTOTEST_EXPORT QDeclarative1Bind : public QObject, public QDeclarativeParserStatus
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(QDeclarative1Bind)
    Q_INTERFACES(QDeclarativeParserStatus)
    Q_PROPERTY(QObject *target READ object WRITE setObject)
    Q_PROPERTY(QString property READ property WRITE setProperty)
    Q_PROPERTY(QVariant value READ value WRITE setValue)
    Q_PROPERTY(bool when READ when WRITE setWhen)

public:
    QDeclarative1Bind(QObject *parent=0);
    ~QDeclarative1Bind();

    bool when() const;
    void setWhen(bool);

    QObject *object();
    void setObject(QObject *);

    QString property() const;
    void setProperty(const QString &);

    QVariant value() const;
    void setValue(const QVariant &);

protected:
    virtual void classBegin();
    virtual void componentComplete();

private:
    void eval();
};

QT_END_NAMESPACE

QML_DECLARE_TYPE(QDeclarative1Bind)

QT_END_HEADER

#endif
