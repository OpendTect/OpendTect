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
RandomLine manager
*/


mExpClass(uiIo) uiRandomLineMan : public uiObjFileMan
{ mODTextTranslationClass(uiRandomLineMan);
public:
				uiRandomLineMan(uiParent*);
				~uiRandomLineMan();

    mDeclInstanceCreatedNotifierAccess(uiRandomLineMan);

protected:

    void			mkFileInfo() override;

};
