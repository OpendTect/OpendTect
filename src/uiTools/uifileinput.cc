/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Lammertink
 Date:          08/08/2000
 RCS:           $Id: uifileinput.cc,v 1.27 2005-01-25 13:31:29 nanne Exp $
________________________________________________________________________

-*/

#include "uifileinput.h"
#include "uifiledlg.h"
#include "uilabel.h"
#include "uibutton.h"
#include "uigeninput.h"
#include "filepath.h"
#include "strmprov.h"


uiFileInput::uiFileInput( uiParent* p, const char* txt, const Setup& setup )
    : uiGenInput( p, txt, FileNameInpSpec(setup.fnm) )
    , forread(setup.forread_)
    , fname(setup.fnm)
    , filter(setup.filter_)
    , newfltr(false)
    , selmodset(false)
    , selmode(uiFileDialog::AnyFile)
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
    , examinebut(0)
{
    setWithSelect( true );
}


uiFileInput::~uiFileInput()
{
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


void uiFileInput::enableExamine( bool yn )
{
    if ( examinebut ) examinebut->setSensitive( yn );
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

    selfltr = dlg.selectedFilter();
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
}


const char* uiFileInput::fileName()
{
    fname = text();
    FilePath fp( fname );
    if ( !fp.isAbsolute() && fname != "" && defseldir != "" )
    {
	fp.insert( defseldir );
	fname = fp.fullPath();
    }
    return fname;
}


void uiFileInput::getFileNames( BufferStringSet& list ) const
{
    BufferString string = text();
    uiFileDialog::string2List( string, list );
}


void uiFileInput::examineFile( CallBacker* )
{
    static BufferString fnm_;
    fnm_ = fileName();

    replaceCharacter( fnm_.buf(), ' ', (char)128 );

    BufferString cmd( "@" );
    cmd += mGetExecScript();

    cmd += " FileBrowser \'";
    cmd += fnm_; cmd += "\' ";

    StreamProvider strmprov( cmd );
    if ( !strmprov.executeCommand(false) )
    {
        BufferString s( "Failed to submit command '" );
        s += strmprov.command(); s += "'";
        ErrMsg( s );
    }
}
