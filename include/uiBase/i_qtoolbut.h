#ifndef i_qtoolbut_h
#define i_qtoolbut_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Lammertink
 Date:          30/05/2001
 RCS:           $Id: i_qtoolbut.h,v 1.3 2004-09-13 09:45:04 nanne Exp $
________________________________________________________________________

-*/

#include <qtoolbutton.h>
#include "callback.h"


class i_QToolButReceiver : public QObject, public CallBacker
{
  Q_OBJECT
public:
    inline              i_QToolButReceiver(QObject* prnt=0, const char* nm=0)
                            : QObject(prnt, nm)
                            , pressed( this ) {}

    Notifier<i_QToolButReceiver> pressed;

public slots:

    void                buttonPressed() { pressed.trigger(); }

};



#endif
