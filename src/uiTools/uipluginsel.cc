/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Bril
 Date:          Feb 2006
________________________________________________________________________

-*/

#include "uipluginsel.h"
#include "uidesktopservices.h"
#include "uigraphicsviewbase.h"
#include "uigraphicsitemimpl.h"
#include "uigraphicsscene.h"
#include "uigroup.h"
#include "uilabel.h"
#include "uimenu.h"
#include "uimsg.h"
#include "uipixmap.h"
#include "uiseparator.h"
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


// This key was deliberately changed from 6.X to 7.X to at least once
// show the new window to users ...
static const char* sOldKeyDoAtStartup = "dTect.Select Plugins";
const char* uiPluginSel::sKeyDoAtStartup() { return "dTect.UI.Select Plugins"; }


class PluginProvider
{
public:

    BufferString	name_;
    BufferString	key_;
    BufferString	url_;
    BufferStringSet	aliases_;
    int			nrplugins_	= 0;

};

class PluginPackage
{
public:
			PluginPackage() : prov_(0)	{}

    const PluginProvider* prov_;
    BufferString	name_;
    BufferString	creator_;
    BufferString	version_;
    BufferString	url_;
    BufferString	pkgkey_;
    bool		isselected_	= true;
    BufferStringSet	plugins_;	// for user display
    BufferStringSet	libs_;		// to use

};


class uiProviderTreeItem : public uiTreeViewItem
{
public:

uiProviderTreeItem( uiTreeView* p, const PluginProvider& prov, bool issel )
    : uiTreeViewItem(p,Setup(toUiString(prov.name_)).
				    type(uiTreeViewItem::CheckBox))
    , prov_(prov)
{
    setChecked( issel, true );
    setIcon( 0, BufferString(prov.key_,".png") );
    mAttachCB( stateChanged, uiProviderTreeItem::checkCB );
}

~uiProviderTreeItem()
{
    detachAllNotifiers();
}

void checkCB( CallBacker* )
{
    const bool ischecked = isChecked();
    for ( int idx=0; idx<nrChildren(); idx++ )
    {
	getChild( idx )->setChecked( ischecked );
	getChild( idx )->stateChanged.trigger();
    }
}

    const PluginProvider&	prov_;

};


class uiPackageTreeItem : public uiTreeViewItem
{
public:

uiPackageTreeItem( uiTreeViewItem* p, PluginPackage& pkg )
    : uiTreeViewItem(p, Setup(toUiString(pkg.name_))
		    .iconname(pkg.pkgkey_).type(uiTreeViewItem::CheckBox))
    , pkg_(pkg)
{
    setChecked( pkg.isselected_, true );
    mAttachCB( stateChanged, uiPackageTreeItem::checkCB );
    setToolTip( 0, toUiString(pkg.plugins_.getDispString(8,false)) );
}

~uiPackageTreeItem()
{
    detachAllNotifiers();
}

void checkCB( CallBacker* )
{
    pkg_.isselected_ = isChecked();
    parent()->setChecked( parent()->isChecked() );
}

    PluginPackage&	    pkg_;

};


uiPluginSel::uiPluginSel( uiParent* p )
    : uiDialog(p,Setup(uiStrings::sEmptyString(),mNoDlgTitle,
			mODHelpKey(mPluginSelHelpID) )
		    .savebutton(true)
		    .savetext(tr("Show this dialog at startup")))
    , rightclickmenu_(*new uiMenu(this))
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
    delete &rightclickmenu_;
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
	pv->key_ = providerpars.getKey( ipar );
	providerpars.get( pv->key_, pv->aliases_ );
	pv->name_ = pv->aliases_.get(0);
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
	const bool isod = data.info_
		&& FixedString(data.info_->packagename_) == "OpendTect";
	if ( havesla && issel && !isod )
	{
	    int pkgidx = getPackageIndex( data.info_->packagename_ );
	    int providx = getProviderIndex( data.info_->creator_ );
	    PluginProvider* pprov = 0;
	    if ( providx >= 0 )
		pprov = providers_[providx];
	    else
	    {
		pprov = new PluginProvider;
		providers_ += pprov;
		providx = providers_.size() - 1;
	    }
	    pprov->nrplugins_++;

	    if ( pprov->name_.isEmpty() )
		pprov->name_ = data.info_->creator_;
	    if ( pprov->name_.isEmpty() )
		pprov->name_.set( "Unknown Provider" );
	    else if ( pprov->name_ == "opendtect.org" )
	    {
		pprov->key_ = "od";
		pprov->url_ = pprov->name_;
	    }
	    if ( pprov->key_.isEmpty() )
		pprov->key_ = "unknownpersons";
	    pprov->aliases_.addIfNew( pprov->name_ );

	    if ( pkgidx < 0 )
	    {
		PluginPackage* package = new PluginPackage;
		package->name_ = data.info_->packagename_;
		packages_ += package;
		pkgidx = packages_.size() - 1;
	    }

	    PluginPackage& pkg = *packages_[pkgidx];
	    pkg.prov_ = pprov;
	    pkg.version_ = data.version();
	    pkg.url_ = data.info_->url_;
	    pkg.plugins_.addIfNew( getCleanPluginName(data.info_->dispname_) );
	    pkg.creator_ = data.info_->creator_;
	    const BufferString modulenm = PIM().moduleName( data.name_ );
	    pkg.libs_.addIfNew( modulenm );
	    pkg.isselected_ = !dontloadlist.isPresent( modulenm );

	    if ( pprov->url_.isEmpty() )
		pprov->url_ = pkg.url_;
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
    pmitem_ = scene().addItem( new uiPixmapItem(pm_) );
    setPrefWidth( pm_.width() );
    setPrefHeight( pm_.height() );
    setStretch( 2, 0 );
    setBackgroundColor( Color(192,192,192) );

    mAttachCB( reDrawn, uiPluginSelBannerDrawer::reDrawnCB );
}

