/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Lammertink
 Date:          08/08/2000
________________________________________________________________________

-*/

#include "uifilesel.h"
#include "uifiledlg.h"
#include "uilabel.h"
#include "uilineedit.h"
#include "uibutton.h"
#include "uicombobox.h"
#include "uimsg.h"
#include "filepath.h"
#include "filesystemaccess.h"
#include "perthreadrepos.h"
#include "oddirs.h"
#include "uistrings.h"


uiFileSel::Setup::Setup( const char* filenm )
    : filename_(filenm)
    , forread_(true)
    , withexamine_(false)
    , examstyle_(File::Text)
    , exameditable_(false)
    , directories_(false)
    , allowallextensions_(true)
    , confirmoverwrite_(true)
    , contenttype_(OD::GeneralContent)
    , defseldir_(GetDataDir())
    , displaylocalpath_(false)
    , checkable_(false)
{
}


uiFileSel::Setup::Setup( OD::FileContentType t, const char* filenm )
    : filename_(filenm)
    , forread_(true)
    , withexamine_(t==OD::TextContent)
    , examstyle_(t==OD::ImageContent?File::Bin:File::Text)
    , exameditable_(false)
    , directories_(false)
    , allowallextensions_(true)
    , confirmoverwrite_(true)
    , contenttype_(t)
    , defseldir_(GetDataDir())
    , displaylocalpath_(false)
    , checkable_(false)
{
}


uiFileSel::uiFileSel( uiParent* p, const uiString& txt, const Setup& setup )
    : uiGroup(p,"File input")
    , setup_(setup)
    , newSelection(this)
    , acceptReq(this)
    , checked(this)
{
    init( txt );
}


uiFileSel::uiFileSel( uiParent* p, const uiString& txt, const char* fnm )
    : uiGroup(p,"File input")
    , setup_(fnm)
    , newSelection(this)
    , acceptReq(this)
    , checked(this)
{
    setup_.forread( true ).defseldir( GetDataDir() ).withexamine( true );
    init( txt );
}


void uiFileSel::init( const uiString& lbltxt )
{
    lbl_ = 0; examinebut_ = 0; checkbox_ = 0;
    selmode_ = OD::SelectAnyFile;
    selmodset_ = false;

    if ( setup_.checkable_ )
    {
	checkbox_ = new uiCheckBox( this, uiString::emptyString() );
	checkbox_->setChecked( true );
	checkbox_->activated.notify( mCB(this,uiFileSel,checkCB) );
    }

    const Factory<File::SystemAccess>& fact = File::SystemAccess::factory();
    uiStringSet protnms( fact.getUserNames() );
    factnms_ = fact.getNames();
    for ( int idx=0; idx<factnms_.size(); idx++ )
    {
	const File::SystemAccess& fsa = fsAccess( idx );
	if ( (setup_.forread_ && !fsa.readingSupported())
	  || (!setup_.forread_ && !fsa.writingSupported()) )
	{
	    protnms.removeSingle( idx );
	    factnms_.removeSingle( idx );
	    idx--;
	}
    }
    protfld_ = new uiComboBox( this, "Protocol" );
    protfld_->addItems( protnms );
    for ( int idx=0; idx<factnms_.size(); idx++ )
	protfld_->setIcon( idx, fsAccess(idx).iconName() );
    if ( checkbox_ )
	protfld_->attach( rightOf, checkbox_ );
    mAttachCB( protfld_->selectionChanged, uiFileSel::protChgCB );
    protfld_->setHSzPol( uiObject::Small );

    fnmfld_ = new uiLineEdit( this, FileNameInpSpec(setup_.filename_),
				    "File Name" );
    setFileName( setup_.filename_ );
    setHAlignObj( fnmfld_ );
    fnmfld_->setHSzPol( uiObject::WideVar );

    if ( lbltxt.isEmpty() )
	protfld_->attach( leftOf, fnmfld_ );
    else
    {
	lbl_ = new uiLabel( this, lbltxt, fnmfld_ );
	protfld_->attach( leftOf, lbl_ );
    }

    selbut_ = uiButton::getStd( this, OD::Select,
				mCB(this,uiFileSel,doSelCB), false );
    selbut_->attach( rightOf, fnmfld_ );

    if ( setup_.withexamine_ )
    {
	examinebut_ = uiButton::getStd( this,
			setup_.exameditable_ ? OD::Edit : OD::Examine,
			mCB(this,uiFileSel,examineFileCB), false );
	examinebut_->setText( setup_.exameditable_
			    ? uiStrings::sEdit() : uiStrings::sExamine() );
	examinebut_->attach( rightOf, selbut_ );
    }

    if ( setup_.directories_ )
    {
	selmodset_ = true;
	selmode_ = OD::SelectDirectory;
    }

    fnmfld_->editingFinished.notify( mCB(this,uiFileSel,inputChgCB) );
    fnmfld_->returnPressed.notify( mCB(this,uiFileSel,fnmEnteredCB) );
    postFinalise().notify( mCB(this,uiFileSel,atFinaliseCB) );
}


