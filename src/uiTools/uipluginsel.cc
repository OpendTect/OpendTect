/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Bril
 Date:          Feb 2006
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "uipluginsel.h"
#include "uigraphicsviewbase.h"
#include "uigraphicsitemimpl.h"
#include "uigraphicsscene.h"
#include "uibutton.h"
#include "uigroup.h"
#include "uilabel.h"
#include "uipixmap.h"
#include "uitreeview.h"

#include "ascstream.h"
#include "file.h"
#include "filepath.h"
#include "iopar.h"
#include "oddirs.h"
#include "plugins.h"
#include "settings.h"
#include "separstr.h"
#include "odver.h"
#include "od_helpids.h"
#include "od_istream.h"



const char* uiProductSel::sKeyDoAtStartup() { return "dTect.Select Plugins"; }

struct PluginProduct
{
    BufferString	    productname_;
    BufferString	    creator_;
    BufferStringSet	    libs_;
    BufferString	    pckgnm_;
    BufferString	    iconnm_;
    bool		    isselected_;
};


struct PluginVendor
{
    BufferString	vendorkey_;
    BufferString	vendorname_;
    BufferStringSet	aliases_;
    int			nrprods_;
};


class uiVendorTreeItem : public uiTreeViewItem
{
public:
			uiVendorTreeItem(uiTreeView*,const char*,bool);
 void			checkCB(CallBacker*);
};



uiVendorTreeItem::uiVendorTreeItem( uiTreeView* parnt,
				    const char* vendorname, bool issel )
    : uiTreeViewItem(parnt,Setup(uiString(vendorname)).
				    type(uiTreeViewItem::CheckBox))
{
    setChecked( issel, true );
    mAttachCB( stateChanged, uiVendorTreeItem::checkCB );
}


void uiVendorTreeItem::checkCB( CallBacker* )
{
    const bool ischecked = isChecked();
    for ( int idx=0; idx<nrChildren(); idx++ )
    {
	getChild( idx )->setChecked( ischecked );
	getChild( idx )->stateChanged.trigger();
    }
}


class uiProductTreeItem : public uiTreeViewItem
{
public:
			    uiProductTreeItem(uiTreeViewItem*,PluginProduct&);

protected:

    void		    checkCB(CallBacker*);
    PluginProduct&	    product_;
};


uiProductTreeItem::uiProductTreeItem( uiTreeViewItem* p,
					PluginProduct& prod )
    : uiTreeViewItem(p, Setup(uiString(prod.productname_))
		    .type(uiTreeViewItem::CheckBox))
    , product_(prod)
{
    setPixmap( 0, ioPixmap(prod.iconnm_) );
    setChecked( prod.isselected_, true );
    mAttachCB( stateChanged, uiProductTreeItem::checkCB );
}


void uiProductTreeItem::checkCB( CallBacker* )
{
    product_.isselected_ = isChecked();
    parent()->setChecked( parent()->isChecked() );
}


uiProductSel::uiProductSel( uiParent* p )
	: uiDialog(p,Setup("",mNoDlgTitle,
                            mODHelpKey(mPluginSelHelpID) )
			.savebutton(true)
			.savetext(tr("Show this dialog at startup")))
{
    BufferString titl( "OpendTect V" );
    titl += GetFullODVersion(); titl +=
			  tr(": Candidate auto-loaded plugins").getFullString();
    setCaption( tr(titl) );

    readVendorList();

    setOkText( tr("Start OpendTect") );
    setSaveButtonChecked( true );
    readPackageList();
    const ObjectSet<PluginManager::Data>& pimdata = PIM().getData();
    makeProductList( pimdata );
    createUI();
}


uiProductSel::~uiProductSel()
{
    deepErase( products_ );
    deepErase( vendors_ );
}

