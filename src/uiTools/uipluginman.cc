/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Bril
 Date:          Oct 2003
 RCS:           $Id: uipluginman.cc,v 1.7 2003-12-04 16:18:39 bert Exp $
________________________________________________________________________

-*/

#include "uipluginman.h"
#include "uilistbox.h"
#include "uitextedit.h"
#include "uibutton.h"
#include "uifiledlg.h"
#include "uimsg.h"
#include "plugins.h"
#include "filegen.h"
#include "strmprov.h"


uiPluginMan::uiPluginMan( uiParent* p )
	: uiDialog(p,Setup("Plugins",""))
{
    uiGroup* leftgrp = new uiGroup( this, "Left group" );
    listfld = new uiListBox( leftgrp, "Plugin list" );
    listfld->setPrefWidthInChar( 25 );
    fillList();
    listfld->selectionChanged.notify( mCB(this,uiPluginMan,selChg) );

    uiPushButton* loadbut = new uiPushButton( leftgrp, " Load a plugin ... ",
	    			mCB(this,uiPluginMan,loadPush) );
    loadbut->attach( alignedBelow, listfld );

    infofld = new uiTextEdit( this, "Info" );
    infofld->attach( rightOf, leftgrp );
    infofld->setPrefWidthInChar( 70 );
    infofld->setPrefHeightInChar( 15 );

    finaliseDone.notify( mCB(this,uiPluginMan,selChg) );
    setCancelText( "" );
}


void uiPluginMan::fillList()
{
    listfld->empty();
    const BufferStringSet& nms = PIM().loadedFileNames();
    for ( int idx=0; idx<nms.size(); idx++ )
	listfld->addItem( PIM().getInfo(nms.get(idx)).dispname );
    if ( nms.size() )
	listfld->setCurrentItem( 0 );
}


void uiPluginMan::selChg( CallBacker* )
{
    int curidx = listfld->currentItem();
    if ( curidx < 0 ) return;

    BufferString fnm = PIM().loadedFileNames().get(curidx);
    const PluginInfo& piinf = PIM().getInfo( fnm );

    BufferString txt;
    txt = "Filename: "; txt += fnm;
    txt += "\nCreated by: "; txt += piinf.creator;
    if ( piinf.version && *piinf.version )
    {
	txt += "\nVersion: ";
	if ( *piinf.version != '=' )
	    txt += piinf.version;
	else
	{
	    BufferString fnm( GetSoftwareDir() );
	    fnm = File_getFullPath( fnm, ".rel." );
	    fnm += piinf.version+1;
	    fnm += ".";
	    const char* plfenv = getenv( "binsubdir" );
	    if ( !plfenv ) plfenv = getenv( "HDIR" );
	    fnm += plfenv;
	    StreamData sd = StreamProvider( fnm ).makeIStream();
	    if ( !sd.usable() )
		txt += "<unknown>";
	    else
	    {
		char buf[80];
		sd.istrm->getline( buf, 80 );
		txt += buf;
	    }
	    sd.close();
	}
    }

    txt += "\n\n";
    txt += piinf.text;

    infofld->setText( txt );
}


void uiPluginMan::loadPush( CallBacker* )
{
#ifdef __win__
    static const char* filt = "*.DLL;;*.*";
    static const char* captn = "Select plugin DLL";
#else
    static const char* filt = "*.so*;;*";
    static const char* captn = "Select plugin shared library";
#endif

    static BufferString defdir;
    if ( defdir == "" )
	defdir = PIM().defDir(true);

    uiFileDialog dlg( this, uiFileDialog::ExistingFile, defdir, filt, captn );
    if ( !dlg.go() ) return;

    BufferString fnm = dlg.fileName();
    if ( !File_exists(fnm) )
	uiMSG().error( "File does not exist" );
    else if ( !PIM().load(fnm) )
	uiMSG().error( "Couldn't load plugin" );
    else
	{ defdir = File_getPathOnly(fnm); fillList(); selChg(0); }
}
