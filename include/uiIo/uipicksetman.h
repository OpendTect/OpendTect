#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:           2003
________________________________________________________________________

-*/

#include "uiiomod.h"
#include "uiobjfileman.h"

/*! \brief
PickSet manager
*/

class uiToolButton;

mExpClass(uiIo) uiPickSetMan : public uiObjFileMan
{ mODTextTranslationClass(uiPickSetMan);
public:
				uiPickSetMan(uiParent*,
					     const char* fixedtrkey=0);
				~uiPickSetMan();

    mDeclInstanceCreatedNotifierAccess(uiPickSetMan);

protected:
    uiToolButton*		mergebut_;

    void			ownSelChg() override;
    void			mkFileInfo() override;
    void			mergeSets(CallBacker*);

};

