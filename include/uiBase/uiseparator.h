#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Lammertink
 Date:          17/1/2001
________________________________________________________________________

-*/

#include "uibasemod.h"
#include "uiobj.h"

//class QFrame;

class uiSeparatorBody;

mExpClass(uiBase) uiSeparator : public uiObject
{
public:

                        uiSeparator(uiParent*,const char* nm="Separator",
				    OD::Orientation ori=OD::Horizontal,
				    bool raised=true);


    void		setRaised(bool);

private:

    uiSeparatorBody*	body_;
    uiSeparatorBody&	mkbody(uiParent*, const char*, bool, bool);

};