void uiProductSel::readVendorList()
{
    const FilePath vendfp( GetSoftwareDir(false), "data", "Vendors" );
    IOPar vendorpars;
    if ( !vendorpars.read(vendfp.fullPath(),".par") )
	return;

    for ( int idx=0; idx<vendorpars.size(); idx++ )
    {
	PluginVendor* pv = new PluginVendor;
	pv->vendorkey_ = vendorpars.getKey(idx);
	vendorpars.get( pv->vendorkey_, pv->aliases_ );
	pv->vendorname_ = pv->aliases_.get(0);
	pv->nrprods_ = 0;
	vendors_ += pv;
    }
}


int uiProductSel::getVendorIndex( const char* vendornm ) const
{
    for ( int idx=0; idx<vendors_.size(); idx++ )
    {
	if ( vendors_[idx]->aliases_.isPresent(vendornm) )
	    return idx;
    }

    return -1;
}


void uiProductSel::readPackageList()
{
    const FilePath prodlistfp( GetSoftwareDir(true), "data", "prodlist.txt" );
    od_istream prodstrm( prodlistfp.fullPath() ) ;
    while ( prodstrm.isOK() )
    {
	BufferString line;
	prodstrm.getLine( line );
	const FileMultiString sepline( line );
	PluginProduct* product = new PluginProduct;
	product->productname_ = sepline[0];
	product->pckgnm_ = sepline[1];
	product->iconnm_ = BufferString( product->pckgnm_, ".png" );
	products_ += product;
    }
}


BufferString getTrueProductName( const char* prodnm )
{
     BufferString prodname( prodnm );
     if ( prodname.find('(') )
     {
	    prodname.replace( '(', '\0' );
	    const int len = prodname.size();
	    prodname[len-1] = '\0';
     }

     return prodname;
}

void uiProductSel::makeProductList(
				const ObjectSet<PluginManager::Data>& pimdata )
{
    BufferStringSet dontloadlist;
    PIM().getNotLoadedByUser( dontloadlist );
    for ( int idx=0; idx<pimdata.size(); idx++ )
    {
	const PluginManager::Data& data = *pimdata[idx];
	if ( data.sla_ && data.sla_->isOK() )
	{
	    const FixedString prodnm = data.info_->productname_;
	    const bool isodprod = prodnm.isEmpty() || prodnm == "OpendTect";
	    if ( data.info_->lictype_ != PluginInfo::COMMERCIAL || isodprod )
		continue;

	    const BufferString trueprodnm = getTrueProductName( prodnm );
	    if ( trueprodnm == "QVRE" )
		continue;

	    const int prodidx = getProductIndex( trueprodnm );
	    const char* modulenm = PIM().moduleName(data.name_);
	    const int vidx = getVendorIndex( data.info_->creator_ );
	    if ( vidx < 0 )
	    {
		//TODO:ADD to Vendors_;
	    }
	    else
		vendors_[vidx]->nrprods_++;

	    if ( prodidx<0 )
	    {
		PluginProduct* product = new PluginProduct();
		product->productname_ = data.info_->productname_;
		product->creator_ = data.info_->creator_;
		product->libs_.add( modulenm );
		product->isselected_ = !dontloadlist.isPresent( modulenm );
		products_ += product;
	    }
	    else
	    {
		products_[prodidx]->libs_.addIfNew( modulenm );
		products_[prodidx]->creator_ = data.info_->creator_;
		products_[prodidx]->libs_.add( modulenm );
		products_[prodidx]->isselected_ =
					!dontloadlist.isPresent( modulenm );
	    }
	}
    }
}


