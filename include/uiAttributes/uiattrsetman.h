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

