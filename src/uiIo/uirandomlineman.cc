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
#include "randomlinegeom.h"
#include "geometry.h"


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
    ConstRefMan<Geometry::RandomLine> rl = Geometry::RLM().get( ioobj.key() );
    if ( !rl )
    { inf.add( uiStrings::sNoInfoAvailable() ); return false; }

    addObjInfo( inf, tr("Number of Nodes"), rl->nrNodes() );
    addObjInfo( inf, uiStrings::sZRange(), toUiString("%1-%2")
			.arg(rl->zRange().start).arg(rl->zRange().stop) );

    return true;
}
