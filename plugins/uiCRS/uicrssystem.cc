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

static StringView sKeyEPSG()	{ return StringView("EPSG"); }

static AuthorityCode cDefProjID()
{ return AuthorityCode(sKeyEPSG(),32631); }

uiProjectionBasedSystem::uiProjectionBasedSystem( uiParent* p )
    : uiCoordSystem( p,sFactoryDisplayName() )
    , crsinfolist_(getCRSInfoList(true))
{
    dispidxs_.setSize( crsinfolist_.size(), 0 );
    dispidxs_.fillWithIncreasingValues();
    curselidx_ = crsinfolist_.indexOf( cDefProjID() );
    createGUI();
    fillList();
}


uiProjectionBasedSystem::uiProjectionBasedSystem( uiParent* p, bool orthogonal )
    : uiCoordSystem( p,sFactoryDisplayName() )
    , crsinfolist_(getCRSInfoList(orthogonal))
{
    orthogonal_ = orthogonal;
    dispidxs_.setSize( crsinfolist_.size(), 0 );
    dispidxs_.fillWithIncreasingValues();
    curselidx_ = crsinfolist_.indexOf( cDefProjID() );
    createGUI();
    fillList();
}


uiProjectionBasedSystem::~uiProjectionBasedSystem()
{
}


void uiProjectionBasedSystem::createGUI()
{
    tablemodel_ = new CRSInfoTableModel( crsinfolist_ );
    projtable_ = new uiTableView( this, "ProjectionTable" );
    projtable_->setModel( tablemodel_ );
    projtable_->setSelectionBehavior( uiTableView::SelectRows );
    projtable_->setSelectionMode( uiTableView::SingleSelection );
    projtable_->setHeaderVisible( OD::Vertical, false );
    projtable_->setSortingEnabled( true );
    projtable_->sortByColumn( 0, true );
    projtable_->setHSzPol( uiObject::WideMax );
    projtable_->setStretch( 2, 2 );
    projtable_->resizeColumnToContents( 0 );
    projtable_->resizeColumnToContents( 1 );
    projtable_->setColumnStretchable( 2, true );
    projtable_->setColumnStretchable( 3, true );
    projtable_->setColumnStretchable( 4, true );
    projtable_->selectionChanged.notify(
				mCB(this,uiProjectionBasedSystem,selChgCB) );

    uiStringSet filteroptions;
    filteroptions.add( tr("Area of use") ).add( tr("Authority code") )
		 .add( tr("Authority name") ).add( uiStrings::sName() )
		 .add( tr("Projection method") );
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

    setHAlignObj( projtable_ );
}


bool uiProjectionBasedSystem::initFields( const Coords::CoordSystem* sys )
{
    mDynamicCastGet( const Coords::ProjectionBasedSystem*,projsys,sys)
    if ( !projsys || !projsys->isOK() )
	return false;

    Coords::AuthorityCode pid = orthogonal_ ?
				projsys->getProjection()->authCode() :
				projsys->getProjection()->getGeodeticAuthCode();
    curselidx_ = crsinfolist_.indexOf( pid );
    setCurrent();
    return true;
}


void uiProjectionBasedSystem::searchCB( CallBacker* )
{
    BufferString str = searchfld_->text();
    if ( str.isEmpty() ) // No Filter, display all.
    {
	dispidxs_.erase();
	dispidxs_.setCapacity( crsinfolist_.size(), true );
	for ( int idx=0; idx<crsinfolist_.size(); idx++ )
	    dispidxs_.add( idx );

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
    TypeSet<int> showrows, hiderows;
    for ( int idx=0; idx<crsinfolist_.size(); idx++ )
    {
	if ( dispidxs_.isPresent(idx) )
	    showrows.add( idx );
	else
	    hiderows.add( idx );
    }

    projtable_->setRowsVisibility( showrows, hiderows );
    setCurrent();
}


void uiProjectionBasedSystem::setCurrent()
{
    if ( curselidx_ >= 0 )
    {
	const RowCol rc = projtable_->mapFromSource( RowCol(curselidx_,0) );
	projtable_->selectRow( rc.row() );
    }
}


bool uiProjectionBasedSystem::acceptOK()
{
    TypeSet<int> selrows;
    if ( !projtable_->getSelectedRows(selrows) || selrows.isEmpty() )
	return false;

    const RowCol srcrc = projtable_->mapToSource( RowCol(selrows[0],0) );
    const int srcrow = srcrc.row();
    if ( srcrow < 0 || srcrow >= crsinfolist_.size() )
	return false;

    const AuthorityCode pid( crsinfolist_.authName(srcrow),
			     crsinfolist_.authCode(srcrow) );
    RefMan<ProjectionBasedSystem> res = new ProjectionBasedSystem;
    res->setProjection( pid );
    outputsystem_ = res;
    return true;
}


void uiProjectionBasedSystem::selChgCB( CallBacker* )
{
}


uiCoordSystem* uiGeodeticCoordSystem::getCRSGeodeticFld( uiParent* p )
{
    return new uiGeodeticCoordSystem( p );
}

