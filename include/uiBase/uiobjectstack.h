#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          7/9/2000
________________________________________________________________________

-*/

#include "uibasemod.h"
#include "uiobj.h"

class uiGroup;
class uiObjStackBody;

mExpClass(uiBase) uiObjectStack : public uiObject
{
public:

                        uiObjectStack(uiParent*,const char*);

    int			addObject(uiObject*);
    int			addGroup(uiGroup*);
    void		setCurrentObject(int);
    int			size() const;

private:

    uiObjStackBody*	body_;
    uiObjStackBody&	mkbody(uiParent*,const char*);

};
