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
#include "oddirs.h"
#include "plugins.h"
#include "settings.h"
#include "separstr.h"
#include "strmdata.h"
#include "strmoper.h"
#include "strmprov.h"
#include "odver.h"
#include "od_helpids.h"
#include "od_strstream.h"



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


static const char* getVendorName( const char* creatorname )
{
    const FixedString crnm( creatorname );
    if ( crnm == "dGB Earth Sciences" )
	return "dgb";
    if ( crnm == "ARK CLS" )
	return "arkcls";
    if ( crnm == "Earthworks & ARK CLS" )
	return "earthworks";
    if ( crnm == "ARKeX" )
	return crnm;

    return "sitfal";
}


static BufferString getTrueProductName( BufferString prodnm )
{
    prodnm.replace( '(', '\0' );
    return prodnm;
}


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


uiProductTreeItem::uiProductTreeItem( uiTreeViewItem* parnt,
					PluginProduct& prod )
    : uiTreeViewItem(parnt,
		    Setup(uiString(prod.productname_))
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
}


void uiProductSel::readPackageList()
{
    const FilePath prodlistfp( GetSoftwareDir(true), "data", "prodlist.txt" );
    StreamData prodlistsd(
			StreamProvider(prodlistfp.fullPath()).makeIStream() );
    od_istream istrm( prodlistsd.istrm );
    while ( istrm.isOK() )
    {
	BufferString line;
	StrmOper::readLine( *prodlistsd.istrm, &line );
	FileMultiString sepline( line );
	PluginProduct* product = new PluginProduct;
	product->productname_ = sepline[0];
	product->pckgnm_ = sepline[1];
	product->iconnm_ = BufferString( product->pckgnm_, ".png" );
	products_ += product;
    }
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

	    const int prodidx = getProductIndex(
				getTrueProductName(data.info_->productname_) );
	    const char* modulenm = PIM().moduleName(data.name_);
	    vendors_.addIfNew( data.info_->creator_ );
	    if ( prodidx<0 )
	    {
		PluginProduct* product = new PluginProduct();
		product->productname_ =
		    getTrueProductName( data.info_->productname_ );
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
    uiPixmap pm("banner.png");
    uiPixmapItem* pmitem = new uiPixmapItem( uiPixmap(pm) );
    banner->scene().addItem( pmitem );
    banner->setPrefHeight( pm.height() );
     
    treefld_ = new uiTreeView( grp, "Plugin tree" );
    treefld_->setStretch( 2, 2 );
    treefld_->showHeader( false );
    treefld_->attach( alignedBelow, banner );
    float height = 0.0f;
    for ( int idv=0; idv<vendors_.size(); idv++ )
    {
	const FixedString vendorname = vendors_[idv]->buf();
	uiVendorTreeItem* venditem =
	    new uiVendorTreeItem( treefld_, vendorname,
					       isVendorSelected(vendorname)  );
	const BufferString iconfnm( getVendorName(vendorname), ".png" );
	venditem->setPixmap( 0, ioPixmap(iconfnm) );
	height++;
	for ( int idx=0; idx< products_.size(); idx++ )
	{
	    PluginProduct& pprod = *products_[idx];
	    if ( vendorname != pprod.creator_ )
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
		dontloadlist += pprod.libs_.get(idp);
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


bool uiProductSel::isVendorSelected( const char* vendnm ) const
{
    for ( int idx=0; idx<products_.size(); idx++ )
    {
	if ( products_[idx]->creator_ == vendnm
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
