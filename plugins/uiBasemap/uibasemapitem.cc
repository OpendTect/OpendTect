/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nanne Hemstra
 Date:		January 2013
________________________________________________________________________

-*/

static const char* rcsID mUsedVar = "$Id$";


#include "uibasemapitem.h"

#include "uibasemap.h"
#include "uidialog.h"
#include "uigeninput.h"
#include "uimenu.h"
#include "uimsg.h"
#include "uiodapplmgr.h"
#include "uiodmain.h"
#include "uistrings.h"
#include "uitreeview.h"

#include "basemapimpl.h"
#include "pixmap.h"


// uiBasemapTreeTop
uiBasemapTreeTop::uiBasemapTreeTop( uiTreeView* tv )
    : uiTreeTopItem(tv)
{}

uiBasemapTreeTop::~uiBasemapTreeTop()
{}


// uiBasemapGroup
const char* uiBasemapGroup::sKeyNrObjs()	{ return "Nr Objects"; }

uiBasemapGroup::uiBasemapGroup( uiParent* p )
    : uiGroup(p)
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

HelpKey uiBasemapGroup::getHelpKey() const
{ return HelpKey::emptyHelpKey(); }


bool uiBasemapGroup::fillPar( IOPar& par ) const
{
    if ( !namefld_ ) return true;

    par.set( sKey::Name(), namefld_->text() );
    return true;
}


bool uiBasemapGroup::usePar( const IOPar& par )
{
    if ( namefld_ )
	namefld_->setText( par.find(sKey::Name()) );

    return true;
}


bool uiBasemapGroup::acceptOK()
{
    const FixedString nm = namefld_->text();
    if ( nm.isEmpty() )
    {
	uiMSG().error( "Please enter a name." );
	return false;
    }

    return true;
}


class uiBasemapGroupDlg : public uiDialog
{
public:
uiBasemapGroupDlg( uiParent* p, uiBasemapItem& itm )
    : uiDialog(p,Setup("Select Basemap parameters",mNoDlgTitle,mNoHelpKey))
    , grp_(0)
{
    grp_ = itm.createGroup( this );
    grp_->setItemName( itm.factoryDisplayName() );
    setHelpKey( grp_->getHelpKey() );
}


bool fillPar( IOPar& par ) const
{ return grp_->fillPar( par ); }

bool usePar( const IOPar& par )
{ return grp_->usePar( par ); }


bool acceptOK( CallBacker* )
{ return grp_ ? grp_->acceptOK() : true; }

    uiBasemapGroup* grp_;
};


// uiBasemapItem
mImplFactory( uiBasemapItem, uiBasemapItem::factory )

uiBasemapItem::uiBasemapItem()
{
    mDefineStaticLocalObject( Threads::Atomic<int>, itmid, (100) );
    id_ = itmid++;
}


// uiBasemapTreeItem
uiBasemapTreeItem::uiBasemapTreeItem( const char* nm )
    : uiTreeItem(nm)
    , pars_(*new IOPar)
{
    mDefineStaticLocalObject( Threads::Atomic<int>, treeitmid, (1000) );
    id_ = treeitmid++;
}

uiBasemapTreeItem::~uiBasemapTreeItem()
{
    checkStatusChange()->remove( mCB(this,uiBasemapTreeItem,checkCB) );
    delete &pars_;
}


bool uiBasemapTreeItem::init()
{
    checkStatusChange()->notify( mCB(this,uiBasemapTreeItem,checkCB) );

    const uiBasemapItem* itm = BMM().getBasemapItem( familyid_ );
    const char* iconnm = itm ? itm->iconName() : 0;
    uitreeviewitem_->setPixmap( 0, ioPixmap(iconnm) );
    return true;
}


void uiBasemapTreeItem::addBasemapObject( BaseMapObject& bmo )
{
    basemapobjs_ += &bmo;
    BMM().getBasemap().addObject( &bmo );
}


BaseMapObject* uiBasemapTreeItem::removeBasemapObject( BaseMapObject& bmo )
{
    basemapobjs_ -= &bmo;
    BMM().getBasemap().removeObject( &bmo );
    return &bmo;
}


void uiBasemapTreeItem::checkCB( CallBacker* )
{
    const bool show = isChecked();
    for ( int idx=0; idx<basemapobjs_.size(); idx++ )
	BMM().getBasemap().show( *basemapobjs_[idx], show );
}


