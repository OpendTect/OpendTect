/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Bril
 Date:          Oct 2003
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "uipluginman.h"

#include "uibutton.h"
#include "uifiledlg.h"
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

    infofld_ = new uiTextEdit( rightgrp, "Info", true );
    infofld_->attach( alignedBelow, filenmfld_ );
    infofld_->setPrefHeightInChar( 10 );
    infofld_->setPrefWidth( 10 );
    uiLabel* infolbl = new uiLabel( rightgrp, uiStrings::sInformation() );
    infolbl->attach( leftOf, infofld_ );

    licensefld_ = new uiTextEdit( rightgrp, "License", true );
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

    BufferString	name_;
    BufferStringSet	pluginnms_;
};


static int getProductIndex( ObjectSet<PluginProduct>& prods,
			    const char* prodnm )
{
    for ( int idx=0; idx<prods.size(); idx++ )
    {
	if ( prods[idx]->name_ == prodnm )
	    return idx;
    }

    prods.add( new PluginProduct(prodnm) );
    return prods.size()-1;
}


void uiPluginMan::fillList()
{
    pluginview_->setEmpty();

    const ObjectSet<PluginManager::Data>& lst = PIM().getData();
    PluginProduct* notloaded = new PluginProduct( "Not loaded" );
    ObjectSet<PluginProduct> productlist;
    productlist.add( new PluginProduct("OpendTect") );
    for ( int idx=0; idx<lst.size(); idx++ )
    {
	const PluginManager::Data& data = *lst[idx];
	if ( !data.info_ || !data.isloaded_ )
	    notloaded->add( data.name_ );
	else
	{
	    const int pidx = getProductIndex( productlist,
					      data.info_->productname_ );
	    productlist[pidx]->add( data.info_->dispname_ );
	}
    }

    productlist.add( notloaded );
    for ( int pidx=0; pidx<productlist.size(); pidx++ )
    {
	PluginProduct* prod = productlist[pidx];
	prod->pluginnms_.sort();
	uiTreeViewItem* pitm = new uiTreeViewItem( pluginview_,
				toUiString(prod->name_) );
	pitm->setSelectable( false );
	for ( int lidx=0; lidx<prod->pluginnms_.size(); lidx++ )
	    new uiTreeViewItem( pitm, toUiString(prod->pluginnms_.get(lidx)) );
    }
}


void uiPluginMan::emptyFields()
{
    productfld_->setEmpty();
    creatorfld_->setEmpty();
    filenmfld_->setEmpty();
    versionfld_->setEmpty();
    infofld_->setEmpty();
    licensefld_->setEmpty();
}


void uiPluginMan::selChg( CallBacker* )
{
    emptyFields();
    if ( !pluginview_->selectedItem() )
	return;

    const char* nm = pluginview_->selectedItem()->text();
    if ( !nm || !*nm || *nm == '-' )
	return;

    BufferString txt;
    const PluginManager::Data* data = nullptr;
    if ( *nm != '-' || *(nm+1) != '-' )
	data = PIM().findDataWithDispName( nm );

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


bool uiPluginMan::rejectOK( CallBacker* )
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
