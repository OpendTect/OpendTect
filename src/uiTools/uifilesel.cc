/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Lammertink
 Date:          08/08/2000
________________________________________________________________________

-*/

#include "uifilesel.h"
#include "uilabel.h"
#include "uilineedit.h"
#include "uibutton.h"
#include "uicombobox.h"
#include "uifileseltool.h"
#include "uimsg.h"
#include "filepath.h"
#include "fileview.h"
#include "filesystemaccess.h"
#include "staticstring.h"
#include "oddirs.h"
#include "uistrings.h"


uiFileSel::Setup::Setup( const char* filenm )
    : uiFileSelectorSetup(filenm)
    , withexamine_(false)
    , examstyle_(File::Text)
    , exameditable_(false)
    , displaylocalpath_(false)
    , checkable_(false)
    , contenttype_(OD::GeneralContent)
{
}


uiFileSel::Setup::Setup( ContentType ct, const char* filenm )
    : uiFileSelectorSetup(filenm)
    , withexamine_(ct==OD::TextContent)
    , examstyle_(ct==OD::ImageContent?File::Bin:File::Text)
    , exameditable_(false)
    , displaylocalpath_(false)
    , checkable_(false)
    , contenttype_(ct)
{
    if ( contenttype_ == OD::ImageContent )
	formats_.addFormat( File::Format::imageFiles() );
}


uiFileSel::uiFileSel( uiParent* p, const uiString& txt, const Setup& setp )
    : uiGroup(p,"File input")
    , setup_(setp)
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
    setup_.withexamine( true );
    init( txt );
}


