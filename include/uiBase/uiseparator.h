#ifndef uiseparator_H
#define uiseparator_H

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Lammertink
 Date:          17/1/2001
 RCS:           $Id: uiseparator.h,v 1.3 2003-11-07 12:21:54 bert Exp $
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
