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


const char* uiPluginSel::sKeyDoAtStartup() { return "dTect.Select Plugins"; }

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



uiVendorTreeItem::uiVendorTreeItem( uiTreeView* p,
				    const char* vendorname, bool issel )
    : uiTreeViewItem(p,Setup(uiString(vendorname)).
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
		    .iconname(prod.iconnm_).type(uiTreeViewItem::CheckBox))
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


uiPluginSel::~uiPluginSel()
{
    deepErase( products_ );
}


void uiPluginSel::readPackageList()
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


void uiPluginSel::createUI()
{
    uiGroup* grp = new uiGroup( this, "OpendTect plugins to load" );
    grp->setFrame( true );

    uiGraphicsViewBase* banner = new uiGraphicsViewBase( grp, "OpendTect" );
    uiPixmap pm("banner.png");
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
	const FixedString vendorname = vendors_[idv]->buf();
	uiVendorTreeItem* venditem =
	    new uiVendorTreeItem( treefld_, vendorname,
					       isVendorSelected(vendorname)  );
	const BufferString iconfnm( getVendorName(vendorname), ".png" );
	venditem->setIcon( 0, iconfnm );
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


bool uiPluginSel::acceptOK( CallBacker* )
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


int uiPluginSel::getProductIndex( const char* prodnm ) const
{
    for ( int idx=0; idx<products_.size(); idx++ )
    {
	if ( products_[idx]->productname_ == prodnm )
	    return idx;
    }

    return -1;
}


bool uiPluginSel::isVendorSelected( const char* vendnm ) const
{
    for ( int idx=0; idx<products_.size(); idx++ )
    {
	if ( products_[idx]->creator_ == vendnm
	    && products_[idx]->isselected_ )
	    return true;
    }

    return false;
}
