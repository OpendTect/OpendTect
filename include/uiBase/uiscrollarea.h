#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uibasemod.h"

#include "uiobj.h"

class ODScrollArea;

mExpClass(uiBase) uiScrollArea : public uiObject
{
friend class		ODScrollArea;
public:
			uiScrollArea(uiParent*,const char* nm="uiScrollArea");
			~uiScrollArea();

    void		setObject(uiObject*);
    uiObject*		getObject();

    void		setObjectResizable(bool);

    void		limitHeight( bool yn )		{ limitheight_ = yn; }
    void		limitWidth( bool yn )		{ limitwidth_ = yn; }

protected:

    ODScrollArea&	mkbody(uiParent*,const char*);
    ODScrollArea*	body_;

    bool		limitheight_	= false;
    bool		limitwidth_	= false;
    uiObject*		object_		= nullptr;
};
