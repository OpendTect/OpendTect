#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:           2003
________________________________________________________________________

-*/

#include "uiattributesmod.h"
#include "uiobjfileman.h"


/*!
\brief AttributeSet Manager
*/

mExpClass(uiAttributes) uiAttrSetMan : public uiObjFileMan
{ mODTextTranslationClass(uiAttrSetMan);
public:
    				uiAttrSetMan(uiParent*);
				~uiAttrSetMan();

    mDeclInstanceCreatedNotifierAccess(uiAttrSetMan);

protected:

    void			mkFileInfo();
};
