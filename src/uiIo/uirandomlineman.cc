/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
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
