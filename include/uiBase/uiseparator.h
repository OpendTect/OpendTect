#ifndef uiseparator_H
#define uiseparator_H

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Lammertink
 Date:          17/1/2001
 RCS:           $Id: uiseparator.h,v 1.8 2012-08-03 13:00:53 cvskris Exp $
________________________________________________________________________

-*/

#include "uibasemod.h"
#include "uiobj.h"

//class QFrame;

class uiSeparatorBody;

mClass(uiBase) uiSeparator : public uiObject
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

