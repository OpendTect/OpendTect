#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
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
