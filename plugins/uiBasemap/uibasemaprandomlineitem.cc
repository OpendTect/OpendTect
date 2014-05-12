/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nanne Hemstra
 Date:		January 2014
 RCS:		$Id$
________________________________________________________________________

-*/

#include "uibasemaprandomlineitem.h"

#include "uiaction.h"
#include "uiioobjsel.h"
#include "uimenu.h"
#include "uimsg.h"
#include "uiodapplmgr.h"
#include "uistrings.h"

#include "basemaprandomline.h"
#include "oduicommon.h"
#include "randomlinetr.h"


// uiBasemapRandomLineGroup
uiBasemapRandomLineGroup::uiBasemapRandomLineGroup( uiParent* p )
    : uiBasemapGroup(p)
{
    rdlsfld_ = new uiIOObjSelGrp( this, mIOObjContext(RandomLineSet),
	uiIOObjSelGrp::Setup(OD::ChooseAtLeastOne) );

    addNameField( rdlsfld_->attachObj() );
}


uiBasemapRandomLineGroup::~uiBasemapRandomLineGroup()
{
}


bool uiBasemapRandomLineGroup::acceptOK()
{
    const int nrobjs = rdlsfld_->nrChosen();
    if ( nrobjs == 0 )
    {
	uiMSG().error( "Select at least one random line" );
	return false;
    }

    const bool res = uiBasemapGroup::acceptOK();
    return res;
}


bool uiBasemapRandomLineGroup::fillPar( IOPar& par ) const
{
    const bool res = uiBasemapGroup::fillPar( par );
    TypeSet<MultiID> mids;
    rdlsfld_->getChosen( mids );
    const int nrobjs = mids.size();
    par.set( sKeyNrObjs(), nrobjs );
    for ( int idx=0; idx<nrobjs; idx++ )
	par.set( IOPar::compKey(sKey::ID(),idx), mids[idx] );

    return res;
}


bool uiBasemapRandomLineGroup::usePar( const IOPar& par )
{
    const bool res = uiBasemapGroup::usePar( par );
    int nrobjs = 0;
    par.get( sKeyNrObjs(), nrobjs );
    TypeSet<MultiID> mids( nrobjs, MultiID::udf() );
    for ( int idx=0; idx<nrobjs; idx++ )
	par.get( IOPar::compKey(sKey::ID(),idx), mids[idx] );

    rdlsfld_->setChosen( mids );
    return res;
}


// uiBasemapRandomLineItem
const char* uiBasemapRandomLineItem::iconName() const
{ return "randomline"; }

uiBasemapGroup* uiBasemapRandomLineItem::createGroup( uiParent* p )
{ return new uiBasemapRandomLineGroup( p ); }

uiBasemapTreeItem* uiBasemapRandomLineItem::createTreeItem( const char* nm )
{ return new uiBasemapRandomLineTreeItem( nm ); }



// uiBasemapRandomLineTreeItem
uiBasemapRandomLineTreeItem::uiBasemapRandomLineTreeItem( const char* nm )
    : uiBasemapTreeItem(nm)
{}


uiBasemapRandomLineTreeItem::~uiBasemapRandomLineTreeItem()
{}


bool uiBasemapRandomLineTreeItem::usePar( const IOPar& par )
{
    uiBasemapTreeItem::usePar( par );

    int nrobjs = 0;
    par.get( uiBasemapGroup::sKeyNrObjs(), nrobjs );

    while ( nrobjs < basemapobjs_.size() )
	delete removeBasemapObject( *basemapobjs_[0] );

    for ( int idx=0; idx<nrobjs; idx++ )
    {
	MultiID mid;
	if ( !par.get(IOPar::compKey(sKey::ID(),idx),mid) )
	    continue;

	if ( basemapobjs_.validIdx(idx) )
	{
	    mDynamicCastGet(Basemap::RandomLineObject*,obj,basemapobjs_[idx])
	    if ( obj ) obj->setMultiID( mid );
	}
	else
	{
	    Basemap::RandomLineObject* obj =
		new Basemap::RandomLineObject( mid );
	    addBasemapObject( *obj );
	    obj->updateGeometry();
	}
    }

    return true;
}


bool uiBasemapRandomLineTreeItem::showSubMenu()
{
    uiMenu mnu( getUiParent(), "Action" );
    mnu.insertItem( new uiAction(uiStrings::sEdit(false)), 0 );
    const int mnuid = mnu.exec();
    return handleSubMenu( mnuid );
}


bool uiBasemapRandomLineTreeItem::handleSubMenu( int mnuid )
{
    if ( mnuid==0 )
    {
	BMM().edit( getFamilyID(), ID() );
    }
    else
	return false;

    return true;
}