uiFileSel::~uiFileSel()
{
}


void uiFileSel::atFinaliseCB( CallBacker* )
{
    setButtonStates();
    checkCB( 0 );
}


const File::SystemAccess& uiFileSel::fsAccess( int protnr ) const
{
    if ( !factnms_.validIdx(protnr) )
	{ pErrMsg("Huh"); protnr = 0; }
    return File::SystemAccess::getByProtocol( factnms_.get(protnr) );
}


void uiFileSel::setDefaultSelectionDir( const char* s )
{
    BufferString fname = text();
    if ( !fname.isEmpty() )
	fname = fileName();

    setup_.defseldir_ = s ? s : GetDataDir();
    setFileName( fname );
}


BufferString uiFileSel::selectedExtension() const
{
    const BufferString fnm( fileName() );
    return BufferString( File::Path(fnm).extension() );
}


BufferString uiFileSel::selectedProtocol() const
{
    const BufferString fnm( fileName() );
    return File::SystemAccess::getProtocol( fnm );
}


uiString uiFileSel::labelText() const
{
    return lbl_ ? lbl_->text() : uiString::emptyString();
}


void uiFileSel::setLabelText( const uiString& txt )
{
    if ( lbl_ )
	lbl_->setText( txt );
}


void uiFileSel::setText( const char* s )
{
    if ( FixedString(s) == fnmfld_->text() )
	return;

    fnmfld_->setText( s );
    fnmfld_->end();
    inputChgCB( 0 );
}


const char* uiFileSel::text() const
{
    return fnmfld_->text();
}


void uiFileSel::setFileName( const char* fnm )
{
    setText( fnm );

    const BufferString filename = fileName();
    if ( setup_.displaylocalpath_ )
    {
	BufferString seldir = File::Path(setup_.defseldir_).fullPath();
	if ( filename.startsWith(seldir) )
	{
	    const char* ptr = filename.str() + seldir.size();
	    if ( *ptr=='\\' || *ptr=='/' )
		setText( ptr+1 );
	}
    }

    const BufferString prot = File::SystemAccess::getProtocol( filename );
    protfld_->setText( prot );
}


void uiFileSel::setButtonStates()
{
    if ( examinebut_ )
	examinebut_->setSensitive( File::exists(fileName()) );

    const int protnr = protfld_->currentItem();
    selbut_->setSensitive( fsAccess(protnr).queriesSupported() );
}


void uiFileSel::protChgCB( CallBacker* )
{
    BufferString fnm( fileName() );
    const BufferString prot( File::SystemAccess::getProtocol(fnm) );
    const BufferString newprot = factnms_.get( protfld_->currentItem() );
    if ( prot != newprot )
    {
	const BufferString filenmonly = File::Path(fnm).fileName();
	fnm = File::SystemAccess::withProtocol( filenmonly, newprot );
	setFileName( fnm );
    }
}


void uiFileSel::inputChgCB( CallBacker* )
{
    setButtonStates();
    newSelection.trigger();
}


void uiFileSel::fnmEnteredCB( CallBacker* )
{
    if ( setup_.forread_ || setup_.defaultext_.isEmpty()
	|| inDirectorySelectMode() )
	return;

    File::Path fp( fileName() );
    const FixedString ext = fp.extension();
    if ( !ext.isEmpty() )
	return;

    fp.setExtension( setup_.defaultext_ );
    setFileName( fp.fullPath() );

    acceptReq.trigger();
}


void uiFileSel::setSensitive( bool yn )
{
    setChildrenSensitive( yn );
    if ( yn )
	checkCB( 0 );
}


