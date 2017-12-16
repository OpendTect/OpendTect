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
#include "uiseparator.h"

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


// This key was deliberately changed from 6.X to 7.X to at least once
// show the new window to users ...
static const char* sOldKeyDoAtStartup = "dTect.Select Plugins";
const char* uiPluginSel::sKeyDoAtStartup() { return "dTect.UI.Select Plugins"; }

struct PluginPackage
{
    BufferString	name_;
    BufferStringSet	plugins_;
    BufferString	creator_;
    BufferStringSet	libs_;
    BufferString	pkgkey_;
    bool		isselected_	= true;
};


struct PluginProvider
{
    BufferString	providerkey_;
    BufferString	providername_;
    BufferStringSet	aliases_;
    int			nrplugins_	= 0;
};


class uiProviderTreeItem : public uiTreeViewItem
{
public:

			uiProviderTreeItem(uiTreeView*,const char*,bool);
			~uiProviderTreeItem() { detachAllNotifiers(); }

 void			checkCB(CallBacker*);

};



uiProviderTreeItem::uiProviderTreeItem( uiTreeView* p,
				    const char* providername, bool issel )
    : uiTreeViewItem(p,Setup(toUiString(providername)).
				    type(uiTreeViewItem::CheckBox))
{
    setChecked( issel, true );
    mAttachCB( stateChanged, uiProviderTreeItem::checkCB );
}


void uiProviderTreeItem::checkCB( CallBacker* )
{
    const bool ischecked = isChecked();
    for ( int idx=0; idx<nrChildren(); idx++ )
    {
	getChild( idx )->setChecked( ischecked );
	getChild( idx )->stateChanged.trigger();
    }
}


class uiPackageTreeItem : public uiTreeViewItem
{
public:
			    uiPackageTreeItem(uiTreeViewItem*,PluginPackage&);
			    ~uiPackageTreeItem() { detachAllNotifiers(); }

protected:

    void		    checkCB(CallBacker*);
    PluginPackage&	    package_;

};


uiPackageTreeItem::uiPackageTreeItem( uiTreeViewItem* p,
					PluginPackage& pkg )
    : uiTreeViewItem(p, Setup(toUiString(pkg.name_))
		    .iconname(pkg.pkgkey_).type(uiTreeViewItem::CheckBox))
    , package_(pkg)
{
    setChecked( pkg.isselected_, true );
    mAttachCB( stateChanged, uiPackageTreeItem::checkCB );
    setToolTip( 0, toUiString(pkg.plugins_.getDispString(8,false)) );
}


void uiPackageTreeItem::checkCB( CallBacker* )
{
    package_.isselected_ = isChecked();
    parent()->setChecked( parent()->isChecked() );
}


uiPluginSel::uiPluginSel( uiParent* p )
	: uiDialog(p,Setup(uiStrings::sEmptyString(),mNoDlgTitle,
                            mODHelpKey(mPluginSelHelpID) )
			.savebutton(true)
			.savetext(tr("Show this dialog at startup")))
{
    setCaption( tr("OpendTect V%1: Optional plugins").arg(GetFullODVersion()) );
    setOkText( tr("Start OpendTect") );
    readProviderList();
    setSaveButtonChecked( true );
    readPackageList();
    makePackageList();
    createUI();
}


uiPluginSel::~uiPluginSel()
{
    deepErase( packages_ );
    deepErase( providers_ );
}

void uiPluginSel::readProviderList()
{
    const File::Path vendfp( mGetSWDirDataDir(), "PluginProviders" );
    IOPar providerpars;
    if ( !providerpars.read(vendfp.fullPath(),".par") )
	return;

    for ( int ipar=0; ipar<providerpars.size(); ipar++ )
    {
	PluginProvider* pv = new PluginProvider;
	pv->providerkey_ = providerpars.getKey( ipar );
	providerpars.get( pv->providerkey_, pv->aliases_ );
	pv->providername_ = pv->aliases_.get(0);
	pv->nrplugins_ = 0;
	providers_ += pv;
    }
}


int uiPluginSel::getProviderIndex( const char* providernm ) const
{
    for ( int iprov=0; iprov<providers_.size(); iprov++ )
    {
	if ( providers_[iprov]->aliases_.isPresent(providernm) )
	    return iprov;
    }

    return -1;
}


static BufferString getCleanPluginName( const char* nm )
{
    BufferString ret( nm );
    char* ptr = ret.findLast( '[' );
    if ( ptr )
	*ptr = '\0';
    ret.trimBlanks();
    return ret;
}


void uiPluginSel::readPackageList()
{
    const File::Path pkglistfp( mGetSWDirDataDir(), "pkglist.txt" );
    od_istream pkgstrm( pkglistfp.fullPath() ) ;
    while ( pkgstrm.isOK() )
    {
	BufferString line;
	pkgstrm.getLine( line );
	const FileMultiString sepline( line );
	PluginPackage* package = new PluginPackage;
	package->name_ = sepline[0];
	package->pkgkey_ = sepline[1];
	packages_ += package;
    }
}


