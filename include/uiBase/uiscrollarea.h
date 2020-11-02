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
friend class		ODScrollArea;
public:
			uiScrollArea(uiParent*,const char* nm="uiScrollArea");
			~uiScrollArea();

    void		setObject(uiObject*);
    uiObject*		getObject();

    void		limitHeight( bool yn )		{ limitheight_ = yn; }
    void		limitWidth( bool yn )		{ limitwidth_ = yn; }

protected:

    ODScrollArea&	mkbody(uiParent*,const char*);
    ODScrollArea*	body_;

    bool		limitheight_	= false;
    bool		limitwidth_	= false;
    uiObject*		object_		= nullptr;
};
