/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Bril
 Date:          Jul 2006
________________________________________________________________________

-*/
static const char* rcsID mUnusedVar = "$Id$";

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
    : uiDialog(p,uiDialog::Setup("Manage color bars",0,mNoHelpID))
{
    listfld = new uiListBox( this, "Color tables" );
    fillList( true );
    
    uiPushButton* addbut = new uiPushButton( this, "Add", false );
    addbut->activated.notify( mCB(this,uiODSysAdmColorTabs,addPush) );
    addbut->attach( alignedBelow, listfld );

    uiPushButton* rmbut = new uiPushButton( this, "Remove", true );
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
	uiMSG().error( "Cannot write new color table defs to file" );
	return false;
    }

    return true;
}
