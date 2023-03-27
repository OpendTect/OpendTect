/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uipluginman.h"

#include "uibutton.h"
#include "uifiledlg.h"
#include "uigeninput.h"
#include "uilabel.h"
#include "uimsg.h"
#include "uistrings.h"
#include "uitextedit.h"
#include "uitreeview.h"

#include "envvars.h"
#include "file.h"
#include "filepath.h"
#include "oddirs.h"
#include "odver.h"
#include "od_helpids.h"
#include "od_istream.h"
#include "odinst.h"
#include "plugins.h"
#include "separstr.h"
#include "settings.h"

#include <iostream>

static const FilePath& getProdListFP( const FilePath* newfp )
{
    static FilePath prodlistfp_( mGetSWDirDataDir(), "prodlist.txt" );
    if ( newfp && newfp->exists() )
	prodlistfp_ = *newfp;

    return prodlistfp_;
}

uiPluginMan::uiPluginMan( uiParent* p )
	: uiDialog(p,Setup(tr("Plugin Management"), mNoDlgTitle,
			    mODHelpKey(mPluginManHelpID) ) )
{
    setCtrlStyle( uiDialog::CloseOnly );
    auto* leftgrp = new uiGroup( this, "Left group" );
    pluginview_ = new uiTreeView( leftgrp, "Plugin list" );
    pluginview_->showHeader( false );
    pluginview_->setHSzPol( uiObject::Wide );
    pluginview_->setStretch( 0, 2 );
    fillList();
    mAttachCB( pluginview_->selectionChanged, uiPluginMan::selChg );
    mAttachCB( pluginview_->returnPressed, uiPluginMan::activateCB );
    mAttachCB( pluginview_->doubleClicked, uiPluginMan::activateCB );

    auto* loadbut = new uiPushButton( leftgrp, tr(" Load a plugin "),
				mCB(this,uiPluginMan,loadPush), false );
    loadbut->attach( alignedBelow, pluginview_ );
    unloadbut_ = new uiPushButton( leftgrp, tr(" Unload a plugin "),
				   mCB(this,uiPluginMan,unLoadPush), false );
    unloadbut_->attach( rightOf, loadbut );

    auto* rightgrp = new uiGroup( this, "Right group" );
    rightgrp->attach( rightOf, leftgrp );

    namefld_ = new uiGenInput( rightgrp, tr("Plugin name") );
    namefld_->setElemSzPol( uiObject::Wide );
    namefld_->setReadOnly();

    productfld_ = new uiGenInput( rightgrp, tr("Product") );
    productfld_->setReadOnly();
    productfld_->setElemSzPol( uiObject::Wide );
    productfld_->attach( alignedBelow, namefld_ );

    creatorfld_ = new uiGenInput( rightgrp, tr("Created by") );
    creatorfld_->setReadOnly();
    creatorfld_->setElemSzPol( uiObject::Wide );
    creatorfld_->attach( rightTo, productfld_ );

    filenmfld_ = new uiGenInput( rightgrp, tr("Library name") );
    filenmfld_->setReadOnly();
    filenmfld_->setElemSzPol( uiObject::Wide );
    filenmfld_->attach( alignedBelow, productfld_ );

    versionfld_ = new uiGenInput( rightgrp, tr("Version") );
    versionfld_->setReadOnly();
    versionfld_->setElemSzPol( uiObject::Wide );
    versionfld_->attach( alignedBelow, creatorfld_ );
    versionfld_->attach( ensureRightOf, filenmfld_ );

    infofld_ = new uiTextBrowser( rightgrp, "Info" );
    infofld_->attach( alignedBelow, filenmfld_ );
    infofld_->setPrefHeightInChar( 10 );
    infofld_->setPrefWidth( 10 );
    auto* infolbl = new uiLabel( rightgrp, uiStrings::sInformation() );
    infolbl->attach( leftOf, infofld_ );

    licensefld_ = new uiTextBrowser( rightgrp, "License" );
    licensefld_->attach( alignedBelow, infofld_ );
    licensefld_->setPrefHeightInChar( 10 );
    licensefld_->setPrefWidth( 10 );
    auto* liclbl = new uiLabel( rightgrp, tr("License") );
    liclbl->attach( leftOf, licensefld_ );

    mAttachCB( postFinalize(), uiPluginMan::selChg );
}


uiPluginMan::~uiPluginMan()
{
    detachAllNotifiers();
}


struct PluginProduct
{
PluginProduct( const char* nm )
    : name_(nm)
{
}

void add( const char* nm )
{
    pluginnms_.add( nm );
}

void setIcon( const char* iconnm )
{
    if ( iconnm_.isEmpty() )
	iconnm_ = iconnm;
}

    BufferString	name_;
    BufferString	iconnm_;
    BufferStringSet	pluginnms_;
};


