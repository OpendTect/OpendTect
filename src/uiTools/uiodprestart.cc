/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Bril
 Date:          Feb 2006
________________________________________________________________________

-*/

#include "uiodprestart.h"
#include "uidesktopservices.h"
#include "uigraphicsviewbase.h"
#include "uigraphicsitemimpl.h"
#include "uigraphicsscene.h"
#include "uigroup.h"
#include "uilabel.h"
#include "uilanguagesel.h"
#include "uimenu.h"
#include "uimsg.h"
#include "uipixmap.h"
#include "uiseparator.h"
#include "uithemesel.h"
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


static const char* sOldKeyDoAtStartup = "dTect.Select Plugins";
const char* uiODPreStart::sKeyDoAtStartup() { return "dTect.Show PreStart"; }


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
    uiString		dispname_;
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
    : uiTreeViewItem(p, Setup(pkg.dispname_)
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


uiODPreStart::uiODPreStart( uiParent* p )
    : uiDialog(p,Setup(uiString::empty(),mNoDlgTitle,
			mODHelpKey(mPluginSelHelpID) )
		    .savebutton(true)
		    .savetext(tr("Show this dialog at startup")))
    , rightclickmenu_(*new uiMenu(this))
    , languagesel_(0)
{
    setName( "OpendTect Pre-Start Window" );
    setCaption( tr("%1 V%2").arg(uiStrings::sOpendTect())
			    .arg(GetFullODVersion()) );
    setOkText( uiStrings::phrStart(uiStrings::sOpendTect()) );
    readProviderList();
    setSaveButtonChecked( true );
    readPackageList();
    makePackageList();
    createUI();
}


uiODPreStart::~uiODPreStart()
{
    deepErase( packages_ );
    deepErase( providers_ );
    delete &rightclickmenu_;
}

void uiODPreStart::readProviderList()
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


int uiODPreStart::getProviderIndex( const char* providernm ) const
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


void uiODPreStart::readPackageList()
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
	package->dispname_ = toUiString( package->name_ );
	package->pkgkey_ = sepline[1];
	packages_ += package;
    }
}


void uiODPreStart::makePackageList()
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
	    mGetPackageDisplayName( *data.info_, pkg.dispname_ );
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


class uiODPreStartBannerDrawer : public uiGraphicsViewBase
{
public:

uiODPreStartBannerDrawer( uiGroup* p )
    : uiGraphicsViewBase( p, "OD Version banner" )
    , pm_("banner.png")
{
    pmitem_ = scene().addItem( new uiPixmapItem(pm_) );
    setPrefWidth( pm_.width() );
    setPrefHeight( pm_.height() );
    setStretch( 2, 0 );
    setBackgroundColor( Color(192,192,192) );

    mAttachCB( reDrawn, uiODPreStartBannerDrawer::reDrawnCB );
}

~uiODPreStartBannerDrawer()
{
    detachAllNotifiers();
}

void reDrawnCB( CallBacker* )
{
    const int parwdth = mNINT32( scene().maxX() );
    const int itmwdth = pmitem_->pixmapSize().width();
    if ( parwdth < 1 || itmwdth < 1 )
	{ pErrMsg("Huh"); return; }

    const float diff = (float)(parwdth - itmwdth);
    pmitem_->setPos( diff * .25f, 0.f );
}

    const uiPixmap	pm_;
    uiPixmapItem*	pmitem_;

};


