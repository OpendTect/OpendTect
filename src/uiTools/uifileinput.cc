/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Lammertink
 Date:          08/08/2000
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uifileinput.cc,v 1.48 2009-08-28 13:15:24 cvsbert Exp $";

#include "uifileinput.h"
#include "uifiledlg.h"
#include "uilabel.h"
#include "uibutton.h"
#include "uilineedit.h"
#include "uigeninput.h"
#include "filepath.h"
#include "oddirs.h"
#include "strmprov.h"


uiFileInput::uiFileInput( uiParent* p, const char* txt, const Setup& setup )
    : uiGenInput( p, txt, FileNameInpSpec(setup.fnm) )
    , forread_(setup.forread_)
    , filter_(setup.filter_)
    , defseldir_(setup.defseldir_)
    , selmodset_(false)
    , selmode_(uiFileDialog::AnyFile)
    , filedlgtype_(setup.filedlgtype_)
    , examinebut_(0)
    , addallexts_(setup.allowallextensions_)
    , tablevw_(setup.examinetablestyle_)
    , confirmoverwrite_(setup.confirmoverwrite_)
{
    setWithSelect( true );
    if ( setup.withexamine_ )
    {
	examinebut_ = new uiPushButton( this, "&Examine", 
					mCB(this,uiFileInput,examineFile),
					false );
	examinebut_->setName( BufferString("Examine ",txt) );
    }
    if ( setup.directories_ )
    {
	selmodset_ = true;
	selmode_ = uiFileDialog::DirectoryOnly;
    }

    mainObject()->finaliseDone.notify( mCB(this,uiFileInput,isFinalised) );
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
{
    setWithSelect( true );
}


uiFileInput::~uiFileInput()
{
}


void uiFileInput::isFinalised( CallBacker* )
{
    if ( examinebut_ )
	examinebut_->attach( rightOf, selbut );
    if ( selbut )
    {
	UserInputObj* uiobj = element(0);
	mDynamicCastGet(uiLineEdit*,le,uiobj)
	if ( le ) le->returnPressed.notify( mCB(this,uiFileInput,doSelect) );
    }
}


void uiFileInput::setFileName( const char* s )
{
    setText( s );
}


void uiFileInput::enableExamine( bool yn )
{
    if ( examinebut_ ) examinebut_->setSensitive( yn );
}


void uiFileInput::doSelect( CallBacker* )
{
    BufferString fname = text();
    BufferString oldfltr = selfltr_;
    if ( fname.isEmpty() )	fname = defseldir_;

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
    FilePath fp( fname );
    if ( !fp.isAbsolute() && !fname.isEmpty() && !defseldir_.isEmpty() )
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
	BufferString cmd( "FileBrowser" );
	if ( tablevw_ )
	    cmd += " --table --maxlines 250";
	else
	    cmd += " --maxlines 5000";
	ExecuteScriptCommand( cmd, fileName() );
    }
}
