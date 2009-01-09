#ifndef uiseparator_H
#define uiseparator_H

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Lammertink
 Date:          17/1/2001
 RCS:           $Id: uiseparator.h,v 1.5 2009-01-09 04:26:14 cvsnanne Exp $
________________________________________________________________________

-*/

#include <uiobj.h>

//class QFrame;

class uiSeparatorBody;

mClass uiSeparator : public uiObject
{
public:

                        uiSeparator(uiParent*,const char* nm="Separator", 
				    bool hor=true, bool raised=true);


    void		setRaised(bool);

private:

    uiSeparatorBody*	body_;
    uiSeparatorBody&	mkbody(uiParent*, const char*, bool, bool);

};

#endif
