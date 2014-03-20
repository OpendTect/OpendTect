/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Lammertink
 Date:          08/08/2000
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "uifileinput.h"
#include "uifiledlg.h"
#include "uilabel.h"
#include "uibutton.h"
#include "uilineedit.h"
#include "uigeninput.h"
#include "uimsg.h"
#include "filepath.h"
#include "oddirs.h"
#include "oscommand.h"
#include "perthreadrepos.h"


uiFileInput::Setup::Setup( const char* filenm )
    : fnm(filenm)
    , forread_(true)
    , withexamine_(false)
    , examstyle_(File::Text)
    , exameditable_(false)
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
    , examstyle_(t==uiFileDialog::Img?File::Bin:File::Text)
    , exameditable_(false)
    , directories_(false)
    , allowallextensions_(true)
    , confirmoverwrite_(true)
    , filedlgtype_(t)
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
    , exameditable_(setup.exameditable_)
    , confirmoverwrite_(setup.confirmoverwrite_)
{
    setFileName( setup.fnm );
    setWithSelect( true );
    if ( setup.withexamine_ )
    {
	const char* buttxt = exameditable_ ? "&Edit" : "&Examine";
	examinebut_ = new uiPushButton( this, buttxt,
			    mCB(this,uiFileInput,examineFile), false );
	examinebut_->setName( BufferString(buttxt+1," ",txt) );
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
	examinebut_->attach( rightOf, selbut_ );
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
	if ( fname.startsWith(seldir) )
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


void uiFileInput::setDefaultExtension( const char* ext )
{ defaultext_ = ext; }


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
	if ( !forread_ && !defaultext_.isEmpty() )
	{
	    FilePath fp( newfname );
	    const FixedString ext = fp.extension();
	    if ( ext.isEmpty() )
		fp.setExtension( defaultext_ );
	    newfname = fp.fullPath();
	}

	setFileName( newfname );
    }

    if ( newfname != oldfname || ( !forread_ && oldfltr != selfltr_ ) )
	valuechanged.trigger( *this );
}


const char* uiFileInput::fileName() const
{
    mDeclStaticString( fname );
    fname = text();
    fname.trimBlanks();
    if ( fname.isEmpty() || fname.firstChar() == '@' )
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
	File::ViewPars vp( examstyle_ );
	vp.editable_ = exameditable_;
	if ( !File::launchViewer(fileName(),vp) )
	    uiMSG().error( "Cannot launch file browser" );
    }
}
