/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Bril
 Date:          Oct 2003
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "uipluginman.h"
#include "uipluginsel.h"
#include "uilistbox.h"
#include "uitextedit.h"
#include "uibutton.h"
#include "uifiledlg.h"
#include "uimsg.h"
#include "uistrings.h"
#include "plugins.h"
#include "oddirs.h"
#include "envvars.h"
#include "file.h"
#include "separstr.h"
#include "filepath.h"
#include "settings.h"
#include "odver.h"
#include "od_helpids.h"

#include <iostream>

uiPluginMan::uiPluginMan( uiParent* p )
	: uiDialog(p,Setup(tr("Plugins"), mNoDlgTitle,
			    mODHelpKey(mPluginManHelpID) ) )
{
    setCtrlStyle( uiDialog::CloseOnly );
    uiGroup* leftgrp = new uiGroup( this, "Left group" );
    listfld = new uiListBox( leftgrp, "Plugin list" );
    listfld->setPrefWidthInChar( 25 );
    fillList();
    listfld->selectionChanged.notify( mCB(this,uiPluginMan,selChg) );

    uiPushButton* loadbut = new uiPushButton( leftgrp, tr(" Load a plugin "),
				mCB(this,uiPluginMan,loadPush), false );
    loadbut->attach( alignedBelow, listfld );
    selatstartfld = new uiCheckBox( leftgrp,
				    tr("Select auto-loaded at startup") );
    selatstartfld->attach( alignedBelow, loadbut );
    selatstartfld->setChecked(
	    Settings::common().isTrue(uiPluginSel::sKeyDoAtStartup()) );

    infofld = new uiTextEdit( this, "Info" );
    infofld->attach( rightOf, leftgrp );
    infofld->setPrefWidthInChar( 70 );
    infofld->setPrefHeightInChar( 20 );

    postFinalise().notify( mCB(this,uiPluginMan,selChg) );
}


void uiPluginMan::fillList()
{
    listfld->setEmpty();
    const ObjectSet<PluginManager::Data>& lst = PIM().getData();
    BufferStringSet early, late, notloaded;

    for ( int idx=0; idx<lst.size(); idx++ )
    {
	const PluginManager::Data& data = *lst[idx];
	if ( !data.info_ || !data.isloaded_ )
	    notloaded.add( data.name_ );
	else if ( data.autotype_ == PI_AUTO_INIT_EARLY )
	    early.add( data.info_->dispname_ );
	else
	    late.add( data.info_->dispname_ );

    }
    early.sort(); late.sort(); notloaded.sort();
    listfld->addItems( late );
    listfld->addItem( uiString::emptyString() );
    listfld->addItem( uiString::emptyString() );
    listfld->addItem( tr("- Base plugins") );
    listfld->addItem( uiString::emptyString() );
    listfld->addItems( early );
    if ( !notloaded.isEmpty() )
    {
	listfld->addItem( uiString::emptyString() );
	listfld->addItem( uiString::emptyString() );
	listfld->addItem( tr("- Not loaded") );
	listfld->addItem( uiString::emptyString() );
	listfld->addItems( notloaded );
    }
    if ( listfld->size() )
	listfld->setCurrentItem( 0 );
}


void uiPluginMan::selChg( CallBacker* )
{
    const char* nm = listfld->getText();
    if ( !nm || !*nm || *nm == '-' )
	{ infofld->setText( "" ); return; }

    BufferString txt;
    const PluginManager::Data* data = 0;
    if ( *nm != '-' || *(nm+1) != '-' )
    {
	data = PIM().findDataWithDispName( nm );
	if ( !data )
	    txt = "This plugin was not loaded";
    }

    if ( data )
    {
	const PluginInfo& piinf = *data->info_;
	txt.add( "Created by: " ).add( piinf.creator_ );
	txt.add( "\nProduct: " ).add( piinf.productname_ );
	txt.add( "\n\nFilename: " ).add( PIM().getFileName(*data) );
	if ( piinf.version_ && *piinf.version_ )
	{
	    txt += "\nVersion: ";

	    if ( *piinf.version_ != '=' )
		txt += piinf.version_;
	    else
	    {
		BufferString ver; GetSpecificODVersion( 0, ver );
		txt += ver;
	    }
	}
	txt.add( "\n-----------------------------------------\n\n" )
	    .add( piinf.text_ );
    }

    infofld->setText( txt );
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

    mDefineStaticLocalObject( BufferString, loaddir, );
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
	{ loaddir = FilePath(fnm).pathOnly(); fillList(); selChg(0); }
}


bool uiPluginMan::rejectOK( CallBacker* )
{
    const bool oldyn =
	Settings::common().isTrue(uiPluginSel::sKeyDoAtStartup());
    const bool newyn = selatstartfld->isChecked();
    if ( oldyn != newyn )
    {
	Settings::common().setYN( uiPluginSel::sKeyDoAtStartup(), newyn );
	Settings::common().write();
    }
    return true;
}
