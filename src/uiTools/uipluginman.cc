/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Bril
 Date:          Oct 2003
________________________________________________________________________

-*/

#include "uipluginman.h"
#include "uilistbox.h"
#include "uitextedit.h"
#include "uisplitter.h"
#include "uiodprestart.h"
#include "uibutton.h"
#include "uifileselector.h"
#include "uifileselector.h"
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
    listfld->setPrefWidthInChar( 40 );
    fillList();
    listfld->selectionChanged.notify( mCB(this,uiPluginMan,selChg) );

    uiPushButton* loadbut = new uiPushButton( leftgrp, tr(" Load a plugin "),
				mCB(this,uiPluginMan,loadPush), false );
    loadbut->setIcon( "import" );
    loadbut->attach( alignedBelow, listfld );
    selatstartfld = new uiCheckBox( leftgrp,
                                    tr("Select auto-loaded at startup") );
    selatstartfld->attach( alignedBelow, loadbut );
    selatstartfld->setChecked(
	    Settings::common().isTrue(uiODPreStart::sKeyDoAtStartup()) );

    uiGroup* rightgrp = new uiGroup( this, "Right group" );
    infofld = new uiTextEdit( rightgrp, "Info" );
    infofld->setPrefWidthInChar( 70 );
    infofld->setPrefHeightInChar( 30 );

    uiSplitter* splitter = new uiSplitter( this );
    splitter->addGroup( leftgrp );
    splitter->addGroup( rightgrp );

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
    listfld->addItems( late.getUiStringSet() );
    listfld->addItem( uiString::empty() );
    listfld->addItem( uiString::empty() );
    listfld->addItem( tr("- Base plugins") );
    listfld->addItem( uiString::empty() );
    listfld->addItems( early.getUiStringSet() );
    if ( !notloaded.isEmpty() )
    {
	listfld->addItem( uiString::empty() );
	listfld->addItem( uiString::empty() );
	listfld->addItem( tr("- Not loaded") );
	listfld->addItem( uiString::empty() );
	listfld->addItems( notloaded.getUiStringSet() );
    }
    if ( listfld->size() )
	listfld->setCurrentItem( 0 );
}


static bool needDispPkgName( const BufferString& pkgnm, BufferString usrnm )
{
    if ( pkgnm.isEmpty() || pkgnm == usrnm || pkgnm == "OpendTect" )
	return false;

    char* vendornm = firstOcc( usrnm.getCStr(), '[' );
    if ( !vendornm )
	return true;
    *(vendornm-1) = '\0';
    return pkgnm != usrnm;
}


static BufferString getCleanDispName( const char* nm )
{
    BufferString ret( nm );
    char* ptr = ret.findLast( '[' );
    if ( ptr )
	*ptr = '\0';
    ret.trimBlanks();
    return ret;
}


void uiPluginMan::selChg( CallBacker* )
{
    const char* nm = listfld->getText();
    if ( !nm || !*nm || *nm == '-' )
	{ infofld->setText( "" ); return; }

    BufferString txt;
    const PluginManager::Data* data = 0;
    if ( *nm != '-' || *(nm+1) != '-' )
	data = PIM().findDataWithDispName( nm );

    if ( !data )
	txt.set( "This plugin was not loaded" );
    else
    {
	const PluginInfo& piinf = *data->info_;
	txt.set( "** " ).add( getCleanDispName(piinf.dispname_) ).add( " **" );
	txt.add( "\n\nCreated by: " ).add( piinf.creator_ );
	if ( needDispPkgName(piinf.packagename_,piinf.dispname_) )
	    txt.add( "\nPackage: " ).add( piinf.packagename_ );

	txt.add( "\n\nFilename: " ).add( PIM().getFileName(*data) );
	txt.add( "\nVersion: " ).add( data->version() );
	txt.add( "\n\n-----------------------------------------\n\n" )
	   .add( piinf.text_ );
    }

    infofld->setText( txt );
}


void uiPluginMan::loadPush( CallBacker* )
{
    mDefineStaticLocalObject( BufferString, loaddir, );
    if ( loaddir.isEmpty() )
    {
	loaddir = PIM().getAutoDir( true );
	if ( !File::exists(loaddir) )
	    loaddir = PIM().getAutoDir( false );
    }

    uiFileSelector::Setup fssu;
    fssu.initialselectiondir( loaddir )
	.setFormat( File::Format::shlibFiles() )
	.onlylocal( true );
    uiFileSelector uifs( this, fssu );
    if ( !uifs.go() )
	return;

    const BufferString fnm = uifs.fileName();
    if ( !File::exists(fnm) )
	uiMSG().error( uiStrings::phrFileDoesNotExist(fnm) );
    else if ( !PIM().load(fnm) )
	uiMSG().error( tr("Could not load plugin") );
    else
	{ loaddir = File::Path(fnm).pathOnly(); fillList(); selChg(0); }
}


bool uiPluginMan::rejectOK()
{
    const bool oldyn =
	Settings::common().isTrue(uiODPreStart::sKeyDoAtStartup());
    const bool newyn = selatstartfld->isChecked();
    if ( oldyn != newyn )
    {
	Settings::common().setYN( uiODPreStart::sKeyDoAtStartup(), newyn );
	Settings::common().write();
    }
    return true;
}
