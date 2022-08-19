#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
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