void uiProductSel::createUI()
{
    uiGroup* grp = new uiGroup( this, "OpendTect plugins to load" );
    grp->setFrame( true );

    uiGraphicsViewBase* banner = new uiGraphicsViewBase( grp, "OpendTect" );
    uiPixmap pm( "banner.png" );
    uiPixmapItem* pmitem = new uiPixmapItem( uiPixmap(pm) );
    banner->scene().addItem( pmitem );
    banner->setPrefHeight( pm.height() );
    banner->setStretch( 2, 0 );

    treefld_ = new uiTreeView( grp, "Plugin tree" );
    treefld_->setStretch( 2, 2 );
    treefld_->showHeader( false );
    treefld_->attach( alignedBelow, banner );
    float height = 0.0f;
    for ( int idv=0; idv<vendors_.size(); idv++ )
    {
	if ( !vendors_[idv]->nrprods_ )
	    continue;

	const BufferString& vendorname = vendors_[idv]->vendorname_;
	uiVendorTreeItem* venditem =
	    new uiVendorTreeItem( treefld_, vendorname, isVendorSelected(idv) );
	const BufferString iconfnm( vendors_[idv]->vendorkey_, ".png" );
	venditem->setPixmap( 0, ioPixmap(iconfnm) );
	height++;
	for ( int idx=0; idx< products_.size(); idx++ )
	{
	    PluginProduct& pprod = *products_[idx];
	    if ( getVendorIndex(pprod.creator_) != idv )
		continue;
	    uiProductTreeItem* item = new uiProductTreeItem( venditem, pprod );
	    item->setPixmap( 0, uiPixmap(pprod.iconnm_) );
	    height++;
	}
    }

    treefld_->expandAll();
    treefld_->setPrefHeightInChar( height*0.9f );
}


bool uiProductSel::acceptOK( CallBacker* )
{
    FileMultiString dontloadlist;
    for ( int idx=0; idx<products_.size(); idx++ )
    {
	const PluginProduct& pprod = *products_[idx];
	if ( !pprod.isselected_ )
	{
	    for( int idp=0; idp<pprod.libs_.size(); idp++ )
	    {
		const BufferString& nm = pprod.libs_.get( idp );
		if ( dontloadlist.indexOf(nm.buf()) < 0 )
		    dontloadlist.add( nm );
	    }
	}
    }

    Settings::common().setYN( sKeyDoAtStartup(), saveButtonChecked() );
    Settings::common().set( PluginManager::sKeyDontLoad(), dontloadlist.rep() );
    Settings::common().write();

    return true;
}


int uiProductSel::getProductIndex( const char* prodnm ) const
{
    for ( int idx=0; idx<products_.size(); idx++ )
    {
	if ( products_[idx]->productname_ == prodnm )
	    return idx;
    }

    return -1;
}


bool uiProductSel::isVendorSelected( int vendoridx ) const
{
    for ( int idx=0; idx<products_.size(); idx++ )
    {
	if ( getVendorIndex(products_[idx]->creator_) == vendoridx
		&& products_[idx]->isselected_ )
	    return true;
    }
	
    return false;
}


//uiPluginSel - Deprecated

const char* uiPluginSel::sKeyDoAtStartup() { return "dTect.Select Plugins"; }

uiPluginSel::uiPluginSel( uiParent* p )
	: uiDialog(p,Setup("",mNoDlgTitle,
                            mODHelpKey(mPluginSelHelpID) )
			.savebutton(true)
			.savetext(tr("Show this dialog at startup")))
	, maxpluginname_(0)
{
    BufferString titl( "OpendTect V" );
    titl += GetFullODVersion(); titl +=
			  tr(": Candidate auto-loaded plugins").getFullString();
    setCaption( tr(titl) );

    setOkText( tr("Start OpendTect") );
    setSaveButtonChecked( true );
    const ObjectSet<PluginManager::Data>& pimdata = PIM().getData();
    makeProductList( pimdata );
    createUI();
}


uiPluginSel::~uiPluginSel()
{
    deepErase( products_ );
}


void uiPluginSel::makeGlobalProductList()
{
}


