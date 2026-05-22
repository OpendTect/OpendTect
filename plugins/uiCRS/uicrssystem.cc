/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uicrssystem.h"

#include "mousecursor.h"
#include "od_helpids.h"
#include "od_iostream.h"
#include "oddirs.h"
#include "survinfo.h"
#include "trckeyzsampling.h"

#include "uicombobox.h"
#include "uifileinput.h"
#include "uigeninput.h"
#include "uilabel.h"
#include "uilatlonginp.h"
#include "uilineedit.h"
#include "uimsg.h"
#include "uistring.h"
#include "uitableview.h"
#include "uitextedit.h"
#include "uitoolbutton.h"

using namespace Coords;

#define cNameCol  2
#define cMethodCol  3
#define cAreaCol  4
#define cRowHeight  18

uiProjectionBasedSystem::uiProjectionBasedSystem( uiParent* p )
    : uiCoordSystem( p,sFactoryDisplayName() )
    , crsinfolist_(getCRSInfoList(true))
{
    dispidxs_.setSize( crsinfolist_.size(), 0 );
    dispidxs_.fillWithIncreasingValues();
    createGUI();
}


uiProjectionBasedSystem::uiProjectionBasedSystem( uiParent* p, bool orthogonal )
    : uiCoordSystem( p,sFactoryDisplayName() )
    , crsinfolist_(getCRSInfoList(orthogonal))
{
    orthogonal_ = orthogonal;
    dispidxs_.setSize( crsinfolist_.size(), 0 );
    dispidxs_.fillWithIncreasingValues();
    createGUI();
}


uiProjectionBasedSystem::~uiProjectionBasedSystem()
{
}


void uiProjectionBasedSystem::createGUI()
{
    tablemodel_ = new CRSInfoTableModel( crsinfolist_ );
    projtable_ = new uiTableView( this, "ProjectionTable" );
    projtable_->setModel( tablemodel_ );
    projtable_->setRowHeight( cRowHeight );
    projtable_->setSelectionBehavior( uiTableView::SelectRows );
    projtable_->setSelectionMode( uiTableView::SingleSelection );
    projtable_->setHeaderVisible( OD::Vertical, false );
    projtable_->setSortingEnabled( true );
    projtable_->sortByColumn( 0, true );
    projtable_->setNrFrozenColumns( 0 );
    projtable_->setHSzPol( uiObject::WideMax );
    projtable_->setStretch( 2, 2 );
    for ( int col=0; col<=cMethodCol; col++ )
	projtable_->resizeColumnToContents( col );

    projtable_->selectionChanged.notify(
				mCB(this,uiProjectionBasedSystem,selChgCB) );

    uiStringSet filteroptions;
    filteroptions.add( tr("Area of use") ).add( tr("Authority code") )
		 .add( tr("Authority name") ).add( uiStrings::sName() );
    if ( orthogonal_ )
	filteroptions.add( tr("Projection method") );

    filtersel_ = new uiLabeledComboBox( this, filteroptions, tr("Filter by") );
    filtersel_->box()->setCurrentItem( 3 );
    filtersel_->attach( alignedAbove, projtable_ );
    filtersel_->setStretch( 0, 0 );

    searchfld_ = new uiLineEdit( this, "Search" );
    searchfld_->setPlaceholderText( tr("ID or name") );
    searchfld_->setStretch( 2, 0 );
    searchfld_->attach( rightOf, filtersel_ );
    searchfld_->returnPressed.notify(
				mCB(this,uiProjectionBasedSystem,searchCB) );

    uiButton* searchbut = new uiToolButton( this, "search", tr("Search"),
				mCB(this,uiProjectionBasedSystem,searchCB) );
    searchbut->attach( rightOf, searchfld_ );

    if ( orthogonal_ )
    {
	showmethodfld_ = new uiCheckBox( this, tr("Show Projection Method"),
			    mCB(this,uiProjectionBasedSystem,showColumnsCB) );
	showmethodfld_->attach( alignedBelow, projtable_ );
    }

    showareafld_ = new uiCheckBox( this, tr("Show Area of use"),
			mCB(this,uiProjectionBasedSystem,showColumnsCB) );
    if ( showmethodfld_ )
	showareafld_->attach( rightTo, showmethodfld_ );
    else
	showareafld_->attach( alignedBelow, projtable_ );

    gotocurrentbut_ = new uiPushButton( this, tr("Go to current"), "updown",
			mCB(this,uiProjectionBasedSystem,goToCurrentCB), true );
    gotocurrentbut_->attach( rightAlignedBelow, projtable_ );
    gotocurrentbut_->setSensitive( false );

    setHAlignObj( projtable_ );
    postFinalize().notify( mCB(this,uiProjectionBasedSystem,showColumnsCB) );
}


bool uiProjectionBasedSystem::initFields( const Coords::CoordSystem* sys )
{
    mDynamicCastGet( const Coords::ProjectionBasedSystem*,projsys,sys)
    if ( !projsys || !projsys->isOK() )
	return false;

    Coords::AuthorityCode pid = orthogonal_ ?
				projsys->getProjection()->authCode() :
				projsys->getProjection()->getGeodeticAuthCode();
    const int curidx = crsinfolist_.indexOf( pid );
    if ( curidx < 0 )
    {
	pErrMsg("Survey CRS not found");
	return false;
    }

    tablemodel_->setCurrentSelection( curidx );
    gotocurrentbut_->setSensitive( true );
    goToCurrentCB( nullptr );
    return true;
}


