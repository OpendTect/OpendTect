/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Lammertink
 Date:          08/08/2000
________________________________________________________________________

-*/


#include "uifileinput.h"
#include "uifiledlg.h"
#include "uilabel.h"
#include "uibutton.h"
#include "uilineedit.h"
#include "uigeninput.h"
#include "uimsg.h"
#include "file.h"
#include "filepath.h"
#include "oddirs.h"
#include "oscommand.h"
#include "staticstring.h"
#include "uistrings.h"


uiFileInput::Setup::Setup( const char* filenm )
    : fnm(filenm)
    , forread_(true)
    , withexamine_(false)
    , examstyle_(File::Text)
    , exameditable_(false)
    , directories_(false)
    , allowallextensions_(true)
    , confirmoverwrite_(true)
    , filedlgtype_(OD::GeneralContent)
    , defseldir_(GetDataDir())
    , displaylocalpath_(false)
{
}


uiFileInput::Setup::Setup( OD::FileContentType t, const char* filenm )
    : fnm(filenm)
    , forread_(true)
    , withexamine_(t==OD::TextContent)
    , examstyle_(t==OD::ImageContent?File::Bin:File::Text)
    , exameditable_(false)
    , directories_(false)
    , allowallextensions_(true)
    , confirmoverwrite_(true)
    , filedlgtype_(t)
    , defseldir_(GetDataDir())
    , displaylocalpath_(false)
{
}


mStartAllowDeprecatedSection

uiFileInput::uiFileInput( uiParent* p, const uiString& txt, const Setup& setup )
    : uiGenInput( p, txt, FileNameInpSpec(setup.fnm) )
    , forread_(setup.forread_)
    , filter_(setup.filter_)
    , defseldir_(setup.defseldir_)
    , displaylocalpath_(setup.displaylocalpath_)
    , selmodset_(false)
    , selmode_(setup.forread_? OD::SelectFileForRead : OD::SelectFileForWrite)
    , filedlgtype_(setup.filedlgtype_)
    , examinebut_(0)
    , addallexts_(setup.allowallextensions_)
    , examstyle_(setup.examstyle_)
    , exameditable_(setup.exameditable_)
    , confirmoverwrite_(setup.confirmoverwrite_)
    , objtype_(setup.objtype_)
    , defaultext_("dat")
{
    setFileName( setup.fnm );
    setWithSelect( true );
    if ( setup.withexamine_ )
    {
	examinebut_ = uiButton::getStd( this,
			exameditable_ ? OD::Edit : OD::Examine,
			mCB(this,uiFileInput,examineFile), false );
	examinebut_->setText(
		exameditable_ ? uiStrings::sEdit() : uiStrings::sExamine() );
    }

    if ( setup.directories_ )
    {
	selmodset_ = true;
	selmode_ = OD::SelectDirectory;
	defaultext_.setEmpty();
    }

    valuechanging.notify( mCB(this,uiFileInput,inputChg) );
    postFinalise().notify( mCB(this,uiFileInput,isFinalised) );
    valuechanged.notify( mCB(this,uiFileInput,fnmEntered) );
}


uiFileInput::uiFileInput( uiParent* p, const uiString& txt, const char* fnm )
    : uiGenInput( p, txt, FileNameInpSpec(fnm) )
    , forread_(true)
    , filter_("")
    , filedlgtype_(OD::GeneralContent)
    , selmodset_(false)
    , selmode_(OD::SelectFileForRead)
    , examinebut_(0)
    , addallexts_(true)
    , confirmoverwrite_(true)
    , defseldir_(GetDataDir())
    , displaylocalpath_(false)
    , defaultext_("dat")
{
    setFileName( fnm );
    setWithSelect( true );
    valuechanged.notify( mCB(this,uiFileInput,fnmEntered) );
}

mStopAllowDeprecatedSection


uiFileInput::~uiFileInput()
{
}


void uiFileInput::isFinalised( CallBacker* )
{
    if ( examinebut_ )
    {
	examinebut_->attach( rightOf, selbut_ );
	enableExamine( File::exists(fileName()) );
    }
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
	BufferString seldir = File::Path(defseldir_).fullPath();
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


void uiFileInput::inputChg( CallBacker* )
{
    enableExamine( File::exists(fileName()) );
}


void uiFileInput::fnmEntered( CallBacker* )
{
    if ( forread_ || defaultext_.isEmpty() || inDirectorySelectMode() )
	return;

    File::Path fp( fileName() );
    const FixedString ext = fp.extension();
    if ( !ext.isEmpty() )
	return;

    fp.setExtension( defaultext_ );
    setFileName( fp.fullPath() );
}


void uiFileInput::doSelect( CallBacker* )
{
    BufferString fname = fileName();
    if ( fname.isEmpty() )
	fname = defseldir_;
    BufferString oldfltr = selfltr_;

    const bool usegendlg = selmode_ == OD::SelectDirectory
			|| filedlgtype_ == OD::GeneralContent;
    uiString seltyp( usegendlg && selmodset_
		&& selmode_ == OD::SelectDirectory
	    ? uiStrings::sDirectory().toLower() : uiStrings::sFile().toLower());
    uiString capt( forread_ ? tr("Choose input %1 %2")
			    : tr("Specify output %1 %2" ) );
    capt.arg( objtype_ );
    capt.arg( seltyp );

mStartAllowDeprecatedSection
    PtrMan<uiFileDialog> dlg = usegendlg
	? new uiFileDialog( this, forread_, fname, filter_, capt )
	: new uiFileDialog( this, filedlgtype_, fname, filter_, capt );
mStopAllowDeprecatedSection

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
    if ( !isSingle(selmode_) )
    {
	BufferStringSet filenames;
	dlg->getFileNames( filenames );
	uiFileDialog::joinFileNamesIntoSingleString( filenames, newfname );
	setFileName( newfname );
    }
    else
    {
	newfname = dlg->fileName();
	if ( !forread_ && !defaultext_.isEmpty() && !inDirectorySelectMode() )
	{
	    File::Path fp( newfname );
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

    File::Path fp( fname );
    if ( !fp.isAbsolute() && !defseldir_.isEmpty() )
    {
	fp.insert( defseldir_ );
	fname = fp.fullPath(); //fname is cleaned here.
    }
    else
	fname = File::Path::mkCleanPath( fname, File::Path::Local );

    return fname;
}


void uiFileInput::getFileNames( BufferStringSet& list ) const
{
    BufferString string = text();
    uiFileDialog::separateFileNamesFromSingleString( string, list );
}


OD::FileSelectionMode uiFileInput::selectMode() const
{
    return selmodset_ ? selmode_ : (forread_ ? OD::SelectFileForRead
					     : OD::SelectFileForWrite);
}


void uiFileInput::setSelectMode( OD::FileSelectionMode m )
{
    selmodset_ = true;
    selmode_ = m;
}


bool uiFileInput::inDirectorySelectMode() const
{
    return selmode_ == OD::SelectDirectory;
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
	    uiMSG().error( tr("Cannot launch file browser") );
    }
}