int uiBasemapTreeItem::uiTreeViewItemType() const
{ return uiTreeViewItem::CheckBox; }


bool uiBasemapTreeItem::showSubMenu()
{ return true; }


bool uiBasemapTreeItem::handleSubMenu(int)
{ return true; }


bool uiBasemapTreeItem::usePar( const IOPar& par )
{
    pars_ = par;

    BufferString nm;
    if ( par.get(sKey::Name(),nm) )
    {
	setName( nm );
	updateColumnText( 0 );
    }

    return true;
}



//uiBasemapManager
uiBasemapManager& BMM()
{
    mDefineStaticLocalObject( PtrMan<uiBasemapManager>, bmm,
			      (new uiBasemapManager) );
    return *bmm;
}


uiBasemapManager::uiBasemapManager()
    : basemap_(0)
    , basemapcursor_(0)
    , treetop_(0)
{
    init();
}


uiBasemapManager::~uiBasemapManager()
{}


void uiBasemapManager::init()
{
    const BufferStringSet& nms = uiBasemapItem::factory().getNames();
    for ( int idx=0; idx<nms.size(); idx++ )
	basemapitems_ += uiBasemapItem::factory().create( nms.get(idx) );
}


void uiBasemapManager::setBasemap( uiBaseMap& bm )
{ basemap_ = &bm; }

uiBaseMap& uiBasemapManager::getBasemap()
{ return *basemap_; }

void uiBasemapManager::setTreeTop( uiTreeTopItem& tt )
{ treetop_ = &tt; }


void uiBasemapManager::add( int itemid )
{
    uiBasemapItem* itm = getBasemapItem( itemid );
    if ( !itm ) return;

    uiBasemapGroupDlg dlg( basemap_, *itm );
    if ( !dlg.go() )
	return;

    IOPar pars;
    dlg.fillPar( pars );

    uiBasemapTreeItem* treeitm = itm->createTreeItem( dlg.grp_->itemName() );
    treeitm->setFamilyID( itm->ID() );
    if ( !treeitm->usePar(pars) )
    {
	delete treeitm;
	return;
    }

    treeitems_ += treeitm;
    treetop_->addChild( treeitm, true );
}


void uiBasemapManager::edit( int itemid, int treeitemid )
{
    uiBasemapItem* itm = getBasemapItem( itemid );
    if ( !itm ) return;

    uiBasemapTreeItem* treeitm = getBasemapTreeItem( treeitemid );
    if ( !treeitm ) return;

    uiBasemapGroupDlg dlg( basemap_, *itm );
    dlg.usePar( treeitm->pars() );
    if ( !dlg.go() )
	return;

    IOPar pars;
    dlg.fillPar( pars );
    treeitm->usePar( pars );
}


uiBasemapItem* uiBasemapManager::getBasemapItem( int id )
{
    for ( int idx=0; idx<basemapitems_.size(); idx++ )
    {
	if ( basemapitems_[idx]->ID() == id )
	    return basemapitems_[idx];
    }

    return 0;
}


uiBasemapTreeItem* uiBasemapManager::getBasemapTreeItem( int id )
{
    for ( int idx=0; idx<treeitems_.size(); idx++ )
    {
	if ( treeitems_[idx]->ID() == id )
	    return treeitems_[idx];
    }

    return 0;
}


void uiBasemapManager::updateMouseCursor( const Coord3& coord )
{
    const bool defined = coord.isDefined();

    if ( defined && !basemapcursor_ )
    {
	basemapcursor_ = new BaseMapMarkers;
	basemapcursor_->setMarkerStyle(0,
		MarkerStyle2D(MarkerStyle2D::Target,5) );
	basemap_->addObject( basemapcursor_ );
    }

    if ( !basemapcursor_ )
	return;

    Threads::Locker lckr( basemapcursor_->lock_,
			  Threads::Locker::DontWaitForLock );

    if ( lckr.isLocked() )
    {
	if ( !defined )
	    basemapcursor_->positions().erase();
	else if ( basemapcursor_->positions().isEmpty() )
		basemapcursor_->positions() += coord;
	else
	    basemapcursor_->positions()[0] = coord;

	lckr.unlockNow();
	basemapcursor_->updateGeometry();
    }
}
