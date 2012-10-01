/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Bril
 Date:          Oct 2003
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id: uipluginman.cc,v 1.34 2012/09/11 10:57:13 cvsbert Exp $";

#include "uipluginman.h"
#include "uipluginsel.h"
#include "uilistbox.h"
#include "uitextedit.h"
#include "uibutton.h"
#include "uifiledlg.h"
#include "uimsg.h"
#include "plugins.h"
#include "oddirs.h"
#include "envvars.h"
#include "file.h"
#include "separstr.h"
#include "filepath.h"
#include "settings.h"
#include "odver.h"
#include <iostream>


uiPluginMan::uiPluginMan( uiParent* p )
	: uiDialog(p,Setup("Plugins",mNoDlgTitle,"0.2.5"))
{
    setCtrlStyle( uiDialog::LeaveOnly );
    uiGroup* leftgrp = new uiGroup( this, "Left group" );
    listfld = new uiListBox( leftgrp, "Plugin list" );
    listfld->setPrefWidthInChar( 25 );
    fillList();
    listfld->selectionChanged.notify( mCB(this,uiPluginMan,selChg) );

    uiPushButton* loadbut = new uiPushButton( leftgrp, " &Load a plugin ",
	    			mCB(this,uiPluginMan,loadPush), false );
    loadbut->attach( alignedBelow, listfld );
    selatstartfld = new uiCheckBox( leftgrp, "Select auto-loaded at startup" ); 
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
	    early.add( data.info_->dispname );
	else
	    late.add( data.info_->dispname );

    }
    early.sort(); late.sort(); notloaded.sort();
    listfld->addItems( late );
    listfld->addItem( "" );
    listfld->addItem( "" );
    listfld->addItem( "- Base plugins" );
    listfld->addItem( "" );
    listfld->addItems( early );
    if ( !notloaded.isEmpty() )
    {
	listfld->addItem( "" );
	listfld->addItem( "" );
	listfld->addItem( "- Not loaded" );
	listfld->addItem( "" );
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

    if ( !data )
	{ infofld->setText( txt ); return; }

    const PluginInfo& piinf = *data->info_;
    txt += "Created by: "; txt += piinf.creator;
    txt += "\n\nFilename: "; txt += PIM().getFileName( *data );
    if ( piinf.version && *piinf.version )
    {
	txt += "\nVersion: ";

	if ( *piinf.version != '=' )
	    txt += piinf.version;
	else
	{
	    BufferString ver; GetSpecificODVersion( 0, ver );
	    txt += ver;
	}
    }

    txt += "\n-----------------------------------------\n\n";
    txt += piinf.text;
    infofld->setText( txt );
}


void uiPluginMan::loadPush( CallBacker* )
{
#ifdef __win__
    static const char* filt = "*.DLL;;*.*";
    static const char* captn = "Select plugin DLL";
#else
    static const char* captn = "Select plugin shared library";
# ifdef __mac__
    static const char* filt = "*.dylib*;;*";
# else
    static const char* filt = "*.so*;;*";
# endif
#endif

    static BufferString loaddir;
    if ( loaddir.isEmpty() )
    {
	loaddir = PIM().getAutoDir( true );
	if ( !File::exists(loaddir) )
	    loaddir = PIM().getAutoDir( false );
    }

    uiFileDialog dlg( this, uiFileDialog::ExistingFile, loaddir, filt, captn );
    if ( !dlg.go() ) return;

    BufferString fnm = dlg.fileName();
    if ( !File::exists(fnm) )
	uiMSG().error( "File does not exist" );
    else if ( !PIM().load(fnm) )
	uiMSG().error( "Couldn't load plugin" );
    else
	{ loaddir = FilePath(fnm).pathOnly(); fillList(); selChg(0); }
}


bool uiPluginMan::rejectOK( CallBacker* )
{
    const bool oldyn = Settings::common().isTrue(uiPluginSel::sKeyDoAtStartup());
    const bool newyn = selatstartfld->isChecked();
    if ( oldyn != newyn )
    {
	Settings::common().setYN( uiPluginSel::sKeyDoAtStartup(), newyn );
	Settings::common().write();
    }
    return true;
}
