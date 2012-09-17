/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Lammertink
 Date:          08/08/2000
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uifileinput.cc,v 1.59 2012/07/04 10:37:42 cvsranojay Exp $";

#include "uifileinput.h"
#include "uifiledlg.h"
#include "uilabel.h"
#include "uibutton.h"
#include "uilineedit.h"
#include "uigeninput.h"
#include "filepath.h"
#include "oddirs.h"
#include "strmprov.h"


uiFileInput::Setup::Setup( const char* filenm )
    : fnm(filenm)
    , forread_(true)
    , withexamine_(false)
    , examstyle_(View)
    , directories_(false)
    , allowallextensions_(true)
    , confirmoverwrite_(true)
    , filedlgtype_(uiFileDialog::Gen)
    , defseldir_(GetDataDir())
    , displaylocalpath_(false)
{
}


uiFileInput::Setup::Setup( uiFileDialog::Type t, const char* filenm )
    : fnm(filenm)
    , forread_(true)
    , withexamine_(t==uiFileDialog::Txt)
    , examstyle_(View)
    , confirmoverwrite_(true)
    , filedlgtype_(t)
    , directories_(false)
    , allowallextensions_(true)
    , defseldir_(GetDataDir())
    , displaylocalpath_(false)
{
}


uiFileInput::uiFileInput( uiParent* p, const char* txt, const Setup& setup )
    : uiGenInput( p, txt, FileNameInpSpec(setup.fnm) )
    , forread_(setup.forread_)
    , filter_(setup.filter_)
    , defseldir_(setup.defseldir_)
    , displaylocalpath_(setup.displaylocalpath_)
    , selmodset_(false)
    , selmode_(uiFileDialog::AnyFile)
    , filedlgtype_(setup.filedlgtype_)
    , examinebut_(0)
    , addallexts_(setup.allowallextensions_)
    , examstyle_(setup.examstyle_)
    , confirmoverwrite_(setup.confirmoverwrite_)
{
    setFileName( setup.fnm );
    setWithSelect( true );
    if ( setup.withexamine_ )
    {
	examinebut_ = new uiPushButton( this,
			    (examstyle_==Setup::Edit ? "&Edit" : "&Examine"),
			    mCB(this,uiFileInput,examineFile), false );

	examinebut_->setName(
	    BufferString(examstyle_==Setup::Edit ? "Edit" : "Examine ",txt) );
    }
    if ( setup.directories_ )
    {
	selmodset_ = true;
	selmode_ = uiFileDialog::DirectoryOnly;
    }

    postFinalise().notify( mCB(this,uiFileInput,isFinalised) );
}


uiFileInput::uiFileInput( uiParent* p, const char* txt, const char* fnm )
    : uiGenInput( p, txt, FileNameInpSpec(fnm) )
    , forread_(true)
    , filter_("")
    , filedlgtype_(uiFileDialog::Gen)
    , selmodset_(false)
    , selmode_(uiFileDialog::AnyFile)
    , examinebut_(0)
    , addallexts_(true)
    , confirmoverwrite_(true)
    , defseldir_(GetDataDir())
    , displaylocalpath_(false)
{
    setFileName( fnm );
    setWithSelect( true );
}


uiFileInput::~uiFileInput()
{
}


void uiFileInput::isFinalised( CallBacker* )
{
    if ( examinebut_ )
	examinebut_->attach( rightOf, selbut );
}


void uiFileInput::setDefaultSelectionDir( const char* s )
{
    BufferString fname = text();
    if ( !fname.isEmpty() )
	fname = fileName();

    defseldir_ = s ? s : GetDataDir();
    setFileName( fname );
}


void uiFileInput::setFileName( const char* s )
{
    setText( s );

    if ( displaylocalpath_ )
    {
	BufferString fname = fileName();
	BufferString seldir = FilePath(defseldir_).fullPath();
	if ( matchString(seldir,fname) )
	{
	    const char* ptr = fname.buf() + seldir.size();
	    if ( *ptr=='\\' || *ptr=='/' )
		setText( ptr+1 );
	}
    }
}


void uiFileInput::enableExamine( bool yn )
{
    if ( examinebut_ )
	examinebut_->setSensitive( yn );
}


void uiFileInput::doSelect( CallBacker* )
{
    BufferString fname = fileName();
    if ( fname.isEmpty() )
	fname = defseldir_;
    BufferString oldfltr = selfltr_;

    const bool usegendlg = selmode_ == uiFileDialog::Directory
			|| selmode_ == uiFileDialog::DirectoryOnly
			|| filedlgtype_ == uiFileDialog::Gen;
    PtrMan<uiFileDialog> dlg = usegendlg
	? new uiFileDialog( this, forread_, fname, filter_ )
	: new uiFileDialog( this, filedlgtype_, fname );

    dlg->setSelectedFilter( selfltr_ );
    if ( usegendlg )
    {
	dlg->setAllowAllExts( addallexts_ );
	if ( selmodset_ )
	    dlg->setMode( selmode_ );
    }
    dlg->setConfirmOverwrite( confirmoverwrite_ );


    if ( !dlg->go() )
	return;

    selfltr_ = dlg->selectedFilter();
    BufferString oldfname( fname );
    BufferString newfname;
    if ( selmode_ == uiFileDialog::ExistingFiles )
    {
	BufferStringSet filenames;
	dlg->getFileNames( filenames );
	uiFileDialog::list2String( filenames, newfname );
	setFileName( newfname );
	deepErase( filenames );
    }
    else
    {
	newfname = dlg->fileName();
	setFileName( newfname );
    }

    if ( newfname != oldfname || ( !forread_ && oldfltr != selfltr_ ) )
	valuechanged.trigger( *this );
}


const char* uiFileInput::fileName() const
{
    static BufferString fname;
    fname = text();
    if ( fname.isEmpty() )
	return fname;
#ifdef __win__ 
    if ( fname.size() == 2 )
	fname += "\\";
#endif

    FilePath fp( fname );
    if ( !fp.isAbsolute() && !defseldir_.isEmpty() )
    {
	fp.insert( defseldir_ );
	fname = fp.fullPath(); //fname is cleaned here.
    }
    else
	fname = FilePath::mkCleanPath( fname, FilePath::Local );

    return fname;
}


void uiFileInput::getFileNames( BufferStringSet& list ) const
{
    BufferString string = text();
    uiFileDialog::string2List( string, list );
}


void uiFileInput::examineFile( CallBacker* )
{
    if ( excb_.willCall() )
	excb_.doCall( this );
    else
    {
	BufferString cmd( "od_FileBrowser" );
	if ( examstyle_ == Setup::Table )
	    cmd += " --table --maxlines 250";
	else
	    cmd += " --maxlines 5000";

	if ( examstyle_ == Setup::Log )
	    cmd += " --log";
	if ( examstyle_ == Setup::Edit )
	    cmd += " --edit";

	ExecuteScriptCommand( cmd, fileName() );
    }
}
