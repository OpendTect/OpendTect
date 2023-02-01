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
#include "uilistbox.h"
#include "uimsg.h"
#include "uistring.h"
#include "uitextedit.h"
#include "uitoolbutton.h"


using namespace Coords;

static StringView sKeyEPSG()	{ return StringView("EPSG"); }

static AuthorityCode cDefProjID()
{ return AuthorityCode(sKeyEPSG(),32631); }

uiProjectionBasedSystem::uiProjectionBasedSystem( uiParent* p )
    : uiCoordSystem( p,sFactoryDisplayName() )
{
    createGUI();
    fetchList();
    fillList();

}


uiProjectionBasedSystem::uiProjectionBasedSystem( uiParent* p, bool orthogonal )
    : uiCoordSystem( p,sFactoryDisplayName() )
{
    orthogonal_ = orthogonal;
    createGUI();
    fetchList();
    fillList();
}


uiProjectionBasedSystem::~uiProjectionBasedSystem()
{
}


void uiProjectionBasedSystem::createGUI()
{
    uiListBox::Setup su( OD::ChooseOnlyOne, tr("Select projection") );
    projselfld_ = new uiListBox( this, su, "ProjectionList" );
    projselfld_->setHSzPol( uiObject::WideVar );
    projselfld_->setNrLines( 10 );
    projselfld_->selectionChanged.notify(
				mCB(this,uiProjectionBasedSystem,selChgCB) );

    uiStringSet filteroptions;
    filteroptions.add( tr("Area of use") ).add( tr("Authority code") )
		 .add( tr("Authority name") ).add( uiStrings::sName() )
		 .add( tr("Projection method") );
    filtersel_ = new uiLabeledComboBox( this, filteroptions, tr("Filter by") );
    filtersel_->box()->setCurrentItem( 3 );
    filtersel_->attach( alignedAbove, projselfld_ );
    filtersel_->setStretch( 0, 0 );

    searchfld_ = new uiLineEdit( this, "Search" );
    searchfld_->setPlaceholderText( tr("ID or name") );
    searchfld_->attach( rightOf, filtersel_ );
    searchfld_->returnPressed.notify(
				mCB(this,uiProjectionBasedSystem,searchCB) );

    uiButton* searchbut = new uiToolButton( this, "search", tr("Search"),
				mCB(this,uiProjectionBasedSystem,searchCB) );
    searchbut->attach( rightOf, searchfld_ );

    uiToolButton* infobut = new uiToolButton( projselfld_, "info",
		tr("View details"), mCB(this,uiProjectionBasedSystem,infoCB) );
    infobut->attach( rightTo, projselfld_->box() );
    infobut->attach( rightBorder );

    setHAlignObj( projselfld_ );
}


bool uiProjectionBasedSystem::initFields( const Coords::CoordSystem* sys )
{
    mDynamicCastGet( const Coords::ProjectionBasedSystem*,projsys,sys)
    if ( !projsys || !projsys->isOK() )
	return false;

    Coords::AuthorityCode pid = orthogonal_ ?
				projsys->getProjection()->authCode() :
				projsys->getProjection()->getGeodeticAuthCode();
    curselidx_ = crsinfolist_->indexOf( pid );
    setCurrent();
    return true;
}


void uiProjectionBasedSystem::searchCB( CallBacker* )
{
    BufferString str = searchfld_->text();
    if ( str.isEmpty() ) // No Filter, display all.
    {
	dispidxs_.erase();
	dispidxs_.setCapacity( crsinfolist_->size(), true );
	for ( int idx=0; idx<crsinfolist_->size(); idx++ )
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
    for ( int idx=0; idx<crsinfolist_->size(); idx++ )
    {
	BufferString crsattr;
	switch ( filtertype )
	{
	    case 0: crsattr = crsinfolist_->areaName( idx ); break;
	    case 1: crsattr = crsinfolist_->authCode( idx ); break;
	    case 2: crsattr = crsinfolist_->authName( idx ); break;
	    case 3: crsattr = crsinfolist_->name( idx ); break;
	    case 4: crsattr = crsinfolist_->projMethod( idx ); break;
	}

	if ( crsattr.matches(gestr,OD::CaseInsensitive) )
	    dispidxs_.add( idx );
    }

    fillList();
}


void uiProjectionBasedSystem::fetchList()
{
    crsinfolist_ = getCRSInfoList( orthogonal_ );
    curselidx_ = crsinfolist_->indexOf( cDefProjID() );
    dispidxs_.setCapacity( crsinfolist_->size(), true ); \
    for ( int idx=0; idx<crsinfolist_->size(); idx++ ) \
	dispidxs_.add( idx );
}


void uiProjectionBasedSystem::fillList()
{
    MouseCursorChanger cursorchanger( MouseCursor::Wait );
    projselfld_->setEmpty();
    uiStringSet itemstodisplay;
    for ( int idx=0; idx<dispidxs_.size(); idx++ )
	itemstodisplay.add( crsinfolist_->getDispString(dispidxs_[idx]) );

    projselfld_->addItems( itemstodisplay );
    setCurrent();
}


void uiProjectionBasedSystem::setCurrent()
{
    const int selidx = curselidx_ < 0 ? -1 : dispidxs_.indexOf( curselidx_ );
    projselfld_->setCurrentItem( selidx );
}


bool uiProjectionBasedSystem::acceptOK()
{
    const int selidx = projselfld_->currentItem();
    if ( !dispidxs_.validIdx(selidx) )
	return false;

    const AuthorityCode pid( crsinfolist_->authName(dispidxs_[selidx]),
			     crsinfolist_->authCode(dispidxs_[selidx]) );
    RefMan<ProjectionBasedSystem> res = new ProjectionBasedSystem;
    res->setProjection( pid );
    outputsystem_ = res;
    return true;
}


void uiProjectionBasedSystem::selChgCB( CallBacker* )
{
}


void uiProjectionBasedSystem::infoCB( CallBacker* )
{
    const int selidx = projselfld_->currentItem();
    if ( !dispidxs_.validIdx(selidx) )
	return;

    const uiString nm = crsinfolist_->getDispString( dispidxs_[selidx] );
    uiDialog infodlg( this, uiDialog::Setup(nm,mNoDlgTitle,mNoHelpKey) );
    infodlg.setCtrlStyle( uiDialog::CloseOnly );
    auto* txtfld = new uiTextEdit( &infodlg );
    txtfld->setText( crsinfolist_->getDescString(dispidxs_[selidx]) );
    txtfld->setPrefHeightInChar( 10 );
    infodlg.go();
}


uiCoordSystem* uiGeodeticCoordSystem::getCRSGeodeticFld( uiParent* p )
{
    return new uiGeodeticCoordSystem( p );
}

