/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
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

#ifndef QDECLARATIVECONTEXT_P_H
#define QDECLARATIVECONTEXT_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include "qdeclarativecontext.h"

#include "private/qdeclarativedata_p.h"
#include "private/qdeclarativeintegercache_p.h"
#include "private/qdeclarativetypenamecache_p.h"
#include "private/qdeclarativenotifier_p.h"
#include "qdeclarativelist.h"
#include "private/qdeclarativeparser_p.h"

#include <QtCore/qhash.h>
#include <QtScript/qscriptvalue.h>
#include <QtCore/qset.h>

#include <private/qobject_p.h>
#include "private/qdeclarativeguard_p.h"

#include <private/qv8_p.h>

QT_BEGIN_NAMESPACE

class QV8Bindings;
class QDeclarativeContext;
class QDeclarativeExpression;
class QDeclarativeEngine;
class QDeclarativeExpression;
class QDeclarativeExpressionPrivate;
class QDeclarativeAbstractExpression;
class QDeclarativeV4Bindings;
class QDeclarativeContextData;

class QDeclarativeContextPrivate : public QObjectPrivate
{
    Q_DECLARE_PUBLIC(QDeclarativeContext)
public:
    QDeclarativeContextPrivate();

    QDeclarativeContextData *data;

    QList<QVariant> propertyValues;
    int notifyIndex;

    static QDeclarativeContextPrivate *get(QDeclarativeContext *context) {
        return static_cast<QDeclarativeContextPrivate *>(QObjectPrivate::get(context));
    }
    static QDeclarativeContext *get(QDeclarativeContextPrivate *context) {
        return static_cast<QDeclarativeContext *>(context->q_func());
    }

    // Only used for debugging
    QList<QPointer<QObject> > instances;

    static int context_count(QDeclarativeListProperty<QObject> *);
    static QObject *context_at(QDeclarativeListProperty<QObject> *, int);
};

class QDeclarativeComponentAttached;
class QDeclarativeGuardedContextData;
class Q_AUTOTEST_EXPORT QDeclarativeContextData
{
public:
    QDeclarativeContextData();
    QDeclarativeContextData(QDeclarativeContext *);
    void clearContext();
    void destroy();
    void invalidate();

    inline bool isValid() const {
        return engine && (!isInternal || !contextObject || !QObjectPrivate::get(contextObject)->wasDeleted);
    }

    // My parent context and engine
    QDeclarativeContextData *parent;
    QDeclarativeEngine *engine;

    void setParent(QDeclarativeContextData *, bool parentTakesOwnership = false);
    void refreshExpressions();

    void addObject(QObject *);

    QUrl resolvedUrl(const QUrl &);

    // My containing QDeclarativeContext.  If isInternal is true this owns publicContext.  
    // If internal is false publicContext owns this.
    QDeclarativeContext *asQDeclarativeContext();
    QDeclarativeContextPrivate *asQDeclarativeContextPrivate();
    quint32 isInternal:1;
    quint32 ownedByParent:1; // unrelated to isInternal; parent context deletes children if true.
    quint32 isJSContext:1;
    quint32 isPragmaLibraryContext:1;
    quint32 dummy:28;
    QDeclarativeContext *publicContext;

    // Property name cache
    QDeclarativeIntegerCache *propertyNames;

    // Context object
    QObject *contextObject;

    // Any script blocks that exist on this context
    QList<v8::Persistent<v8::Object> > importedScripts;

    // Context base url
    QUrl url;

    // List of imports that apply to this context
    QDeclarativeTypeNameCache *imports;

    // My children
    QDeclarativeContextData *childContexts;

    // My peers in parent's childContexts list
    QDeclarativeContextData  *nextChild;
    QDeclarativeContextData **prevChild;

    // Expressions that use this context
    QDeclarativeAbstractExpression *expressions;

    // Doubly-linked list of objects that are owned by this context
    QDeclarativeData *contextObjects;

