#pragma once
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Oct 2006
________________________________________________________________________

-*/

#include "uiseismod.h"
#include "uiobjfileman.h"

class uiToolButton;

mExpClass(uiSeis) uiSeisPreStackMan : public uiObjFileMan
{  mODTextTranslationClass(uiSeisPreStackMan);
public:
			uiSeisPreStackMan(uiParent*,bool for2d);
			~uiSeisPreStackMan();

    mDeclInstanceCreatedNotifierAccess(uiSeisPreStackMan);

protected:
    static uiString	createCaption(bool for2d);

    bool		is2d_;

    void		ownSelChg();
    void		mkFileInfo();

    void		copyPush(CallBacker*);
    void                mergePush(CallBacker*);
    void                mkMultiPush(CallBacker*);

    uiToolButton*	copybut_;
    uiToolButton*	mergebut_;
};


