#ifndef uiseparator_H
#define uiseparator_H

/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        A.H. Lammertink
 Date:          17/1/2001
 RCS:           $Id: uiseparator.h,v 1.1 2001-01-26 09:54:09 arend Exp $
________________________________________________________________________

-*/

#include <uiobj.h>
class QFrame;
template <class T> class i_QObjWrapper;
mTemplTypeDefT(i_QObjWrapper,QFrame,i_QFrame)

class uiSeparator : public uiWrapObj<i_QFrame>
{
public:

                        uiSeparator(uiObject*,const char* nm="Separator", 
				    bool hor=true, bool raised=false);

    void		setRaised( bool yn );

protected:

    const QWidget*	qWidget_() const;
};

#endif
