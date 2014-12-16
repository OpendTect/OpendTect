/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Henrique Mageste
 Date:		November 2014
 RCS:		$Id$
________________________________________________________________________

-*/

static const char* rcsID mUsedVar = "$Id$";

#include "uibasemapoutlineitem.h"

#include "basemapoutline.h"

#include "uimenu.h"
#include "uistrings.h"
#include "seistrctr.h"

uiBasemapOutlineGroup::uiBasemapOutlineGroup( uiParent* p, bool isadd )
    : uiBasemapIOObjGroup(p,*Seis::getIOObjContext(Seis::Vol,true), isadd)
{
    addNameField();
}


uiBasemapOutlineGroup::~uiBasemapOutlineGroup()
{
}


bool uiBasemapOutlineGroup::acceptOK()
{
    const bool res = uiBasemapIOObjGroup::acceptOK();
    return res;
}


bool uiBasemapOutlineGroup::fillPar(IOPar& par) const
{
    const bool res = uiBasemapIOObjGroup::fillPar( par );
    return res;
}


bool uiBasemapOutlineGroup::usePar(const IOPar& par)
{
    const bool res = uiBasemapIOObjGroup::usePar( par );
    return res;
}



// uiBasemapOutlineTreeItem
uiBasemapOutlineTreeItem::uiBasemapOutlineTreeItem( const char* nm )
    : uiBasemapTreeItem(nm)
{
}


uiBasemapOutlineTreeItem::~uiBasemapOutlineTreeItem()
{
}


bool uiBasemapOutlineTreeItem::usePar( const IOPar& par )
{
    uiBasemapTreeItem::usePar( par );

// probably this will be used to set the outline line
//    BufferString lsstr;
//    par.get( sKeyLS(), lsstr );
//    LineStyle ls;
//    ls.fromString( lsstr );

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
	    mDynamicCastGet(Basemap::OutlineObject*,obj,basemapobjs_[idx])
	    if ( obj ) obj->setMultiID( mid );
	}
	else
	{
	    Basemap::OutlineObject* obj =
		new Basemap::OutlineObject( mid );
	    addBasemapObject( *obj );
	    obj->updateGeometry();
	}
    }

    return true;
}


bool uiBasemapOutlineTreeItem::showSubMenu()
{
    uiMenu mnu( getUiParent(), uiStrings::sAction() );
    mnu.insertItem( new uiAction(uiStrings::sEdit(false)), 0 );
    const int mnuid = mnu.exec();
    return handleSubMenu( mnuid );
}


bool uiBasemapOutlineTreeItem::handleSubMenu( int mnuid )
{
    if ( mnuid==0 )
	BMM().edit( getFamilyID(), ID() );
    else
	return false;

    return true;
}



// uiBasemapOutlineItem
const char* uiBasemapOutlineItem::iconName() const
{ return "cube_z"; }

uiBasemapGroup* uiBasemapOutlineItem::createGroup( uiParent* p, bool isadd )
{ return new uiBasemapOutlineGroup( p, isadd ); }

uiBasemapTreeItem* uiBasemapOutlineItem::createTreeItem( const char* nm )
{ return new uiBasemapOutlineTreeItem( nm ); }