void uiPluginSel::makePackageList()
{
    const ObjectSet<PluginManager::Data>& pimdata = PIM().getData();
    BufferStringSet dontloadlist;
    PIM().getNotLoadedByUser( dontloadlist );
    for ( int ipim=0; ipim<pimdata.size(); ipim++ )
    {
	const PluginManager::Data& data = *pimdata[ipim];
	const bool havesla = data.sla_ && data.sla_->isOK();
	const bool issel = data.info_ && data.info_->useronoffselectable_;
	const bool isod = data.info_ && FixedString(data.info_->packagename_) == "OpendTect";
	if ( havesla && issel && !isod )
	{
	    int pkgidx = getPackageIndex( data.info_->packagename_ );
	    const char* modulenm = PIM().moduleName( data.name_ );
	    int providx = getProviderIndex( data.info_->creator_ );
	    if ( providx < 0 )
	    {
		PluginProvider* pv = new PluginProvider;
		pv->providername_ = data.info_->creator_;
		if ( pv->providername_.isEmpty() )
		    pv->providername_.set( "Unknown Provider" );
		if ( pv->providername_ == "opendtect.org" )
		    pv->providerkey_ = "od";
		else
		    pv->providerkey_ = "unknownpersons";
		pv->aliases_.add( pv->providername_ );
		providers_ += pv;
		providx = providers_.size() - 1;
	    }
	    providers_[providx]->nrplugins_++;

	    if ( pkgidx < 0 )
	    {
		PluginPackage* package = new PluginPackage();
		package->name_ = data.info_->packagename_;
		packages_ += package;
		pkgidx = packages_.size() - 1;
	    }

	    PluginPackage& pkg = *packages_[pkgidx];
	    pkg.libs_.addIfNew( modulenm );
	    pkg.plugins_.addIfNew( getCleanPluginName(data.info_->dispname_) );
	    pkg.creator_ = data.info_->creator_;
	    pkg.isselected_ = !dontloadlist.isPresent( modulenm );
	}
    }
}


class uiPluginSelBannerDrawer : public uiGraphicsViewBase
{
public:

uiPluginSelBannerDrawer( uiGroup* p )
    : uiGraphicsViewBase( p, "OD Version banner" )
    , pm_("banner.png")
{
    uiPixmapItem* pmitem = new uiPixmapItem( pm_ );
    scene().addItem( pmitem );
    setPrefWidth( pm_.width() );
    setPrefHeight( pm_.height() );
    setStretch( 2, 0 );
}

    const uiPixmap pm_;
};


void uiPluginSel::createUI()
{
    if ( providers_.isEmpty() )
    {
	new uiLabel( this, tr("[No optional plugins installed]") );
	return;
    }

    uiGroup* grp = new uiGroup( this, "OpendTect plugins to load" );
    grp->setFrame( true );

    uiPluginSelBannerDrawer* banner = new uiPluginSelBannerDrawer( grp );
    uiSeparator* sep = new uiSeparator( grp );
    sep->attach( stretchedBelow, banner );

    uiTreeView* treefld = new uiTreeView( grp, "Plugin tree" );
    treefld->showHeader( false );
    treefld->attach( ensureBelow, sep );
    int height = 0;
    for ( int iprov=0; iprov<providers_.size(); iprov++ )
    {
	if ( providers_[iprov]->nrplugins_ < 1 )
	    continue;

	const BufferString& providername = providers_[iprov]->providername_;
	uiProviderTreeItem* provitem = new uiProviderTreeItem( treefld,
			    providername, isProviderSelected(iprov) );
	const BufferString iconfnm( providers_[iprov]->providerkey_, ".png" );
	provitem->setIcon( 0, iconfnm );
	height++;
	for ( int ipkg=0; ipkg< packages_.size(); ipkg++ )
	{
	    PluginPackage& ppkg = *packages_[ipkg];
	    if ( getProviderIndex(ppkg.creator_) != iprov )
		continue;
	    uiPackageTreeItem* item = new uiPackageTreeItem( provitem, ppkg );
	    item->setPixmap( 0, uiPixmap(ppkg.pkgkey_) );
	    height++;
	}
    }

    int prefheight = height + 3;
    if ( prefheight > 30 )
	prefheight = 30;

    treefld->expandAll();
    treefld->setPrefWidth( banner->pm_.width() );
    treefld->setPrefHeightInChar( prefheight );
    treefld->setStretch( 0, 2 );

    setPrefWidth( banner->pm_.width() );
}

bool uiPluginSel::acceptOK()
{
    if ( packages_.isEmpty() )
	return true;

    FileMultiString dontloadlist;
    for ( int ipkg=0; ipkg<packages_.size(); ipkg++ )
    {
	const PluginPackage& ppkg = *packages_[ipkg];
	if ( !ppkg.isselected_ )
	{
	    for( int idp=0; idp<ppkg.libs_.size(); idp++ )
	    {
		const BufferString& nm = ppkg.libs_.get( idp );
		if ( dontloadlist.indexOf(nm.buf()) < 0 )
		    dontloadlist.add( nm );
	    }
	}
    }

    Settings::common().removeWithKey( sOldKeyDoAtStartup );
    Settings::common().setYN( sKeyDoAtStartup(), saveButtonChecked() );
    Settings::common().set( PluginManager::sKeyDontLoad(), dontloadlist.rep() );
    Settings::common().write();

    return true;
}


int uiPluginSel::getPackageIndex( const char* pkgnm ) const
{
    for ( int ipkg=0; ipkg<packages_.size(); ipkg++ )
    {
	if ( packages_[ipkg]->name_ == pkgnm )
	    return ipkg;
    }

    return -1;
}


bool uiPluginSel::isProviderSelected( int provideridx ) const
{
    for ( int ipkg=0; ipkg<packages_.size(); ipkg++ )
    {
	if ( getProviderIndex(packages_[ipkg]->creator_) == provideridx
		&& packages_[ipkg]->isselected_ )
	    return true;
    }

    return false;
}
