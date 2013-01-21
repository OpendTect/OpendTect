#ifndef qptr_h
#define qptr_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Kristofer Tingdahl
 Date:          Nov 2012
 RCS:           $Id$
________________________________________________________________________

-*/

#include "commondefs.h"

class NotifierAccess;
class i_QPtrImpl;
namespace  Threads { class Mutex; }

mFDQtclass(QObject);

/*!
\brief Class with a pointer to a QObject. The pointer will be set to null
when the QObject goes out of scope.
*/

mExpClass(Basic) QObjPtr
{
public:
				QObjPtr(mQtclass(QObject)* = 0);
				~QObjPtr();

				operator mQtclass(QObject)*();
				operator const mQtclass(QObject)*() const;
    mQtclass(QObject*)		operator->();
    const mQtclass(QObject*)	operator->() const;
    
    mQtclass(QObject)*		operator=(mQtclass(QObject)*);

    Threads::Mutex&		mutex();
    
    NotifierAccess&		deleteNotifier();

protected:

    i_QPtrImpl*		impl_;
};


#endif