~uiPluginSelBannerDrawer()
{
    detachAllNotifiers();
}

void reDrawnCB( CallBacker* )
{
    const int parwdth = scene().maxX();
    const int itmwdth = pmitem_->pixmapSize().width();
    if ( parwdth < 1 || itmwdth < 1 )
	{ pErrMsg("Huh"); return; }

    const float diff = (float)(parwdth - itmwdth);
    pmitem_->setPos( diff * .25f, 0.f );
}

    const uiPixmap	pm_;
    uiPixmapItem*	pmitem_;

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

    treefld_ = new uiTreeView( grp, "Plugin tree" );
    treefld_->showHeader( false );
    treefld_->attach( ensureBelow, sep );
    int height = 0;
    for ( int iprov=0; iprov<providers_.size(); iprov++ )
    {
	const PluginProvider& prov = *providers_[iprov];
	if ( prov.nrplugins_ < 1 )
	    continue;

	uiProviderTreeItem* provitem = new uiProviderTreeItem( treefld_,
			    prov, isProviderSelected(iprov) );
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

    treefld_->expandAll();
    treefld_->setPrefWidth( banner->pm_.width() );
    treefld_->setPrefHeightInChar( prefheight );
    treefld_->setStretch( 2, 2 );
    mAttachCB( treefld_->rightButtonPressed, uiPluginSel::rightClickCB );

    setPrefWidth( banner->pm_.width() );
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


void uiPluginSel::rightClickCB( CallBacker* )
{
    uiTreeViewItem* itm = treefld_->itemNotified();
    mDynamicCastGet( uiPackageTreeItem*, pkgitm, itm );
    mDynamicCastGet( uiProviderTreeItem*, provitm, itm );
    if ( pkgitm )
	doPkgMnu( pkgitm->pkg_ );
    else if ( provitm )
	doProvMnu( provitm->prov_ );
}


bool uiPluginSel::fillRightClickMenu( const BufferString& url, bool withinfo )
{
    rightclickmenu_.clear();
    if ( withinfo )
	rightclickmenu_.insertAction( new uiAction(tr("Info"), "info"), 0 );
    if ( !url.isEmpty() )
	rightclickmenu_.insertAction( new uiAction(tr("Web site [%1]")
			    .arg(url), "link"), 1 );
    return !rightclickmenu_.isEmpty();
}


void uiPluginSel::launchURL( const char* url )
{
    uiDesktopServices::openUrl( url );
}


void uiPluginSel::doPkgMnu( const PluginPackage& pkg )
{
    if ( !fillRightClickMenu(pkg.url_,true) )
	return;

    const int res = rightclickmenu_.exec();
    if ( res == 1 )
	launchURL( pkg.url_ );
    else if ( res == 0 )
    {
	uiString msg( tr("Name: %1\nCreated by: %2\nVersion: %3")
		.arg( pkg.name_ ).arg( pkg.creator_ ).arg( pkg.version_ ) );
	if ( pkg.prov_ )
	    msg.append( tr("\n\nProvided by: %1").arg( pkg.prov_->name_ ) );
	uiMSG().about( msg );
    }
}


void uiPluginSel::doProvMnu( const PluginProvider& prov )
{
    if ( !fillRightClickMenu(prov.url_,false) )
	return;

    const int res = rightclickmenu_.exec();
    if ( res == 1 )
	launchURL( prov.url_ );
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
