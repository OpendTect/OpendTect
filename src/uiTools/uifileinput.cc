/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uifileinput.h"

#include "filepath.h"
#include "oddirs.h"
#include "perthreadrepos.h"

#include "uibutton.h"
#include "uifiledlg.h"
#include "uigeninput.h"
#include "uimsg.h"
#include "uiselsimple.h"
#include "uistrings.h"

#include <string.h>


// uiFileInput::Setup

uiFileInput::Setup::Setup( const char* filenm )
    : fnm(filenm)
    , forread_(true)
    , withexamine_(false)
    , examstyle_(File::ViewStyle::Text)
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
    , examstyle_(t==uiFileDialog::Img ? File::ViewStyle::Bin
				      : File::ViewStyle::Text)
    , exameditable_(false)
    , directories_(false)
    , allowallextensions_(true)
    , confirmoverwrite_(true)
    , filedlgtype_(t)
    , defseldir_(GetDataDir())
    , displaylocalpath_(false)
{
}


uiFileInput::Setup::~Setup()
{}


// uiFileInput

uiFileInput::uiFileInput( uiParent* p, const uiString& txt, const Setup& setup )
    : uiGenInput( p, txt, FileNameInpSpec(setup.fnm) )
    , forread_(setup.forread_)
    , filter_(setup.filter_)
    , defseldir_(setup.defseldir_)
    , displaylocalpath_(setup.displaylocalpath_)
    , addallexts_(setup.allowallextensions_)
    , examstyle_(setup.examstyle_)
    , exameditable_(setup.exameditable_)
    , confirmoverwrite_(setup.confirmoverwrite_)
    , defaultext_("dat")
    , objtype_(setup.objtype_)
    , filedlgtype_(setup.filedlgtype_)
{
    setStretch( 2, 0 );
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
	selmode_ = uiFileDialog::DirectoryOnly;
	defaultext_.setEmpty();
    }

    mAttachCB( valueChanging, uiFileInput::inputChg );
    mAttachCB( postFinalize(), uiFileInput::isFinalized );
    mAttachCB( valueChanged, uiFileInput::fnmEntered );
    mAttachCB( checked, uiFileInput::checkCB );
}


uiFileInput::uiFileInput( uiParent* p, const uiString& txt, const char* fnm )
    : uiGenInput( p, txt, FileNameInpSpec(fnm) )
    , forread_(true)
    , filter_("")
    , defseldir_(GetDataDir())
    , displaylocalpath_(false)
    , addallexts_(true)
    , confirmoverwrite_(true)
    , defaultext_("dat")
    , filedlgtype_(uiFileDialog::Gen)
{
    setStretch( 2, 0 );
    setFileName( fnm );
    setWithSelect( true );
    mAttachCB( valueChanged, uiFileInput::fnmEntered );
    mAttachCB( checked, uiFileInput::checkCB );
}


uiFileInput::~uiFileInput()
{
    detachAllNotifiers();
}


void uiFileInput::isFinalized( CallBacker* )
{
    if ( !examinebut_ )
	return;

    examinebut_->attach( rightOf, selbut_ );
    inputChg( nullptr );
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
{
    defaultext_ = ext;
}


void uiFileInput::inputChg( CallBacker* )
{
    bool enable = false;
    BufferStringSet fnms;
    getFileNames( fnms );
    for ( int idx=0; idx<fnms.size(); idx++ )
    {
	enable = File::exists( fnms.get(idx) );
	if ( enable )
	   break;
    }

    enableExamine( enable );
}


void uiFileInput::fnmEntered( CallBacker* )
{
    const bool isdir = selmode_==uiFileDialog::Directory ||
		       selmode_==uiFileDialog::DirectoryOnly;
    if ( isdir || forread_ || defaultext_.isEmpty() )
	return;

    FilePath fp( fileName() );
    const StringView ext = fp.extension();
    if ( !ext.isEmpty() )
	return;

    fp.setExtension( defaultext_ );
    setFileName( fp.fullPath() );
}


void uiFileInput::checkCB( CallBacker* )
{
    const bool ischecked = isChecked();
    if ( examinebut_ )
	examinebut_->setSensitive( ischecked );
}


BufferString getExtFromFilter( const char* fltr )
{
    BufferString filter( fltr );
    char* extptr = filter.find( "*." );
    if ( !extptr )
	return "";

    extptr+=2;
    if ( !extptr || extptr[0]=='*' )
	return "";

    const int len = strlen( extptr );
    for ( int idx=0; idx<len; idx++ )
    {
	if ( extptr[idx]==')' || extptr[idx]==' ' )
	{
	    extptr[idx] = '\0';
	    return extptr;
	}
    }

    return "";
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
    uiString seltyp( usegendlg && selmodset_
	    && (selmode_ == uiFileDialog::Directory
	     || selmode_ == uiFileDialog::DirectoryOnly)
	    ? uiStrings::sFolder().toLower() : uiStrings::sFile().toLower());
    uiString capt( forread_ ? tr("Choose input %1 %2")
			    : tr("Specify output %1 %2" ) );
    capt.arg( objtype_ );
    capt.arg( seltyp );

    PtrMan<uiFileDialog> dlg = usegendlg
	? new uiFileDialog( this, forread_, fname, filter_, capt )
	: new uiFileDialog( this, filedlgtype_, fname, filter_, capt );

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
	filenames.setEmpty();
    }
    else
    {
	newfname = dlg->fileName();
	const bool isdir = selmode_==uiFileDialog::Directory ||
			   selmode_==uiFileDialog::DirectoryOnly;
	if ( !forread_ && !defaultext_.isEmpty() && !isdir )
	{
	    FilePath fp( newfname );
	    const StringView ext = fp.extension();
	    if ( ext.isEmpty() )
	    {
		BufferString selext = getExtFromFilter( selfltr_ );
		fp.setExtension( selext.isEmpty() ? defaultext_ : selext );
	    }

	    newfname = fp.fullPath();
	}

	setFileName( newfname );
    }

    if ( newfname != oldfname || ( !forread_ && oldfltr != selfltr_ ) )
	valueChanged.trigger( *this );
}