static int getProductIndex( ObjectSet<PluginProduct>& prods,
			    const char* pnm, bool createnew )
{
    BufferString prodnm = pnm;
    if ( prodnm.isEmpty() )
	prodnm = "Other";

    for ( int idx=0; idx<prods.size(); idx++ )
    {
	if ( prods[idx]->name_ == prodnm )
	    return idx;
    }

    if ( !createnew )
	return -1;

    prods.add( new PluginProduct(prodnm) );
    return prods.size()-1;
}


static void setIcons( ObjectSet<PluginProduct>& products )
{
    od_istream prodstrm( getProdListFP(nullptr).fullPath() ) ;
    while ( prodstrm.isOK() )
    {
	BufferString line;
	prodstrm.getLine( line );
	const FileMultiString sepline( line );
	const int pidx = getProductIndex( products, sepline[0], false );
	if ( pidx < 0 )
	    continue;

	products[pidx]->setIcon( sepline[1] );
    }
}



void uiPluginMan::fillList()
{
    pluginview_->setEmpty();

    const ObjectSet<PluginManager::Data>& lst = PIM().getData();
    auto* notloaded = new PluginProduct( "Not loaded" );
    ObjectSet<PluginProduct> productlist;
    productlist.add( new PluginProduct("OpendTect") );
    productlist.add( new PluginProduct("OpendTect Pro") );
    productlist[0]->iconnm_ = "opendtect";

    BufferStringSet productnms;
    for ( auto* pmdata : lst )
    {
	if ( pmdata->info_ && pmdata->isloaded_ )
	    productnms.add( pmdata->info_->productname_ );
    }

    productnms.sort();
    for ( auto* nm : productnms )
	getProductIndex( productlist, nm->buf(), true );

    for ( int idx=0; idx<lst.size(); idx++ )
    {
	const PluginManager::Data& data = *lst[idx];
	if ( !data.info_ || !data.isloaded_ )
	    notloaded->add( data.name_ );
	else
	{
	    const int pidx = getProductIndex( productlist,
					      data.info_->productname_, true );
	    productlist[pidx]->add( data.info_->dispname_ );
	}
    }

    productlist.add( notloaded );
    setIcons( productlist );

    for ( int pidx=0; pidx<productlist.size(); pidx++ )
    {
	PluginProduct* prod = productlist[pidx];
	prod->pluginnms_.sort();
	uiTreeViewItem* pitm = new uiTreeViewItem( pluginview_,
				toUiString(prod->name_) );
	pitm->setIcon( 0,
		prod->iconnm_.isEmpty() ? "plugin" : prod->iconnm_.buf() );
	for ( int lidx=0; lidx<prod->pluginnms_.size(); lidx++ )
	    new uiTreeViewItem( pitm, toUiString(prod->pluginnms_.get(lidx)) );
    }
}


void uiPluginMan::emptyFields()
{
    namefld_->setEmpty();
    productfld_->setEmpty();
    creatorfld_->setEmpty();
    filenmfld_->setEmpty();
    versionfld_->setEmpty();
    infofld_->setEmpty();
    licensefld_->setEmpty();
}


void uiPluginMan::activateCB( CallBacker* )
{
    uiTreeViewItem* itm = pluginview_->itemNotified();
    if ( !itm || itm->nrChildren()==0 )
	return;

    itm->setOpen( !itm->isOpen() );
}


void uiPluginMan::selChg( CallBacker* )
{
    emptyFields();
    unloadbut_->setSensitive( false );
    const uiTreeViewItem* itm = pluginview_->selectedItem();
    if ( !itm )
	return;

    const BufferString nm = itm->text();
    if ( nm.isEmpty() )
	return;

    if ( itm->nrChildren() > 0 )
    {
	productfld_->setText( nm );
	infofld_->setText( "Expand tree item to see information on the plugin "
			   "libraries which belong to this product" );
	return;
    }

    const PluginManager::Data* data = PIM().findDataWithDispName( nm );
    if ( !data )
    {
	infofld_->setText( "This plugin was not loaded" );
	return;
    }

    const PluginInfo& piinf = *data->info_;
    namefld_->setText( piinf.dispname_ );
    productfld_->setText( piinf.productname_ );
    creatorfld_->setText( piinf.creator_ );
    filenmfld_->setText( data->name_ );
    if ( piinf.version_ && *piinf.version_ )
    {
	BufferString vtxt;
	if ( *piinf.version_ == '=' )
	{
	    GetSpecificODVersion( nullptr, vtxt );
	    if ( vtxt.isEmpty() )
		vtxt.set( ODInst::getPkgVersion(nullptr) );
	}
	else
	    vtxt.set( piinf.version_ );

	versionfld_->setText( vtxt );
    }

    infofld_->setText( piinf.text_ );
    const bool isfreeplugin = piinf.lictype_ == PluginInfo::GPL;
    unloadbut_->setSensitive( isDeveloperBuild() || isfreeplugin );

    BufferString licmsg;
    if ( isfreeplugin )
	licmsg = "Plugin released under GPL license";
    else
    {
	licmsg = "Commercial Plugin.\n\n";
	licmsg += piinf.licmsg_;
    }
    licensefld_->setText( licmsg );
}


