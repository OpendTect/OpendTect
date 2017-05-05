/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Raman
 Date:          May 2017
________________________________________________________________________

-*/

#include "uicrssystem.h"

#include "uilistbox.h"
#include "uilineedit.h"
#include "uimsg.h"
#include "uistring.h"
#include "uitoolbutton.h"
#include "od_helpids.h"

using namespace Coords;


uiProjectionBasedSystem::uiProjectionBasedSystem( uiParent* p )
    : uiPositionSystem( p,sFactoryDisplayName() )
    , curselidx_(-1)
{
    uiListBox::Setup su( OD::ChooseOnlyOne, tr("Select projection") );
    projselfld_ = new uiListBox( this, su, "ProjectionList" );

    uiButton* searchbut = new uiToolButton( projselfld_, "search", tr("Search"),
	    			mCB(this,uiProjectionBasedSystem,searchCB) );
    searchbut->attach( rightAlignedAbove, projselfld_->box() );

    searchfld_ = new uiLineEdit( projselfld_, "Search" );
    searchfld_->setPlaceholderText( tr("ID or name") );
    searchfld_->attach( leftOf, searchbut );
    searchfld_->editingFinished.notify(
	    			mCB(this,uiProjectionBasedSystem,searchCB) );

    setHAlignObj( projselfld_ );
    fetchList();
    fillList();
}


bool uiProjectionBasedSystem::initFields( const Coords::PositionSystem* sys )
{
    mDynamicCastGet( const Coords::ProjectionBasedSystem*, from, sys );
    if ( !from || !from->isOK() )
	return false;

    Coords::ProjectionID pid = from->getProjection()->id();
    curselidx_ = ids_.indexOf( pid );
    setCurrent();
    return true;
}


void uiProjectionBasedSystem::searchCB( CallBacker* )
{
    const BufferString str = searchfld_->text();
    if ( str.isEmpty() ) // No Filter, display all.
    {
	dispidxs_.erase();
	dispidxs_.setCapacity( ids_.size(), true );
	for ( int idx=0; idx<ids_.size(); idx++ )
	    dispidxs_.add( idx );

	fillList();
	return;
    }

    if ( str.size() < 3 ) return; // Not enough to search for.

    MouseCursorChanger mcch( MouseCursor::Wait );
    if ( str.isNumber(true) ) // Search for ID
    {
	Coords::ProjectionID searchid = Coords::ProjectionID::get( str.toInt());
	const int selidx = ids_.indexOf( searchid );
	if ( selidx < 0 )
	{
	    uiMSG().message( tr("Projection ID %1 was not found")
		    		.arg(searchid.getI()) );
	    return;
	}

	dispidxs_.erase();
	dispidxs_ += selidx;
    }
    else // Search for Name
    {
	dispidxs_.erase();
	BufferString gestr = str;
	if ( !str.find('*') )
	{ gestr = '*'; gestr += str; gestr += '*'; }

	for ( int idx=0; idx<names_.size(); idx++ )
	{
	    if ( names_.get(idx).matches(gestr,CaseInsensitive) )
		dispidxs_.add( idx );
	}
    }

    fillList();
}


void uiProjectionBasedSystem::fetchList()
{
    Projection::getAll( ids_, names_, true );
    int* idxs = names_.getSortIndexes();
    names_.useIndexes( idxs );
    TypeSet<Coords::ProjectionID> tmp( ids_ );
    tmp.getReOrdered( idxs, ids_ );
    delete [] idxs;
    dispidxs_.setCapacity( ids_.size(), true );
    for ( int idx=0; idx<ids_.size(); idx++ )
	dispidxs_.add( idx );
}


void uiProjectionBasedSystem::fillList()
{
    projselfld_->setEmpty();
    uiStringSet itemstodisplay;
    for ( int idx=0; idx<dispidxs_.size(); idx++ )
    {
	const int index = dispidxs_[idx];
	uiString itmtxt = toUiString("[%1] %2" ).arg(ids_[index].getI())
	    			.arg(names_.get(index));
	itemstodisplay.add( itmtxt );
    }
    
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
    if ( !ids_.validIdx(selidx) )
	return false;

    const ProjectionID pid = ids_[selidx];
    RefMan<ProjectionBasedSystem> res = new ProjectionBasedSystem();
    res->setProjection( pid );
    outputsystem_ = res;
    return true;
}
