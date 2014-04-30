/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nanne Hemstra
 Date:		January 2013
________________________________________________________________________

-*/

static const char* rcsID mUsedVar = "$Id: uibasemaptreeitem.cc 34190 2014-04-16 20:09:04Z nanne.hemstra@dgbes.com $";


#include "uibasemapitem.h"

#include "uibasemap.h"
#include "uigeninput.h"
#include "uimenu.h"
#include "uimsg.h"
#include "uiodapplmgr.h"
#include "uiodmain.h"
#include "uistrings.h"
#include "uitreeview.h"


mImplFactory( uiBasemapItem, uiBasemapItem::factory )

uiBasemapItem::uiBasemapItem()
    : treeitem_(0)
    , basemap_(0)
    , treetop_(0)
{
}


uiBasemapItem::~uiBasemapItem()
{
    if ( treeitem_ )
	treeitem_->checkStatusChange()->remove(
					mCB(this,uiBasemapItem,checkCB) );
}

void uiBasemapItem::setBasemap( uiBaseMap& bm )
{ basemap_ = &bm; }


void uiBasemapItem::addBasemapObject( BaseMapObject& bmo )
{
    basemap_->addObject( &bmo );
    basemapobjs_ += &bmo;
}


void uiBasemapItem::setTreeTop( uiTreeTopItem& tt )
{ treetop_ = &tt; }


void uiBasemapItem::addTreeItem( uiTreeItem& itm )
{
    treeitem_ = &itm;
    treetop_->addChild( &itm, true );
    itm.checkStatusChange()->notify( mCB(this,uiBasemapItem,checkCB) );
}


void uiBasemapItem::checkCB( CallBacker* cb )
{
    show( treeitem_->isChecked() );
}


void uiBasemapItem::show( bool yn )
{
    for ( int idx=0; idx<basemapobjs_.size(); idx++ )
	basemap_->show( *basemapobjs_[idx], yn );
}


uiODApplMgr& uiBasemapItem::applMgr()
{ return ODMainWin()->applMgr(); }

void uiBasemapItem::fillPar( IOPar& ) const
{}

bool uiBasemapItem::usePar( const IOPar& )
{ return true; }


// uiBasemapTreeTop
uiBasemapTreeTop::uiBasemapTreeTop( uiTreeView* tv )
    : uiTreeTopItem(tv)
{}

uiBasemapTreeTop::~uiBasemapTreeTop()
{}


// uiBasemapTreeItem
uiBasemapTreeItem::uiBasemapTreeItem( const char* nm )
    : uiTreeItem(nm)
{}

uiBasemapTreeItem::~uiBasemapTreeItem()
{}

int uiBasemapTreeItem::uiTreeViewItemType() const
{ return uiTreeViewItem::CheckBox; }


// uiBasemapGroup
uiBasemapGroup::uiBasemapGroup( uiParent* p, const char* nm )
    : uiGroup(p,nm)
    , namefld_(0)
{
}


uiBasemapGroup::~uiBasemapGroup()
{}


void uiBasemapGroup::addNameField( uiObject* attachobj )
{
    namefld_ = new uiGenInput( this, "Name" );
    namefld_->attach( alignedBelow, attachobj );
}


void uiBasemapGroup::setItemName( const char* nm )
{ if ( namefld_ ) namefld_->setText( nm ); }


const char* uiBasemapGroup::itemName() const
{ return namefld_ ? namefld_->text() : ""; }


bool uiBasemapGroup::fillPar( IOPar& par ) const
{
    if ( !namefld_ ) return true;

    const FixedString nm = namefld_->text();
    if ( nm.isEmpty() )
    {
	uiMSG().error( "Please enter a name." );
	return false;
    }

    par.set( sKey::Name(), nm );
    return true;
}


bool uiBasemapGroup::usePar( const IOPar& par )
{
    if ( namefld_ )
	namefld_->setText( par.find(sKey::Name()) );

    return true;
}
