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

#ifndef QDECLARATIVEANIMATEDIMAGE_P_H
#define QDECLARATIVEANIMATEDIMAGE_P_H

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

#include "private/qdeclarativeimage_p_p.h"

#ifndef QT_NO_MOVIE

QT_BEGIN_NAMESPACE

class QMovie;
class QNetworkReply;

class QDeclarative1AnimatedImagePrivate : public QDeclarative1ImagePrivate
{
    Q_DECLARE_PUBLIC(QDeclarative1AnimatedImage)

public:
    QDeclarative1AnimatedImagePrivate()
      : playing(true), paused(false), preset_currentframe(0), _movie(0), reply(0), redirectCount(0)
    {
    }

    bool playing;
    bool paused;
    int preset_currentframe;
    QMovie *_movie;
    QNetworkReply *reply;
    int redirectCount;
};

QT_END_NAMESPACE

#endif // QT_NO_MOVIE

#endif // QDECLARATIVEANIMATEDIMAGE_P_H
