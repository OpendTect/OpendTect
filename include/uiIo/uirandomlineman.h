#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:           2015
________________________________________________________________________

-*/

#include "uiiocommon.h"
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

    virtual bool	gtItemInfo(const IOObj&,uiPhraseSet&) const;

};
