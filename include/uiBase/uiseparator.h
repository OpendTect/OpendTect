#ifndef uiseparator_H
#define uiseparator_H

/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        A.H. Lammertink
 Date:          17/1/2001
 RCS:           $Id: uiseparator.h,v 1.2 2001-08-23 14:59:17 windev Exp $
________________________________________________________________________

-*/

#include <uiobj.h>

//class QFrame;

class uiSeparatorBody;

class uiSeparator : public uiObject
{
public:

                        uiSeparator(uiParent*,const char* nm="Separator", 
				    bool hor=true, bool raised=false);


    void		setRaised( bool yn );

private:

    uiSeparatorBody*	body_;
    uiSeparatorBody&	mkbody(uiParent*, const char*, bool, bool);

};

#endif
