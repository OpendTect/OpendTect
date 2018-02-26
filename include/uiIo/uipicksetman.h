#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:           2003
________________________________________________________________________

-*/

#include "uiiocommon.h"
#include "uiobjfileman.h"

/*!\brief 'Manage PickSet/Polygon' */

class uiToolButton;

mExpClass(uiIo) uiPickSetMan : public uiObjFileMan
{ mODTextTranslationClass(uiPickSetMan);
public:

			uiPickSetMan(uiParent*,const char* fixedtrkey=0);
			~uiPickSetMan();

    mDeclInstanceCreatedNotifierAccess(uiPickSetMan);

protected:

    uiToolButton*	mergebut_;

    virtual void	ownSelChg();
    virtual bool	gtItemInfo(const IOObj&,uiPhraseSet&) const;
    void		mergeSets(CallBacker*);

};