void uiFileInput::ensureAbsolutePath( BufferString& fname ) const
{
    FilePath fp( fname );
    if ( !fp.isAbsolute() && !defseldir_.isEmpty() )
    {
	fp.insert( defseldir_ );
	fname = fp.fullPath(); //fname is cleaned here.
    }
    else
	fname = FilePath::mkCleanPath( fname, FilePath::Local );
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

    ensureAbsolutePath( fname );
    return fname;
}


const char* uiFileInput::pathOnly() const
{
    mDeclStaticString( path );
    const FilePath fnmfp( fileName() );
    path = fnmfp.pathOnly();
    return path.buf();
}


const char* uiFileInput::baseName() const
{
    mDeclStaticString( base );
    const FilePath fnmfp( fileName() );
    base = fnmfp.baseName();
    return base.buf();
}


void uiFileInput::getFileNames( BufferStringSet& list ) const
{
    const BufferString allfnms = text();
    if ( allfnms.isEmpty() )
	return;

    uiFileDialog::string2List( allfnms, list );
    for ( int idx=0; idx<list.size(); idx++ )
	ensureAbsolutePath( list.get(idx) );
}


void uiFileInput::examineFile( CallBacker* )
{
    if ( excb_.willCall() )
    {
	excb_.doCall( this );
	return;
    }

    BufferStringSet fnms;
    getFileNames( fnms );
    int selidx = 0;
    if ( fnms.size() > 1 )
    {
	uiSelectFromList::Setup listsetup( tr("Select file to examine"), fnms );
	uiSelectFromList listdlg( this, listsetup );
	if ( !listdlg.go() )
	    return;

	selidx = listdlg.selection();
	if ( selidx < 0 )
	    return;
    }

    File::ViewPars vp( examstyle_ );
    vp.editable_ = exameditable_;
    if ( !File::launchViewer(fnms.get(selidx),vp) )
	uiMSG().error( tr("Cannot launch file browser") );
}


// uiASCIIFileInput
static uiString getLabel( bool forread )
{
    return forread ? uiStrings::sInputASCIIFile()
		   : uiStrings::sOutputASCIIFile();
}


static uiFileInput::Setup getSetup( bool forread )
{
    uiFileInput::Setup fsu;
    fsu.forread(forread)
	.defseldir( forread ? GetImportFromDir() : GetExportToDir() );
    if ( forread )
	fsu.withexamine(true);
    else
	fsu.filter(File::asciiFilesFilter());

    return fsu;
}


uiASCIIFileInput::uiASCIIFileInput( uiParent* p, bool forread )
    : uiFileInput(p,getLabel(forread),getSetup(forread))
{
    setDefaultExtension( "txt" );
    mAttachCB( valueChanged, uiASCIIFileInput::fileSelCB );
}


uiASCIIFileInput::uiASCIIFileInput( uiParent* p, const uiString& lbl,
				    bool forread )
    : uiFileInput(p,lbl,getSetup(forread))
{
    setDefaultExtension( "txt" );
    mAttachCB( valueChanged, uiASCIIFileInput::fileSelCB );
}


uiASCIIFileInput::~uiASCIIFileInput()
{
    detachAllNotifiers();
}


void uiASCIIFileInput::fileSelCB( CallBacker* )
{
    const BufferString fnm = fileName();
    if ( fnm.isEmpty() )
	return;

    if ( forread_ )
    {
	if ( !File::exists(fnm) )
	    return;

	SetImportFromDir( pathOnly() );
    }
    else
	SetExportToDir( pathOnly() );
}
