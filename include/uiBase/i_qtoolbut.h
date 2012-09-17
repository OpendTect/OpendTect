#ifndef i_qtoolbut_h
#define i_qtoolbut_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Lammertink
 Date:          30/05/2001
 RCS:           $Id: i_qtoolbut.h,v 1.6 2009/07/22 16:01:20 cvsbert Exp $
________________________________________________________________________

-*/

#include <QToolButton>
#include "callback.h"


class i_QToolButReceiver : public QObject, public CallBacker
{
  Q_OBJECT
public:
    inline              i_QToolButReceiver(QObject* prnt=0, const char* nm=0)
                            : QObject(prnt)
                            , pressed( this )
    			{ setObjectName( nm ); }

    Notifier<i_QToolButReceiver> pressed;
    const CallBacker*		 getCallBacker() const	{ return this; }

public slots:

    void                buttonPressed() { pressed.trigger(); }

};



#endif
