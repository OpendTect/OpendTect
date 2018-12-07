/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          July 2018
________________________________________________________________________

-*/

#include "uistorcollbuilder.h"

#include "uitable.h"
#include "uibuttongroup.h"
#include "uitoolbutton.h"

#include "dbman.h"
#include "ioobj.h"


void uiStorableCollectionBuilder::Setup::set( const IOObjContext& ctxt )
{
    PtrMan<IOObj> oneobj = DBM().getFirst( ctxt );
    canopen_ = oneobj;
}



uiStorableCollectionBuilder::uiStorableCollectionBuilder( uiParent* p,
						  const Setup& setup )
    : uiGroup(p,"Storable Collection Builder")
{
    const CallBack selcb( mCB(this,uiStorableCollectionBuilder,selChgCB) );

    tbl_ = new uiTable( this, uiTable::Setup(), "Collection Build Table" );
    tbl_->setPrefWidth( setup.pixwidth_ );
    tbl_->setSelectionMode( uiTable::SingleRow );
    tbl_->setTableReadOnly( true );
    tbl_->setNrRows( 0 );
    tbl_->setNrCols( setup.nrcols_ );
    tbl_->rowClicked.notify( selcb );
    tbl_->selectionChanged.notify( selcb );

    bgrp_ = new uiButtonGroup( this, "Operations buttons", OD::Vertical );
#   define mAddBut( butnm, cb, ic, phr ) \
    if ( setup.cb.willCall() ) \
	butnm = new uiToolButton( bgrp_, ic, \
			uiStrings::phr(setup.objtypename_), setup.cb )
    mAddBut( addbut_, addcb_, "create", phrAdd );
    mAddBut( edbut_, edcb_, "edit", phrEdit );
    mAddBut( rmbut_, rmcb_, "remove", phrRemove );
    mAddBut( openbut_, opencb_, "open", phrOpen );
    mAddBut( savebut_, savecb_, "save", phrSave );
    openbut_->setSensitive( setup.canopen_ );
    bgrp_->attach( rightOf, tbl_ );

    postFinalise().notify( selcb );
}


uiStorableCollectionBuilder::~uiStorableCollectionBuilder()
{
    detachAllNotifiers();
}


void uiStorableCollectionBuilder::selChgCB( CallBacker* )
{
    updLooks();
}


void uiStorableCollectionBuilder::displayButtons( bool yn )
{
    bgrp_->display( yn );
}


void uiStorableCollectionBuilder::updLooks()
{
    const int nrrows = tbl_->nrRows();
    const bool haverows = nrrows > 0;

    if ( haverows && tbl_->currentRow() < 0 )
	tbl_->setCurrentCell( RowCol(0,0) );

    rmbut_->setSensitive( haverows );
    edbut_->setSensitive( haverows );
    savebut_->setSensitive( haverows );
}


void uiStorableCollectionBuilder::setCanOpen( bool yn )
{
    openbut_->setSensitive( yn );
}


bool uiStorableCollectionBuilder::canOpen() const
{
    return openbut_->isSensitive();
}