void uiODPreStart::createUI()
{
    uiGroup* grp = new uiGroup( this, "OD PreStart Main Group" );
    grp->setFrame( true );

    uiODPreStartBannerDrawer* banner = new uiODPreStartBannerDrawer( grp );
    uiSeparator* sep = new uiSeparator( grp );
    sep->attach( stretchedBelow, banner );

    uiGroup* presgrp = new uiGroup( grp, "Presentation group" );
    presgrp->attach( ensureBelow, sep );
    if ( uiLanguageSel::haveMultipleLanguages() )
	languagesel_ = new uiLanguageSel( presgrp, false );
    themesel_ = new uiThemeSel( presgrp, false );
    if ( languagesel_ )
	themesel_->attach( rightOf, languagesel_ );

    treefld_ = new uiTreeView( grp, "Plugin tree" );
    treefld_->showHeader( false );
    treefld_->attach( centeredBelow, presgrp );
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
    mAttachCB( treefld_->rightButtonPressed, uiODPreStart::rightClickCB );

    treefld_->attach( centeredBelow, presgrp );
    setPrefWidth( banner->pm_.width() );
}


int uiODPreStart::getPackageIndex( const char* pkgnm ) const
{
    for ( int ipkg=0; ipkg<packages_.size(); ipkg++ )
    {
	if ( packages_[ipkg]->name_ == pkgnm )
	    return ipkg;
    }

    return -1;
}


bool uiODPreStart::isProviderSelected( int provideridx ) const
{
    for ( int ipkg=0; ipkg<packages_.size(); ipkg++ )
    {
	if ( getProviderIndex(packages_[ipkg]->creator_) == provideridx
		&& packages_[ipkg]->isselected_ )
	    return true;
    }

    return false;
}


void uiODPreStart::rightClickCB( CallBacker* )
{
    uiTreeViewItem* itm = treefld_->itemNotified();
    mDynamicCastGet( uiPackageTreeItem*, pkgitm, itm );
    mDynamicCastGet( uiProviderTreeItem*, provitm, itm );
    if ( pkgitm )
	doPkgMnu( pkgitm->pkg_ );
    else if ( provitm )
	doProvMnu( provitm->prov_ );
}


bool uiODPreStart::fillRightClickMenu( const BufferString& url, bool withinfo )
{
    rightclickmenu_.clear();
    if ( withinfo )
	rightclickmenu_.insertAction( new uiAction(uiStrings::sInfo(),
				      "info"), 0 );
    if ( !url.isEmpty() )
	rightclickmenu_.insertAction( new uiAction(tr("Web site [%1]")
			    .arg(url), "link"), 1 );
    return !rightclickmenu_.isEmpty();
}


void uiODPreStart::launchURL( const char* url )
{
    uiDesktopServices::openUrl( url );
}


void uiODPreStart::doPkgMnu( const PluginPackage& pkg )
{
    if ( !fillRightClickMenu(pkg.url_,true) )
	return;

    const int res = rightclickmenu_.exec();
    if ( res == 1 )
	launchURL( pkg.url_ );
    else if ( res == 0 )
    {
	uiString msg( tr("Name: %1").arg(pkg.dispname_) );
	msg.appendPhrase(tr("Created by: %1").arg(pkg.creator_),
			    uiString::Empty,uiString::AddNewLine);
	msg.appendPhrase(tr("Version: %1").arg(pkg.version_),
			    uiString::Empty,uiString::AddNewLine);
	if ( pkg.prov_ )
	{
	    msg.appendPhrase( tr("Provided by: %1").arg( pkg.prov_->name_ ),
				    uiString::Empty, uiString::LeaveALine );
	}
	uiMSG().about( msg );
    }
}


void uiODPreStart::doProvMnu( const PluginProvider& prov )
{
    if ( !fillRightClickMenu(prov.url_,false) )
	return;

    const int res = rightclickmenu_.exec();
    if ( res == 1 )
	launchURL( prov.url_ );
}


bool uiODPreStart::acceptOK()
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

    themesel_->putInSettings( false );
    if ( languagesel_ )
	languagesel_->commit( false );

    Settings::common().removeWithKey( sOldKeyDoAtStartup );
    Settings::common().setYN( sKeyDoAtStartup(), saveButtonChecked() );
    Settings::common().set( PluginManager::sKeyDontLoad(), dontloadlist.rep() );
    Settings::common().write();

    return true;
}
