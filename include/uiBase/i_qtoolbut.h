#ifndef i_qtoolbut_h
#define i_qtoolbut_h

/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        A.H. Lammertink
 Date:          30/05/2001
 RCS:           $Id: i_qtoolbut.h,v 1.1 2001-05-30 16:36:13 arend Exp $
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
