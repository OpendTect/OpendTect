#ifndef i_qtoolbut_h
#define i_qtoolbut_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Lammertink
 Date:          30/05/2001
 RCS:           $Id: i_qtoolbut.h,v 1.2 2003-11-07 12:21:54 bert Exp $
________________________________________________________________________

-*/

#include "uitoolbar.h"
#include "pixmap.h"
#include <qtoolbar.h>
#include <qtoolbutton.h>


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
