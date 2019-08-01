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
class uiToolButton;


/*!\brief 'Manage PickSet/Polygon' */


mExpClass(uiIo) uiPickSetMan : public uiObjFileMan
{ mODTextTranslationClass(uiPickSetMan);
public:

			uiPickSetMan(uiParent*,const char* fixedtrkey=0);
			~uiPickSetMan();

    mDeclInstanceCreatedNotifierAccess(uiPickSetMan);

protected:

    uiToolButton*	edbut_;
    uiToolButton*	mergebut_;

    virtual void	ownSelChg();
    virtual bool	gtItemInfo(const IOObj&,uiPhraseSet&) const;
    void		edSetCB(CallBacker*);
    void		mergeSetsCB(CallBacker*);

};
