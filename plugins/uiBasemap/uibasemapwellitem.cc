/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nanne Hemstra
 Date:		January 2014
 RCS:		$Id$
________________________________________________________________________

-*/

#include "uibasemapwellitem.h"

#include "uiaction.h"
#include "uiioobjsel.h"
#include "uimenu.h"
#include "uimsg.h"
#include "uiodapplmgr.h"
#include "uistrings.h"

#include "basemapwell.h"
#include "oduicommon.h"
#include "welltransl.h"


// uiBasemapWellGroup
const char* uiBasemapWellGroup::sKeyNrWells()	{ return "Nr Wells"; }

uiBasemapWellGroup::uiBasemapWellGroup( uiParent* p )
    : uiBasemapGroup(p)
{
    wellsfld_ = new uiIOObjSelGrp( this, mIOObjContext(Well),
	uiIOObjSelGrp::Setup(OD::ChooseAtLeastOne) );

    addNameField( wellsfld_->attachObj() );
}


uiBasemapWellGroup::~uiBasemapWellGroup()
{
}


bool uiBasemapWellGroup::acceptOK()
{
    const int nrwells = wellsfld_->nrChosen();
    if ( nrwells == 0 )
    {
	uiMSG().error( "Select at least one well" );
	return false;
    }

    const bool res = uiBasemapGroup::acceptOK();
    return res;
}


bool uiBasemapWellGroup::fillPar( IOPar& par ) const
{
    const bool res = uiBasemapGroup::fillPar( par );
    TypeSet<MultiID> mids;
    wellsfld_->getChosen( mids );
    const int nrwells = mids.size();
    par.set( sKeyNrWells(), nrwells );
    for ( int idx=0; idx<nrwells; idx++ )
	par.set( IOPar::compKey(sKey::ID(),idx), mids[idx] );

    return res;
}


bool uiBasemapWellGroup::usePar( const IOPar& par )
{
    const bool res = uiBasemapGroup::usePar( par );
    int nrwells = 0;
    par.get( uiBasemapWellGroup::sKeyNrWells(), nrwells );
    TypeSet<MultiID> mids( nrwells, MultiID::udf() );
    for ( int idx=0; idx<nrwells; idx++ )
	par.get( IOPar::compKey(sKey::ID(),idx), mids[idx] );

    wellsfld_->setChosen( mids );
    return res;
}


// uiBasemapWellItem
const char* uiBasemapWellItem::iconName() const
{ return "well"; }

uiBasemapGroup* uiBasemapWellItem::createGroup( uiParent* p )
{ return new uiBasemapWellGroup( p ); }

uiBasemapTreeItem* uiBasemapWellItem::createTreeItem( const char* nm )
{ return new uiBasemapWellTreeItem( nm ); }



// uiBasemapWellTreeItem
uiBasemapWellTreeItem::uiBasemapWellTreeItem( const char* nm )
    : uiBasemapTreeItem(nm)
{}


uiBasemapWellTreeItem::~uiBasemapWellTreeItem()
{}


bool uiBasemapWellTreeItem::usePar( const IOPar& par )
{
    int nrwells = 0;
    par.get( uiBasemapWellGroup::sKeyNrWells(), nrwells );
    for ( int idx=0; idx<nrwells; idx++ )
    {
	MultiID mid;
	if ( !par.get(IOPar::compKey(sKey::ID(),idx),mid) )
	    continue;

	Basemap::WellObject* obj = new Basemap::WellObject( mid );
	addBasemapObject( *obj );
	obj->updateGeometry();
    }

    pars_ = par;
    return true;
}


bool uiBasemapWellTreeItem::showSubMenu()
{
    uiMenu mnu( getUiParent(), "Action" );
    mnu.insertItem( new uiAction(uiStrings::sEdit(false)), 0 );
    const int mnuid = mnu.exec();
    return handleSubMenu( mnuid );
}


bool uiBasemapWellTreeItem::handleSubMenu( int mnuid )
{
    if ( mnuid==0 )
    {
	BMM().edit( factoryKeyword(), name() );
    }
    else
	return false;

    return true;
}

