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


// uiBasemapTreeTop
uiBasemapTreeTop::uiBasemapTreeTop( uiTreeView* tv )
    : uiTreeTopItem(tv)
{}

uiBasemapTreeTop::~uiBasemapTreeTop()
{}


// uiBasemapGroup
mImplFactory1Param( uiBasemapGroup, uiParent*, uiBasemapGroup::factory )

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
uiBasemapGroupDlg( uiParent* p, const char* itmnm )
    : uiDialog(p,Setup("Select Basemap parameters",mNoDlgTitle,mNoHelpKey))
{
    grp_ = uiBasemapGroup::factory().create( itmnm, this );
    setHelpKey( grp_->getHelpKey() );
}


bool fillPar( IOPar& par ) const
{ return grp_->fillPar( par ); }

bool usePar( const IOPar& par )
{ return grp_->usePar( par ); }


bool acceptOK( CallBacker* )
{ return grp_->acceptOK(); }

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
mImplFactory1Param( uiBasemapTreeItem, const char*, uiBasemapTreeItem::factory )

uiBasemapTreeItem::uiBasemapTreeItem( const char* nm )
    : uiTreeItem(nm)
    , pars_(*new IOPar)
{
}

uiBasemapTreeItem::~uiBasemapTreeItem()
{
    checkStatusChange()->remove( mCB(this,uiBasemapTreeItem,checkCB) );
    delete &pars_;
}


bool uiBasemapTreeItem::init()
{
    checkStatusChange()->notify( mCB(this,uiBasemapTreeItem,checkCB) );
    return true;
}


void uiBasemapTreeItem::addBasemapObject( BaseMapObject& bmo )
{
    basemapobjs_ += &bmo;
    BMM().getBasemap().addObject( &bmo );
}


void uiBasemapTreeItem::checkCB( CallBacker* )
{
    const bool show = isChecked();
    for ( int idx=0; idx<basemapobjs_.size(); idx++ )
	BMM().getBasemap().show( *basemapobjs_[idx], show );
}


int uiBasemapTreeItem::uiTreeViewItemType() const
{ return uiTreeViewItem::CheckBox; }


//uiBasemapManager
uiBasemapManager& BMM()
{
    mDefineStaticLocalObject( PtrMan<uiBasemapManager>, bmm,
			      (new uiBasemapManager) );
    return *bmm;
}


uiBasemapManager::uiBasemapManager()
    : basemap_(0)
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


void uiBasemapManager::add( const char* keyw )
{
    uiBasemapGroupDlg dlg( basemap_, keyw );
    if ( !dlg.go() )
	return;

    IOPar pars;
    dlg.fillPar( pars );

    uiBasemapTreeItem* itm =
	uiBasemapTreeItem::factory().create( keyw, dlg.grp_->itemName() );
    if ( !itm->usePar(pars) )
    {
	delete itm;
	return;
    }

    treeitems_ += itm;
    treetop_->addChild( itm, true );
}


void uiBasemapManager::edit( const char* keyw, const char* itmnm )
{
    uiBasemapTreeItem* itm = 0;
    const FixedString treeitemnm = itmnm;
    for ( int idx=0; idx<treeitems_.size(); idx++ )
    {
	if ( treeitemnm == treeitems_[idx]->name() )
	{
	    itm = treeitems_[idx];
	    break;
	}
    }

    if ( !itm ) return;

    uiBasemapGroupDlg dlg( basemap_, keyw );
    dlg.usePar( itm->pars() );
    if ( !dlg.go() )
	return;

    IOPar pars;
    dlg.fillPar( pars );
    itm->usePar( pars );
}
