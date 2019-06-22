/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Jul 2006
________________________________________________________________________

-*/

#include "uiodsysadmcoltabs.h"

#include "uibutton.h"
#include "uicolseqimport.h"
#include "uifilesel.h"
#include "uilabel.h"
#include "uilistbox.h"
#include "uimsg.h"

#include "coltabseqmgr.h"
#include "oddirs.h"
#include "iopar.h"


uiODSysAdmColorTabs::uiODSysAdmColorTabs( uiParent* p )
    : uiDialog(p,uiDialog::Setup(uiStrings::phrManage(tr("color bars")),
		    mNoDlgTitle,mNoHelpKey))
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
    ColTab::SeqMGR().getSequenceNames( seqnames );
    seqnames.sort();
    listfld->addItems( seqnames );

    if ( setcur && listfld->size() )
	listfld->setCurrentItem( 0 );
}


void uiODSysAdmColorTabs::addPush( CallBacker* )
{
    const int sz = listfld->size();
    uiColSeqImport dlg( this );
    if ( dlg.go() )
	rebuildList( sz );
}


void uiODSysAdmColorTabs::rmPush( CallBacker* )
{
    const int curidx = listfld->currentItem();
    if ( curidx < 0 ) return;
    ColTab::SeqMGR4Edit().removeByName( listfld->itemText(curidx) );
    rebuildList( curidx );
}


void uiODSysAdmColorTabs::rebuildList( int curidx )
{
    fillList( false );

    if ( curidx >= ColTab::SeqMGR().size() )
	curidx = ColTab::SeqMGR().size() -1;
    if ( curidx >= 0 )
	listfld->setCurrentItem( curidx );
}


bool uiODSysAdmColorTabs::acceptOK()
{
    uiRetVal uirv = ColTab::SeqMGR().write(true);
    if ( uirv.isError() )
	{ uiMSG().error( uirv ); return false; }

    return true;
}