    // Doubly-linked list of context guards (XXX merge with contextObjects)
    QDeclarativeGuardedContextData *contextGuards;

    // id guards
    struct ContextGuard : public QDeclarativeGuard<QObject>
    {
        ContextGuard() : context(0) {}
        inline ContextGuard &operator=(QObject *obj)
        { QDeclarativeGuard<QObject>::operator=(obj); return *this; }
        virtual void objectDestroyed(QObject *) { 
            if (context->contextObject && !QObjectPrivate::get(context->contextObject)->wasDeleted) bindings.notify(); 
        }
        QDeclarativeContextData *context;
        QDeclarativeNotifier bindings;
    };
    ContextGuard *idValues;
    int idValueCount;
    void setIdProperty(int, QObject *);
    void setIdPropertyData(QDeclarativeIntegerCache *);

    // Linked contexts. this owns linkedContext.
    QDeclarativeContextData *linkedContext;

    // Linked list of uses of the Component attached property in this
    // context
    QDeclarativeComponentAttached *componentAttached;

    // Optimized binding objects.  Needed for deferred properties.
    QDeclarativeV4Bindings *v4bindings;
    QV8Bindings *v8bindings;

    // Return the outermost id for obj, if any.
    QString findObjectId(const QObject *obj) const;

    static QDeclarativeContextData *get(QDeclarativeContext *context) {
        return QDeclarativeContextPrivate::get(context)->data;
    }

private:
    ~QDeclarativeContextData() {}
};

class QDeclarativeGuardedContextData
{
public:
    inline QDeclarativeGuardedContextData();
    inline QDeclarativeGuardedContextData(QDeclarativeContextData *);
    inline ~QDeclarativeGuardedContextData();

    inline QDeclarativeContextData *contextData();
    inline void setContextData(QDeclarativeContextData *);

    inline bool isNull() const { return !m_contextData; }

    inline operator QDeclarativeContextData*() const { return m_contextData; }
    inline QDeclarativeContextData* operator->() const { return m_contextData; }
    inline QDeclarativeGuardedContextData &operator=(QDeclarativeContextData *d);

private:
    QDeclarativeGuardedContextData &operator=(const QDeclarativeGuardedContextData &);
    QDeclarativeGuardedContextData(const QDeclarativeGuardedContextData &);
    friend class QDeclarativeContextData;

    inline void clear();

    QDeclarativeContextData *m_contextData;
    QDeclarativeGuardedContextData  *m_next;
    QDeclarativeGuardedContextData **m_prev;
};

QDeclarativeGuardedContextData::QDeclarativeGuardedContextData()
: m_contextData(0), m_next(0), m_prev(0)
{
}

QDeclarativeGuardedContextData::QDeclarativeGuardedContextData(QDeclarativeContextData *data)
: m_contextData(0), m_next(0), m_prev(0)
{
    setContextData(data);
}

QDeclarativeGuardedContextData::~QDeclarativeGuardedContextData()
{
    clear();
}

void QDeclarativeGuardedContextData::setContextData(QDeclarativeContextData *contextData)
{
    clear();

    if (contextData) {
        m_contextData = contextData;
        m_next = contextData->contextGuards;
        if (m_next) m_next->m_prev = &m_next;
        m_prev = &contextData->contextGuards;
        contextData->contextGuards = this;
    }
}

QDeclarativeContextData *QDeclarativeGuardedContextData::contextData()
{
    return m_contextData;
}

void QDeclarativeGuardedContextData::clear()
{
    if (m_prev) {
        *m_prev = m_next;
        if (m_next) m_next->m_prev = m_prev;
        m_contextData = 0;
        m_next = 0;
        m_prev = 0;
    }
}

QDeclarativeGuardedContextData &
QDeclarativeGuardedContextData::operator=(QDeclarativeContextData *d)
{
    setContextData(d);
    return *this;
}

QT_END_NAMESPACE

#endif // QDECLARATIVECONTEXT_P_H
