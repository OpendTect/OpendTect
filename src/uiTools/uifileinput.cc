/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Lammertink
 Date:          08/08/2000
 RCS:           $Id: uifileinput.cc,v 1.21 2004-01-12 15:02:03 bert Exp $
________________________________________________________________________

-*/

#include "uifileinput.h"
#include "uifiledlg.h"
#include "uilabel.h"
#include "uibutton.h"
#include "uigeninput.h"
#include "filegen.h"
#include "uifilebrowser.h"


uiFileInput::uiFileInput( uiParent* p, const char* txt, const Setup& setup )
    : uiGenInput( p, txt, FileNameInpSpec(setup.fnm) )
    , forread(setup.forread_)
    , fname(setup.fnm)
    , filter(setup.filter_)
    , newfltr(false)
    , selmodset(false)
    , selmode(uiFileDialog::AnyFile)
    , browser(0)
    , examinebut(0)
{
    setWithSelect( true );
    if ( setup.withexamine_ )
	examinebut = new uiPushButton( this, "Examine ...", 
					mCB(this,uiFileInput,examineFile) );
    if ( setup.dirs_ )
    {
	selmodset = true;
	selmode = uiFileDialog::DirectoryOnly;
    }

    mainObject()->finaliseDone.notify( mCB(this,uiFileInput,isFinalised) );
}


uiFileInput::uiFileInput( uiParent* p, const char* txt, const char* fnm )
    : uiGenInput( p, txt, FileNameInpSpec(fnm) )
    , forread(true)
    , fname(fnm)
    , filter("")
    , newfltr(false)
    , selmodset(false)
    , selmode(uiFileDialog::AnyFile)
    , browser(0)
    , examinebut(0)
{
    setWithSelect( true );
}


uiFileInput::~uiFileInput()
{
    if ( browser ) browser->reject(0);
}


void uiFileInput::isFinalised( CallBacker* )
{
    if ( examinebut )
	examinebut->attach( rightOf, selbut );
}


void uiFileInput::setFileName( const char* s )
{
    setText( s );
}


void uiFileInput::doSelect( CallBacker* )
{
    fname = text();
    if ( fname == "" )	fname = defseldir;
    if ( newfltr )	filter = selfltr;

    uiFileDialog dlg( this, forread, fname, filter );

    if ( selmodset )
	dlg.setMode( selmode );

    if ( !dlg.go() )
	return;

    BufferString oldfname( fname );
    BufferString newfname;
    if ( selmode == uiFileDialog::ExistingFiles )
    {
	BufferStringSet filenames;
	dlg.getFileNames( filenames );
	uiFileDialog::list2String( filenames, newfname );
	setFileName( newfname );
	deepErase( filenames );
    }
    else
    {
	newfname = dlg.fileName();
	setFileName( newfname );
    }

    if ( newfname != oldfname )
	valuechanged.trigger( *this );

    if ( examinebut && browser )
	browser->setFileName( fileName() );
}


const char* uiFileInput::fileName()
{
    fname = text();
    if ( !File_isAbsPath(fname) && fname != "" && defseldir != "" )
	fname = File_getFullPath( defseldir, fname );
    return fname;
}


void uiFileInput::getFileNames( BufferStringSet& list ) const
{
    BufferString string = text();
    uiFileDialog::string2List( string, list );
}


void uiFileInput::examineFile( CallBacker* )
{
    const char* fnm = fileName();
    if ( browser )
	browser->setFileName( fnm );
    else
	browser = new uiFileBrowser( this, fnm );

    browser->show();
}