void uiFileSel::init( const uiString& lbltxt )
{
    lbl_ = 0; examinebut_ = 0; checkbox_ = 0; protfld_ = 0;

    if ( setup_.checkable_ )
    {
	checkbox_ = new uiCheckBox( this, lbltxt );
	checkbox_->setChecked( true );
	checkbox_->activated.notify( mCB(this,uiFileSel,checkCB) );
    }

    const bool forread = isForRead( setup_.selmode_ );
    uiStringSet protnms;
    File::SystemAccess::getProtocolNames( factnms_, forread );
    const int nrfactnms =
#ifdef ENABLE_REMOTE_FILESEL_UI
	    factnms_.size();
#else
            1;
#endif

    if ( nrfactnms > 1 && !setup_.onlylocal_ )
    {
	protfld_ = new uiComboBox( this, "Protocol" );
	for ( int idx=0; idx<nrfactnms; idx++ )
	{
	    const File::SystemAccess& fsa = fsAccess( idx );
	    protfld_->addItem( fsa.userName() );
	    protfld_->setIcon( idx, fsa.iconName() );
	}
	if ( checkbox_ )
	    checkbox_->attach( leftOf, protfld_ );
	mAttachCB( protfld_->selectionChanged, uiFileSel::protChgCB );
	protfld_->setHSzPol( uiObject::Small );
    }

    fnmfld_ = new uiLineEdit( this, FileNameInpSpec(), "File Name" );
    setHAlignObj( fnmfld_ );
    fnmfld_->setHSzPol( uiObject::WideVar );

    if ( lbltxt.isEmpty() || checkbox_ )
    {
	if ( protfld_ )
	    protfld_->attach( leftOf, fnmfld_ );
	else if ( checkbox_ )
	    checkbox_->attach( leftOf, fnmfld_ );
    }
    else if ( !checkbox_ )
    {
	lbl_ = new uiLabel( this, lbltxt, fnmfld_ );
	if ( protfld_ )
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

    // needs to be done before finalising
    setFileNames( setup_.initialselection_ );

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


const char* uiFileSel::protKy( int protnr ) const
{
    if ( !factnms_.validIdx(protnr) )
	{ pErrMsg("Huh"); protnr = 0; }
    return factnms_.get( protnr ).str();
}


const File::SystemAccess& uiFileSel::fsAccess( int protnr ) const
{
    return File::SystemAccess::getByProtocol( protKy(protnr) );
}


const char* uiFileSel::curProtKy() const
{
    return protfld_ ? protKy( protfld_->currentItem() ) : "file";
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
    return lbl_ ? lbl_->text()
	 : (checkbox_ ? checkbox_->text() : uiString::empty());
}


void uiFileSel::setLabelText( const uiString& txt )
{
    if ( lbl_ )
	lbl_->setText( txt );
    else if ( checkbox_ )
	checkbox_->setText( txt );
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


void uiFileSel::setSel( const BufferStringSet& fnms )
{
    setText( uiFileSelTool::joinSelection(fnms) );
}


void uiFileSel::setFileNames( const BufferStringSet& fnms )
{
    if ( !setup_.displaylocalpath_ )
	{ setSel(fnms); return; }

    const BufferString seldir =
	File::Path(setup_.initialselectiondir_).fullPath();
    BufferStringSet dispfnms;
    for ( int idx=0; idx<fnms.size(); idx++ )
    {
	const BufferString filename = fnms.get( idx );
	if ( !filename.startsWith(seldir) )
	    dispfnms.add( filename );
	else
	{
	    const char* ptr = filename.str() + seldir.size();
	    if ( *ptr=='\\' || *ptr=='/' )
		dispfnms.add( ptr+1 );
	}
    }

    setSel( dispfnms );
}


void uiFileSel::setFileName( const char* fnm )
{
    BufferStringSet bss;
    if ( fnm && *fnm )
	bss.add( fnm );
    setFileNames( bss );
}


void uiFileSel::setButtonStates()
{
    if ( examinebut_ )
	examinebut_->setSensitive( File::exists(fileName()) );

    if ( protfld_ )
    {
	const int protnr = protfld_->currentItem();
	selbut_->setSensitive( fsAccess(protnr).queriesSupported() );
    }
}


void uiFileSel::protChgCB( CallBacker* )
{
    if ( !protfld_ )
	return;

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
    if ( protfld_ )
	protfld_->setText( selectedProtocol() );
    newSelection.trigger();
}


void uiFileSel::fnmEnteredCB( CallBacker* )
{
    if ( !setup_.isForWrite() || setup_.defaultextension_.isEmpty() )
	return;

    File::Path fp( fileName() );
    fp.setExtension( setup_.defaultextension_ );
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
    fnmfld_->setSensitive( ischecked );
    selbut_->setSensitive( ischecked );
    if ( protfld_ )
	protfld_->setSensitive( ischecked );
    if ( examinebut_ )
	examinebut_->setSensitive( ischecked );

    checked.trigger();
}


void uiFileSel::doSelCB( CallBacker* )
{
    uiFileSelector::Setup fssu( setup_ );
    fssu.initialselection_.setEmpty();
    getFileNames( fssu.initialselection_ );
    if ( fssu.initialselection_.isEmpty()
      && !setup_.initialselectiondir_.isEmpty() )
	fssu.initialselection_.add( setup_.initialselectiondir_ );

    const uiString seltyp( isDirectory(setup_.selmode_)
	    ? uiStrings::sDirectory().toLower() : uiStrings::sFile().toLower());

    const BufferString prot = curProtKy();
    const uiFileSelToolProvider& fsp = uiFileSelToolProvider::get( prot );
    PtrMan<uiFileSelTool> uifs = fsp.getSelTool( this, fssu );
    if ( !uifs )
	{ pErrMsg( BufferString("No selector for ",prot) ); return; }
    uifs->caption() = setup_.isForWrite() ? tr("Specify output %1 %2" )
					  : tr("Choose input %1 %2");
    uifs->caption().arg( objtype_ ).arg( seltyp );
    if ( !uifs->go() )
	return;

    BufferStringSet newselection;
    uifs->getSelected( newselection );
    setFileNames( newselection );
}


void uiFileSel::getFileNames( BufferStringSet& nms ) const
{
    uiFileSelTool::separateSelection( text(), nms );
    for ( int idx=0; idx<nms.size(); idx++ )
    {
	BufferString& fname = nms.get( idx );
	File::Path fp( fname );
	if ( !fp.isAbsolute() && !setup_.initialselectiondir_.isEmpty() )
	{
	    fp.insert( setup_.initialselectiondir_ );
	    fname = fp.fullPath();
	}
    }
}


const char* uiFileSel::fileName() const
{
    BufferStringSet nms;
    getFileNames( nms );
    mDeclStaticString( fname );
    if ( nms.isEmpty() )
	fname.setEmpty();
    else
	fname.set( nms.get(0) );
    return fname.buf();
}


void uiFileSel::setSelectionMode( SelectionMode mode )
{
    const bool tosingle = isSingle( mode ) && !isSingle( setup_.selmode_ );
    setup_.selmode_ = mode;
    if ( tosingle )
    {
	BufferString curtxt = text();
	char* ptrsep = curtxt.find( ';' );
	if ( ptrsep )
	{
	    *ptrsep = '\0';
	    setText( curtxt );
	}
    }
}


void uiFileSel::examineFileCB( CallBacker* )
{
    File::ViewPars vp( setup_.examstyle_ );
    vp.editable_ = setup_.exameditable_;
    if ( !File::launchViewer(fileName(),vp) )
	uiMSG().error( tr("Cannot launch file browser") );
}
