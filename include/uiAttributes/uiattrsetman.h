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
{ mODTextTranslationClass(uiAttrSetMan)
public:

			uiAttrSetMan(uiParent*,bool is2d);
			~uiAttrSetMan();

    mDeclInstanceCreatedNotifierAccess(uiAttrSetMan);

protected:

    virtual void	ownSelChg();
    virtual bool	gtItemInfo(const IOObj&,uiPhraseSet&) const;

    uiListBox*		attribfld_;

};