void uiFileSel::setChecked( bool yn )
{
    if ( checkbox_ )
    {
	checkbox_->setChecked( yn );
	checkCB(0);
    }
}


bool uiFileSel::isChecked() const
{
    return !checkbox_ || checkbox_->isChecked();
}


void uiFileSel::checkCB( CallBacker* )
{
    if ( !checkbox_ )
	return;
    const bool ischecked = checkbox_->isChecked();
    protfld_->setSensitive( ischecked );
    fnmfld_->setSensitive( ischecked );
    selbut_->setSensitive( ischecked );
    if ( examinebut_ )
	examinebut_->setSensitive( ischecked );
}


void uiFileSel::doSelCB( CallBacker* )
{
    BufferString fname = fileName();
    if ( fname.isEmpty() )
	fname = setup_.defseldir_;

    const bool usegendlg = isDirectory(selmode_)
			|| setup_.contenttype_ == OD::GeneralContent;
    uiString seltyp( usegendlg && selmodset_ && isDirectory(selmode_)
	    ? uiStrings::sDirectory().toLower() : uiStrings::sFile().toLower());
    uiString capt( setup_.forread_ ? tr("Choose input %1 %2")
			    : tr("Specify output %1 %2" ) );
    capt.arg( objtype_ );
    capt.arg( seltyp );

    const BufferString filter = setup_.formats_.getFileFilters();
    PtrMan<uiFileDialog> dlg = usegendlg
	? new uiFileDialog( this, setup_.forread_, fname, filter, capt )
	: new uiFileDialog( this, setup_.contenttype_, fname, filter, capt );

    if ( !setup_.defaultext_.isEmpty() )
    {
	const int fmtidx = setup_.formats_.indexOf( setup_.defaultext_ );
	if ( fmtidx >= 0 )
	    dlg->setSelectedFilter(
		    setup_.formats_.format(fmtidx).getFileFilter() );
    }
    if ( usegendlg )
    {
	dlg->setAllowAllExts( setup_.allowallextensions_ );
	if ( selmodset_ )
	    dlg->setMode( selmode_ );
    }
    dlg->setConfirmOverwrite( setup_.confirmoverwrite_ );

    if ( !dlg->go() )
	return;

    BufferString oldfname( fname );
    BufferString newfname;
    if ( isMultiSelect(selmode_) )
    {
	BufferStringSet filenames;
	dlg->getFileNames( filenames );
	uiFileDialog::joinFileNamesIntoSingleString( filenames, newfname );
	setFileName( newfname );
    }
    else
    {
	newfname = dlg->fileName();
	if ( !setup_.forread_ && !setup_.defaultext_.isEmpty()
		&& !inDirectorySelectMode() )
	{
	    File::Path fp( newfname );
	    const FixedString ext = fp.extension();
	    if ( ext.isEmpty() )
		fp.setExtension( setup_.defaultext_ );
	    newfname = fp.fullPath();
	}

	setFileName( newfname );
    }

    if ( newfname != oldfname )
	newSelection.trigger();
}


const char* uiFileSel::fileName() const
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
    if ( !fp.isAbsolute() && !setup_.defseldir_.isEmpty() )
    {
	fp.insert( setup_.defseldir_ );
	fname = fp.fullPath(); //fname is cleaned here.
    }
    else
	fname = File::Path::mkCleanPath( fname, File::Path::Local );

    return fname;
}


void uiFileSel::getFileNames( BufferStringSet& list ) const
{
    BufferString string = text();
    uiFileDialog::separateFileNamesFromSingleString( string, list );
}


OD::FileSelectionMode uiFileSel::selectMode() const
{
    return selmodset_ ? selmode_
		      : (setup_.forread_ ? OD::SelectExistingFile
					 : OD::SelectAnyFile);
}


void uiFileSel::setSelectMode( OD::FileSelectionMode mode )
{
    selmodset_ = true;
    selmode_ = mode;
}


bool uiFileSel::inDirectorySelectMode() const
{
    return isDirectory( selmode_ );
}


void uiFileSel::examineFileCB( CallBacker* )
{
    if ( examinecb_.willCall() )
	examinecb_.doCall( this );
    else
    {
	File::ViewPars vp( setup_.examstyle_ );
	vp.editable_ = setup_.exameditable_;
	if ( !File::launchViewer(fileName(),vp) )
	    uiMSG().error( tr("Cannot launch file browser") );
    }
}
