#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiattributesmod.h"
#include "uiobjfileman.h"

class uiListBox;

/*!
\brief AttributeSet Manager
*/

mExpClass(uiAttributes) uiAttrSetMan : public uiObjFileMan
{ mODTextTranslationClass(uiAttrSetMan);
public:
			uiAttrSetMan(uiParent*,bool is2d=false);
			~uiAttrSetMan();

    mDeclInstanceCreatedNotifierAccess(uiAttrSetMan);

protected:

    void		mkFileInfo() override;

    uiListBox*		attribfld_;
};