void uiProjectionBasedSystem::showColumnsCB( CallBacker* )
{
    const bool showmethod = showmethodfld_ ? showmethodfld_->isChecked()
					   : false;
    const bool showarea = showareafld_->isChecked();
    projtable_->setColumnHidden( cMethodCol, !showmethod );
    projtable_->setColumnHidden( cAreaCol, !showarea );
    projtable_->setColumnStretchable( cNameCol, !showmethod && !showarea );
    if ( showmethod || showarea )
    {
	projtable_->resizeColumnToContents( cNameCol );
	if ( showmethod )
	{
	    projtable_->setColumnStretchable( cMethodCol, !showarea );
	    if ( showarea )
		projtable_->resizeColumnToContents( cMethodCol );
	}

	if ( showarea )
	    projtable_->setColumnStretchable( cAreaCol, true );
    }
}



void uiProjectionBasedSystem::goToCurrentCB( CallBacker* )
{
    const int curidx = tablemodel_->currentSelection();
    if ( curidx < 0 )
	return;

    const RowCol sourcerc( curidx, 0 );
    projtable_->scrollTo( sourcerc );
    projtable_->selectRow( projtable_->mapFromSource(sourcerc).row() );
}


void uiProjectionBasedSystem::searchCB( CallBacker* )
{
    BufferString str = searchfld_->text();
    if ( str.isEmpty() ) // No Filter, display all.
    {
	dispidxs_.setEmpty();
	dispidxs_.setSize( crsinfolist_.size(), 0 );
	dispidxs_.fillWithIncreasingValues( 0 );
	fillList();
	return;
    }

    if ( str.size() < 2 ) return; // Not enough to search for.

    BufferString gestr = str;
    if ( !str.find('*') )
    {
	gestr = "*";
	gestr.add( str ).add( '*' );
    }

    MouseCursorChanger mcch( MouseCursor::Wait );
    dispidxs_.erase();
    const int filtertype = filtersel_->box()->currentItem();
    for ( int idx=0; idx<crsinfolist_.size(); idx++ )
    {
	BufferString crsattr;
	switch ( filtertype )
	{
	    case 0: crsattr = crsinfolist_.areaName( idx ); break;
	    case 1: crsattr = crsinfolist_.authCode( idx ); break;
	    case 2: crsattr = crsinfolist_.authName( idx ); break;
	    case 3: crsattr = crsinfolist_.name( idx ); break;
	    case 4: crsattr = crsinfolist_.projMethod( idx ); break;
	}

	if ( crsattr.matches(gestr,OD::CaseInsensitive) )
	    dispidxs_.add( idx );
    }

    fillList();
}


void uiProjectionBasedSystem::fillList()
{
    MouseCursorChanger cursorchanger( MouseCursor::Wait );
    const int curidx = tablemodel_->currentSelection();
    TypeSet<int> selrows;
    projtable_->getSelectedRows( selrows, true );
    const bool dispall = dispidxs_.size() == crsinfolist_.size();
    if ( dispall )
    {
	// Just display all the rows without any checks;
	TypeSet<int> nothing;
	projtable_->setRowsVisibility( dispidxs_, nothing );
	if ( !selrows.isEmpty() )
	{
	    const RowCol rc = projtable_->mapFromSource(
						RowCol(selrows.first(),0) );
	    projtable_->selectRow( rc.row() );
	}

	gotocurrentbut_->setSensitive( curidx >= 0 );
	return;
    }

    TypeSet<int> showrows, hiderows;
    for ( int idx=0; idx<crsinfolist_.size(); idx++ )
    {
	const int viewidx = projtable_->mapFromSource( RowCol(idx,0) ).row();
	if ( dispidxs_.isPresent(idx) )
	    showrows.add( viewidx );
	else
	    hiderows.add( viewidx );
    }

    projtable_->setRowsVisibility( showrows, hiderows );
    gotocurrentbut_->setSensitive( curidx>=0 && dispidxs_.isPresent(curidx) );
    if ( !selrows.isEmpty() && dispidxs_.isPresent(selrows.first()) )
    {
	const RowCol rc = projtable_->mapFromSource(
					    RowCol(selrows.first(),0) );
	projtable_->selectRow( rc.row() );
    }
}


bool uiProjectionBasedSystem::acceptOK()
{
    TypeSet<int> selrows;
    if ( !projtable_->getSelectedRows(selrows,true) || selrows.isEmpty() )
	return false;

    const int selrow = selrows.first();
    if ( selrow < 0 || selrow >= crsinfolist_.size() )
	return false;

    const AuthorityCode pid( crsinfolist_.authName(selrow),
			     crsinfolist_.authCode(selrow) );
    RefMan<ProjectionBasedSystem> res = new ProjectionBasedSystem;
    res->setProjection( pid );
    outputsystem_ = res;
    initFields( outputsystem_.ptr() );
    return true;
}


void uiProjectionBasedSystem::selChgCB( CallBacker* )
{
}


uiCoordSystem* uiGeodeticCoordSystem::getCRSGeodeticFld( uiParent* p )
{
    return new uiGeodeticCoordSystem( p );
}

