/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Bril
 Date:          Feb 2006
________________________________________________________________________

-*/

#include "uipluginsel.h"
#include "uigraphicsviewbase.h"
#include "uigraphicsitemimpl.h"
#include "uigraphicsscene.h"
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


const char* uiPluginSel::sKeyDoAtStartup() { return "dTect.Select Plugins"; }

struct PluginProduct
{
			PluginProduct()
				: isselected_(true)	{}

    BufferString	productname_;
    BufferString	creator_;
    BufferStringSet	libs_;
    BufferString	pckgnm_;
    bool		isselected_;
};


struct PluginVendor
{
			PluginVendor()
				: nrprods_(0)		{}

    BufferString	vendorkey_;
    BufferString	vendorname_;
    BufferStringSet	aliases_;
    int			nrprods_;
};


class uiVendorTreeItem : public uiTreeViewItem
{
public:
			uiVendorTreeItem(uiTreeView*,const char*,bool);
			~uiVendorTreeItem() { detachAllNotifiers(); }
 void			checkCB(CallBacker*);
};



uiVendorTreeItem::uiVendorTreeItem( uiTreeView* p,
				    const char* vendorname, bool issel )
    : uiTreeViewItem(p,Setup(mToUiStringTodo(vendorname)).
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
			    ~uiProductTreeItem() { detachAllNotifiers(); }

protected:

    void		    checkCB(CallBacker*);
    PluginProduct&	    product_;
};


uiProductTreeItem::uiProductTreeItem( uiTreeViewItem* p,
					PluginProduct& prod )
    : uiTreeViewItem(p, Setup(mToUiStringTodo(prod.productname_))
		    .iconname(prod.pckgnm_).type(uiTreeViewItem::CheckBox))
    , product_(prod)
{
    setChecked( prod.isselected_, true );
    mAttachCB( stateChanged, uiProductTreeItem::checkCB );
}


void uiProductTreeItem::checkCB( CallBacker* )
{
    product_.isselected_ = isChecked();
    parent()->setChecked( parent()->isChecked() );
}


uiPluginSel::uiPluginSel( uiParent* p )
	: uiDialog(p,Setup(uiString::empty(),mNoDlgTitle,
                            mODHelpKey(mPluginSelHelpID) )
			.savebutton(true)
			.savetext(tr("Show this dialog at startup")))
{
    BufferString titl( "OpendTect V" );
    titl += GetFullODVersion(); titl +=
			  tr(": Candidate auto-loaded plugins").getString();
    setCaption( tr(titl) );

    readVendorList();

    setOkText( tr("Start OpendTect") );
    setSaveButtonChecked( true );
    readPackageList();
    const ObjectSet<PluginManager::Data>& pimdata = PIM().getData();
    makeProductList( pimdata );
    createUI();
    showAlwaysOnTop();
}


uiPluginSel::~uiPluginSel()
{
    deepErase( products_ );
    deepErase( vendors_ );
}

void uiPluginSel::readVendorList()
{
    const File::Path vendfp( mGetSWDirDataDir(), "Vendors" );
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


int uiPluginSel::getVendorIndex( const char* vendornm ) const
{
    for ( int idx=0; idx<vendors_.size(); idx++ )
    {
	if ( vendors_[idx]->aliases_.isPresent(vendornm) )
	    return idx;
    }

    return -1;
}


void uiPluginSel::readPackageList()
{
    const File::Path prodlistfp( mGetSWDirDataDir(), "prodlist.txt" );
    od_istream prodstrm( prodlistfp.fullPath() ) ;
    while ( prodstrm.isOK() )
    {
	BufferString line;
	prodstrm.getLine( line );
	const FileMultiString sepline( line );
	PluginProduct* product = new PluginProduct;
	product->productname_ = sepline[0];
	product->pckgnm_ = sepline[1];
	products_ += product;
    }
}


void uiPluginSel::makeProductList(
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

	    const int prodidx = getProductIndex( data.info_->productname_ );
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


void uiPluginSel::createUI()
{
    uiGroup* grp = new uiGroup( this, "OpendTect plugins to load" );

    uiGraphicsViewBase* banner = new uiGraphicsViewBase( grp, "OpendTect" );
    uiPixmap pm( "banner.png" );
    uiPixmapItem* pmitem = new uiPixmapItem( pm );
    banner->scene().addItem( pmitem );
    banner->setPrefHeight( pm.height() );
    banner->setPrefWidth( pm.width() );
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
	venditem->setIcon( 0, iconfnm );
	height++;
	for ( int idx=0; idx< products_.size(); idx++ )
	{
	    PluginProduct& pprod = *products_[idx];
	    if ( getVendorIndex(pprod.creator_) != idv )
		continue;
	    uiProductTreeItem* item = new uiProductTreeItem( venditem, pprod );
	    item->setPixmap( 0, uiPixmap(pprod.pckgnm_) );
	    height++;
	}
    }

    treefld_->expandAll();
    treefld_->setPrefHeightInChar( height ? height+2 : height );
}


bool uiPluginSel::acceptOK()
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
    Settings::common().set( PluginManager::sKeyDontLoad(), dontloadlist.str() );
    Settings::common().write();

    return true;
}


int uiPluginSel::getProductIndex( const char* prodnm ) const
{
    for ( int idx=0; idx<products_.size(); idx++ )
    {
	if ( products_[idx]->productname_ == prodnm
		|| products_[idx]->pckgnm_ == prodnm )
	    return idx;
    }

    return -1;
}


bool uiPluginSel::isVendorSelected( int vendoridx ) const
{
    for ( int idx=0; idx<products_.size(); idx++ )
    {
	if ( getVendorIndex(products_[idx]->creator_) == vendoridx
		&& products_[idx]->isselected_ )
	    return true;
    }

    return false;
}
