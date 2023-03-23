#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "gendefs.h"

class NotifierAccess;
class i_QPtrImpl;
namespace  Threads { class Mutex; }

mFDQtclass(QObject);

/*!
\brief Class with a pointer to a QObject. The pointer will be set to null
when the QObject goes out of scope.
*/

class QObjPtr
{
public:
				QObjPtr(mQtclass(QObject)* = nullptr);
				~QObjPtr();

				operator mQtclass(QObject)*();
				operator const mQtclass(QObject)*() const;
    mQtclass(QObject*)		operator->();
    const mQtclass(QObject*)	operator->() const;
    
    mQtclass(QObject)*		operator=(mQtclass(QObject)*);

    Threads::Mutex&		mutex();
    
    NotifierAccess&		deleteNotifier();

protected:

    i_QPtrImpl*			impl_;
};
