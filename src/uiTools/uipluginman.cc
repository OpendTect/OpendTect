/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Bril
 Date:          Oct 2003
________________________________________________________________________

-*/

#include "uipluginman.h"

#include "uibutton.h"
#include "uifileselector.h"
#include "uigeninput.h"
#include "uilabel.h"
#include "uimsg.h"
#include "uipluginsel.h"
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
#include "plugins.h"
#include "separstr.h"
#include "settings.h"

#include <iostream>

uiPluginMan::uiPluginMan( uiParent* p )
	: uiDialog(p,Setup(tr("Plugin Management"), mNoDlgTitle,
			    mODHelpKey(mPluginManHelpID) ) )
{
    setCtrlStyle( uiDialog::CloseOnly );
    uiGroup* leftgrp = new uiGroup( this, "Left group" );
    pluginview_ = new uiTreeView( leftgrp, "Plugin list" );
    pluginview_->showHeader( false );
    pluginview_->setHSzPol( uiObject::Wide );
    pluginview_->setStretch( 0, 2 );
    fillList();
    pluginview_->selectionChanged.notify( mCB(this,uiPluginMan,selChg) );
    pluginview_->returnPressed.notify( mCB(this,uiPluginMan,activateCB) );
    pluginview_->doubleClicked.notify( mCB(this,uiPluginMan,activateCB) );

    uiPushButton* loadbut = new uiPushButton( leftgrp, tr(" Load a plugin "),
				mCB(this,uiPluginMan,loadPush), false );
    loadbut->attach( alignedBelow, pluginview_ );

    selatstartfld_ = new uiCheckBox( leftgrp,
				    tr("Select auto-loaded at startup") );
    selatstartfld_->attach( alignedBelow, loadbut );
    selatstartfld_->setChecked(
	    Settings::common().isTrue(uiPluginSel::sKeyDoAtStartup()) );

    uiGroup* rightgrp = new uiGroup( this, "Right group" );
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
    uiLabel* infolbl = new uiLabel( rightgrp, uiStrings::sInformation() );
    infolbl->attach( leftOf, infofld_ );

    licensefld_ = new uiTextBrowser( rightgrp, "License" );
    licensefld_->attach( alignedBelow, infofld_ );
    licensefld_->setPrefHeightInChar( 10 );
    licensefld_->setPrefWidth( 10 );
    uiLabel* liclbl = new uiLabel( rightgrp, tr("License") );
    liclbl->attach( leftOf, licensefld_ );

    postFinalise().notify( mCB(this,uiPluginMan,selChg) );
}


struct PluginProduct
{
PluginProduct(const char* nm)
    : name_(nm)	{}

void add( const char* nm )
{ pluginnms_.add( nm ); }

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
    const File::Path prodlistfp( mGetSWDirDataDir(), "prodlist.txt" );
    od_istream prodstrm( prodlistfp.fullPath() ) ;
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
    PluginProduct* notloaded = new PluginProduct( "Not loaded" );
    ObjectSet<PluginProduct> productlist;
    productlist.add( new PluginProduct("OpendTect") );
    productlist[0]->iconnm_ = "opendtect";
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
	if ( *piinf.version_ != '=' )
	    vtxt += piinf.version_;
	else
	    GetSpecificODVersion( nullptr, vtxt );
	versionfld_->setText( vtxt );
    }

    infofld_->setText( piinf.text_ );

    BufferString licmsg;
    if ( piinf.lictype_ == PluginInfo::GPL )
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
    const uiString& captn = uiStrings::phrSelect(tr("plugin DLL"));
#else
    const uiString& captn = uiStrings::phrSelect(tr("plugin shared library"));
#endif

    mDefineStaticLocalObject( BufferString, loaddir, )
    if ( loaddir.isEmpty() )
    {
	loaddir = PIM().getAutoDir( true );
	if ( !File::exists(loaddir) )
	    loaddir = PIM().getAutoDir( false );
    }

    uiFileSelector::Setup fssu( OD::SelectFileForRead );
    fssu.initialselectiondir( loaddir )
	.setFormat( File::Format::shlibFiles() )
	.onlylocal( true );
    uiFileSelector uifs( this, fssu );
    uifs.caption() = captn;
    if ( !uifs.go() ) return;

    const BufferString fnm = uifs.fileName();
    if ( !File::exists(fnm) )
	uiMSG().error( uiStrings::phrFileDoesNotExist(fnm) );
    else if ( !PIM().load(fnm) )
	uiMSG().error( tr("Couldn't load plugin") );
    else
    {
	loaddir = File::Path(fnm).pathOnly();
	fillList();
	selChg(nullptr);
    }
}


bool uiPluginMan::rejectOK()
{
    const bool oldyn =
	Settings::common().isTrue(uiPluginSel::sKeyDoAtStartup());
    const bool newyn = selatstartfld_->isChecked();
    if ( oldyn != newyn )
    {
	Settings::common().setYN( uiPluginSel::sKeyDoAtStartup(), newyn );
	Settings::common().write();
    }
    return true;
}
