/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Raman
 Date:          May 2017
________________________________________________________________________

-*/

#include "uicrssystem.h"

#include "uilistbox.h"
#include "uicombobox.h"
#include "uistring.h"
#include "od_helpids.h"

using namespace Coords;


uiProjectionBasedSystem::uiProjectionBasedSystem( uiParent* p )
    : uiPositionSystem( p,sFactoryDisplayName() )
{
    uiListBox::Setup su( OD::ChooseOnlyOne, tr("Select projection") );
    projselfld_ = new uiListBox( this, su, "ProjectionList" );

    uiStringSet optlist;
    optlist.add( uiStrings::sID() ).add( uiStrings::sName() );
//    sortselfld_ = new uiComboBox( this, optlist );
//    sortselfld_->attach( alignRightAbove, projselfld_ );
    setHAlignObj( projselfld_ );
    fetchList();
    fillList( true );
}


bool uiProjectionBasedSystem::initFields( const Coords::PositionSystem* sys )
{
    mDynamicCastGet( const Coords::ProjectionBasedSystem*, from, sys );
    if ( !from || !from->isOK() )
	return false;

    Coords::ProjectionID pid = from->getProjection()->id();
    setCurrent( pid );
    return true;
}


void uiProjectionBasedSystem::fetchList()
{
    Projection::getAll( ids_, names_, true );
}


void uiProjectionBasedSystem::fillList( bool sortbyid )
{
    projselfld_->setEmpty();
    //TODO:: Implement sorting
    for ( int idx=0; idx<ids_.size(); idx++ )
    {
	uiString itmtxt = toUiString("[%1] %2" ).arg(ids_[idx].getI())
	    			.arg(names_.get(idx));
	projselfld_->addItem( itmtxt );
    }
}


void uiProjectionBasedSystem::setCurrent( ProjectionID pid )
{
    const int idxof = ids_.indexOf( pid );
    if ( idxof >= 0 )
	projselfld_->setCurrentItem( idxof );
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
