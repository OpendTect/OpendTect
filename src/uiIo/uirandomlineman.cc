/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:          September 2003
________________________________________________________________________

-*/

#include "uirandomlineman.h"

#include "ioobjctxt.h"
#include "randomlinetr.h"
#include "od_helpids.h"


mDefineInstanceCreatedNotifierAccess(uiRandomLineMan)

uiRandomLineMan::uiRandomLineMan( uiParent* p )
    : uiObjFileMan(p,uiDialog::Setup(
             uiStrings::phrManage( uiStrings::sRandomLine(mPlural)),mNoDlgTitle,
                 mODHelpKey(mRandomLineManHelpID) )
                 .nrstatusflds(1).modal(false),
		   RandomLineSetTranslatorGroup::ioContext())
{
    createDefaultUI();
    mTriggerInstanceCreatedNotifier();
    selChg( this );
}


uiRandomLineMan::~uiRandomLineMan()
{
}


bool uiRandomLineMan::gtItemInfo( const IOObj& ioobj, uiPhraseSet& inf ) const
{
    // should at least check existence
    mImplTODOGtItemInfo();
}