void uiPluginSel::makeProductList(
				const ObjectSet<PluginManager::Data>& pimdata )
{
    for ( int idx=0; idx<pimdata.size(); idx++ )
    {
	const PluginManager::Data& data = *pimdata[idx];
	if ( data.sla_ && data.sla_->isOK() )
	{
	    const FixedString prodnm = data.info_->productname_;
	    const bool isodprod = prodnm.isEmpty() || prodnm == "OpendTect";
	    if ( data.info_->lictype_ != PluginInfo::COMMERCIAL || isodprod )
		continue;

	    const int prodidx = getProductIndex( data.info_->productname_ );
	    if ( prodidx<0 )
	    {
		PluginProduct* product = new PluginProduct();
		product->productname_ = data.info_->productname_;
		product->creator_ = data.info_->creator_;
		product->libs_.add( PIM().moduleName(data.name_) );
		products_ += product;
	    }
	    else
		products_[prodidx]->libs_.addIfNew(
				PIM().moduleName(data.name_) );

	    const int strsz = prodnm.size() + 6;
	    maxpluginname_ = maxpluginname_ < strsz ? strsz : maxpluginname_;
	}
    }

    for ( int idx=products_.size()-1; idx>=0; idx-- )
    {
	const PluginProduct* product = products_[idx];
	if ( product->libs_.isEmpty() )
	    products_.removeSingle( idx );
    }
}


void uiPluginSel::createUI()
{
    uiLabel* lbl = new uiLabel( this, tr("Please select the plugins to load"));
    uiGroup* grp = new uiGroup( this, "OpendTect plugins to load" );
    grp->setFrame( true );
    grp->attach( centeredBelow, lbl );

    const int nrproducts = products_.size();
    const int nrrows = nrproducts % 2 == 0 ? (nrproducts/2) : (nrproducts/2)+1;
    for ( int idx=0; idx<nrproducts; idx++ )
    {
	const PluginProduct& pprod = *products_[idx];
	const BufferString label( pprod.productname_, " ", pprod.creator_ );
	uiCheckBox* cb = new uiCheckBox( grp, label );
	if ( !pprod.creator_.isEmpty() )
	    cb->setToolTip( BufferString("a ",pprod.creator_, " plugin") );
	cb->setPrefWidthInChar( maxpluginname_+6.f );
	cb->setChecked( true );
	cb->setStretch( 2, 0 ); 
	cbs_ += cb;

	if ( idx < nrrows )
	{
	    if ( idx == 0 )
		continue;
	    cb->attach( alignedBelow, cbs_[idx-1] );
	}
	else
	    cb->attach( rightOf, cbs_[(idx-nrrows)] );
    }

    BufferStringSet dontloadlist;
    PIM().getNotLoadedByUser( dontloadlist );
    for ( int idx=0; idx<dontloadlist.size() && !cbs_.isEmpty(); idx++ )
    {
	const int prodidx = getProductIndexForLib( dontloadlist.get(idx) );
	if( prodidx < 0 )
	    continue;
	cbs_[prodidx]->setChecked( false );
    }
}


bool uiPluginSel::acceptOK( CallBacker* )
{
    FileMultiString dontloadlist;
    for ( int idx=0; idx<cbs_.size(); idx++ )
    {
	if ( !cbs_[idx]->isChecked() )
	{
	    for( int idp=0; idp<products_[idx]->libs_.size(); idp++ )
		dontloadlist += products_[idx]->libs_.get(idp);
	}
    }

    Settings::common().setYN( sKeyDoAtStartup(), saveButtonChecked() );
    Settings::common().set( PluginManager::sKeyDontLoad(), dontloadlist.rep() );
    Settings::common().write();

    return true;
}


int uiPluginSel::getProductIndex( const char* prodnm ) const
{
    for ( int idx=0; idx<products_.size(); idx++ )
    {
	if ( products_[idx]->productname_ == prodnm )
	    return idx;
    }

    return -1;
}


int uiPluginSel::getProductIndexForLib( const char* libnm ) const
{
    for ( int idx=0; idx<products_.size(); idx++ )
    {
	if ( products_[idx]->libs_.isPresent(libnm) )
	    return idx;
    }

    return -1;
}
