#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:          October 2020
________________________________________________________________________

-*/

#include "uibasemod.h"
#include "uigroup.h"
#include "uiobj.h"

mFDQtclass(QScrollrea)


mExpClass(uiBase) uiScrollArea : public uiObject
{
friend class		uiScrollAreaBody;
public:
			uiScrollArea(uiParent*,const char* nm="uiScrollArea");
			~uiScrollArea();

    void		setObject(uiObject*);
    uiObject*		getObject();

protected:

    uiScrollAreaBody&	mkbody(uiParent*,const char*);
    uiScrollAreaBody*	body_;

    uiObject*		object_;
};
