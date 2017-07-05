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
    , checkable_(false)
{
}


uiFileSel::Setup::Setup( uiFileDialog::Type t, const char* filenm )
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
    , checkable_(false)
{
}


uiFileSel::uiFileSel( uiParent* p, const uiString& txt, const Setup& setup )
    : uiGroup(p,"File input")
    , forread_(setup.forread_)
    , filter_(setup.filter_)
    , defseldir_(setup.defseldir_)
    , displaylocalpath_(setup.displaylocalpath_)
    , filedlgtype_(setup.filedlgtype_)
    , addallexts_(setup.allowallextensions_)
    , examstyle_(setup.examstyle_)
    , exameditable_(setup.exameditable_)
    , confirmoverwrite_(setup.confirmoverwrite_)
    , objtype_(setup.objtype_)
    , newSelection(this)
    , acceptReq(this)
    , checked(this)
{
    init( setup, txt );
}


uiFileSel::uiFileSel( uiParent* p, const uiString& txt, const char* fnm )
    : uiGroup(p,"File input")
    , forread_(true)
    , filter_("")
    , filedlgtype_(uiFileDialog::Gen)
    , addallexts_(true)
    , confirmoverwrite_(true)
    , defseldir_(GetDataDir())
    , displaylocalpath_(false)
    , newSelection(this)
    , acceptReq(this)
    , checked(this)
{
    Setup setup( fnm );
    setup.withexamine( true );
    init( setup, txt );
}


void uiFileSel::init( const Setup& setup, const uiString& lbltxt )
{
    lbl_ = 0; examinebut_ = 0; checkbox_ = 0;
    defaultext_ = "dat";
    selmode_ = uiFileDialog::AnyFile;
    selmodset_ = false;

    if ( setup.checkable_ )
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
	if ( (forread_ && !fsa.readingSupported())
	  || (!forread_ && !fsa.writingSupported()) )
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

    fnmfld_ = new uiLineEdit( this, FileNameInpSpec(setup.fnm), "File Name" );
    setFileName( setup.fnm );
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

    if ( setup.withexamine_ )
    {
	examinebut_ = uiButton::getStd( this,
			exameditable_ ? OD::Edit : OD::Examine,
			mCB(this,uiFileSel,examineFileCB), false );
	examinebut_->setText(
		exameditable_ ? uiStrings::sEdit() : uiStrings::sExamine() );
	examinebut_->attach( rightOf, selbut_ );
    }

    if ( setup.directories_ )
    {
	selmodset_ = true;
	selmode_ = uiFileDialog::DirectoryOnly;
	defaultext_.setEmpty();
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

    defseldir_ = s ? s : GetDataDir();
    setFileName( fname );
}


void uiFileSel::setNoManualEdit()
{
    fnmfld_->setSensitive( false );
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


void uiFileSel::setFileName( const char* s )
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
	    {
		setText( ptr+1 );
		return;
	    }
	}
    }
}


void uiFileSel::setButtonStates()
{
    if ( examinebut_ )
	examinebut_->setSensitive( File::exists(fileName()) );

    const int protnr = protfld_->currentItem();
    selbut_->setSensitive( fsAccess(protnr).queriesSupported() );
}


void uiFileSel::setDefaultExtension( const char* ext )
{ defaultext_ = ext; }


void uiFileSel::protChgCB( CallBacker* )
{
    setButtonStates();
}


void uiFileSel::inputChgCB( CallBacker* )
{
    setButtonStates();
    newSelection.trigger();
}


void uiFileSel::fnmEnteredCB( CallBacker* )
{
    if ( forread_ || defaultext_.isEmpty() || inDirectorySelectMode() )
	return;

    File::Path fp( fileName() );
    const FixedString ext = fp.extension();
    if ( !ext.isEmpty() )
	return;

    fp.setExtension( defaultext_ );
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
	fname = defseldir_;
    BufferString oldfltr = selfltr_;

    const bool usegendlg = selmode_ == uiFileDialog::Directory
			|| selmode_ == uiFileDialog::DirectoryOnly
			|| filedlgtype_ == uiFileDialog::Gen;
    uiString seltyp( usegendlg && selmodset_
	    && (selmode_ == uiFileDialog::Directory
	     || selmode_ == uiFileDialog::DirectoryOnly)
	    ? uiStrings::sDirectory().toLower() : uiStrings::sFile().toLower());
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

    if ( newfname != oldfname || (!forread_ && oldfltr != selfltr_) )
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
    if ( !fp.isAbsolute() && !defseldir_.isEmpty() )
    {
	fp.insert( defseldir_ );
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


uiFileDialog::Mode uiFileSel::selectMode() const
{
    return selmodset_ ? selmode_
		      : (forread_ ? uiFileDialog::ExistingFile
				  : uiFileDialog::AnyFile);
}


void uiFileSel::setSelectMode( uiFileDialog::Mode m )
{
    selmodset_ = true;
    selmode_ = m;
}


bool uiFileSel::inDirectorySelectMode() const
{
    return selmode_ == uiFileDialog::Directory ||
	   selmode_ == uiFileDialog::DirectoryOnly;
}


void uiFileSel::examineFileCB( CallBacker* )
{
    if ( examinecb_.willCall() )
	examinecb_.doCall( this );
    else
    {
	File::ViewPars vp( examstyle_ );
	vp.editable_ = exameditable_;
	if ( !File::launchViewer(fileName(),vp) )
	    uiMSG().error( tr("Cannot launch file browser") );
    }
}
