/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:          September 2003
________________________________________________________________________

-*/

#include "uirandomlineman.h"

#include "ctxtioobj.h"
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


void uiRandomLineMan::mkFileInfo()
{
    if ( !curioobj_ ) { setInfo( "" ); return; }

    setInfo( getFileInfo() );
}
