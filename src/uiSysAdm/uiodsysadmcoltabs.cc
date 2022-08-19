/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiodsysadmcoltabs.h"

#include "uibutton.h"
#include "uicoltabimport.h"
#include "uifileinput.h"
#include "uilabel.h"
#include "uilistbox.h"
#include "uimsg.h"

#include "coltabsequence.h"
#include "oddirs.h"
#include "filepath.h"
#include "iopar.h"
#include "keystrs.h"
#include "settings.h"


uiODSysAdmColorTabs::uiODSysAdmColorTabs( uiParent* p )
    : uiDialog(p,uiDialog::Setup(
         uiStrings::phrManage( uiStrings::sColorTable(mPlural)),
	 uiString::emptyString(),mNoHelpKey))
{
    listfld = new uiListBox( this, "Color tables" );
    fillList( true );

    uiPushButton* addbut = new uiPushButton( this, uiStrings::sAdd(), false );
    addbut->activated.notify( mCB(this,uiODSysAdmColorTabs,addPush) );
    addbut->attach( alignedBelow, listfld );

    uiPushButton* rmbut = new uiPushButton( this, uiStrings::sRemove(),
                                            true );
    rmbut->activated.notify( mCB(this,uiODSysAdmColorTabs,rmPush) );
    rmbut->attach( rightOf, addbut );
}


uiODSysAdmColorTabs::~uiODSysAdmColorTabs()
{
}


void uiODSysAdmColorTabs::fillList( bool setcur )
{
    listfld->setEmpty();
    BufferStringSet seqnames;
    ColTab::SM().getSequenceNames( seqnames );
    seqnames.sort();
    listfld->addItems( seqnames );

    if ( setcur && listfld->size() )
	listfld->setCurrentItem( 0 );
}


void uiODSysAdmColorTabs::addPush( CallBacker* )
{
    const int sz = listfld->size();
    uiColTabImport dlg( this );
    if ( dlg.go() )
	rebuildList( sz );
}


void uiODSysAdmColorTabs::rmPush( CallBacker* )
{
    const int curidx = listfld->currentItem();
    if ( curidx < 0 ) return;
    ColTab::SM().remove( curidx );
    rebuildList( curidx );
}


void uiODSysAdmColorTabs::rebuildList( int curidx )
{
    fillList( false );

    if ( curidx >= ColTab::SM().size() )
	curidx = ColTab::SM().size() -1;
    if ( curidx >= 0 )
	listfld->setCurrentItem( curidx );
}


bool uiODSysAdmColorTabs::acceptOK( CallBacker* )
{
    if ( !ColTab::SM().write(true) )
    {
	uiMSG().error( tr("Cannot write new color table defs to file") );
	return false;
    }

    return true;
}