void uiPluginMan::loadPush( CallBacker* )
{
#ifdef __win__
    const char* filt = "*.DLL;;*.*";
    const uiString& captn = uiStrings::phrSelect(tr("plugin DLL"));
#else
    const uiString& captn = uiStrings::phrSelect(tr("plugin shared library"));
# ifdef __mac__
    const char* filt = "*.dylib*;;*";
# else
    const char* filt = "*.so*;;*";
# endif
#endif

    mDefineStaticLocalObject( BufferString, loaddir, )
    if ( loaddir.isEmpty() )
    {
	loaddir = PIM().getAutoDir( true );
	if ( !File::exists(loaddir) )
	    loaddir = PIM().getAutoDir( false );
    }

    uiFileDialog dlg( this, uiFileDialog::ExistingFile, loaddir, filt,
		      captn );
    if ( !dlg.go() ) return;

    BufferString fnm = dlg.fileName();
    if ( !File::exists(fnm) )
	uiMSG().error( uiStrings::sFileDoesntExist() );
    else if ( !PIM().load(fnm) )
	uiMSG().error( tr("Couldn't load plugin") );
    else
    {
	loaddir = FilePath(fnm).pathOnly();
	fillList();
	selChg(nullptr);
    }
}


void uiPluginMan::unLoadPush( CallBacker* )
{
    uiTreeViewItem* itm = pluginview_->selectedItem();
    if ( !itm || itm->nrChildren() > 0 )
    {
	uiMSG().error( tr("Please select a plugin to unload") );
	return;
    }

    const BufferString plugindispnm = itm->text();
    const PluginManager::Data* data =
			PIM().findDataWithDispName( plugindispnm.buf() );
    if ( !data || !data->info_ )
    {
	uiMSG().error( tr("Cannot find Plugin to unload") );
	return;
    }
    else if ( !data->isloaded_ || !data->sla_ )
    {
	uiMSG().error( tr("The plugin does not appear to be loaded") );
	return;
    }

    if ( !PIM().unload(data->name_) )
	return;

    fillList();
    selChg( nullptr );
}



bool uiPluginMan::rejectOK( CallBacker* )
{
    return true;
}



static SharedLibAccess* prodloader_ = nullptr;

static bool doBasicProdSelFn( bool& skippluginsel, uiRetVal& msgs )
{
    BufferString libnm( 256, false );
    SharedLibAccess::getLibName( "uidGBTools", libnm.getCStr(),libnm.bufSize());
    const FilePath fp( GetLibPlfDir(), libnm );
    if ( !fp.exists() )
    {
	msgs.setOK(); //PRO not installed
	return true;
    }

    delete prodloader_;
    prodloader_ = new SharedLibAccess( fp.fullPath() );
    if ( !prodloader_ || !prodloader_->isOK() )
    {
	msgs.add( od_static_tr("doBasicProdSelFn",
			       "Cannot load uidGBTools library") );
	if ( prodloader_ )
	{
	    msgs.add( toUiString(prodloader_->errMsg()) );
	    prodloader_->close();
	}
	deleteAndNullPtr( prodloader_ );
	return false;
    }

    using VoidVoidFn = void(*)(void);
    VoidVoidFn preinitfn =
	    (VoidVoidFn)prodloader_->getFunction( "PreInituidGBToolsPlugin" );
    if ( preinitfn )
	(*preinitfn)();
    else
	msgs.add( od_static_tr("doBasicProdSelFn",
		    "Cannot load uidGBTools library") );

    return preinitfn;
}


using boolFrombooluiRetValFn = bool(*)(bool&,uiRetVal&);
static boolFrombooluiRetValFn prodselfn_ = doBasicProdSelFn;

mGlobal(uiTools) void setGlobal_uiTools_Fns(boolFrombooluiRetValFn);
void setGlobal_uiTools_Fns( boolFrombooluiRetValFn prodselfn )
{
    prodselfn_ = prodselfn;
}


extern "C" {
    mGlobal(uiTools) bool doProductSelection(bool&,uiRetVal&);
}

mExternC(uiTools) bool doProductSelection( bool& skippluginsel,
					   uiRetVal& msgs )
{
    if ( !doBasicProdSelFn(skippluginsel,msgs) )
	return false;

    const bool res = prodselfn_ == doBasicProdSelFn
		   ? true : (*prodselfn_)(skippluginsel,msgs);
    if ( prodloader_ )
	prodloader_->close();
    deleteAndNullPtr( prodloader_ );

    return res;
}

extern "C" {
    mGlobal(uiTools) void setProdSelFp(const FilePath&);
}

mExternC(uiTools) void setProdSelFp( const FilePath& fp )
{
    getProdListFP( &fp );
}
