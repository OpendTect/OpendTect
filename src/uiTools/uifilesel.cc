/*+
________________________________________________________________________

 Copyright:	(C) 1995-2023 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uifilesel.h"

#include "filepath.h"
#include "filesystemaccess.h"

#include "uibutton.h"
#include "uicombobox.h"
#include "uifileseltool.h"
#include "uilabel.h"
#include "uilineedit.h"
#include "uimsg.h"
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
	formats_.addFormat( FileFormat::imageFiles() );
}


uiFileSel::uiFileSel( uiParent* p, const uiString& txt, const Setup& setp )
    : uiGroup(p,"File input")
    , newSelection(this)
    , acceptReq(this)
    , checked(this)
    , protocolChanged(this)
    , setup_(setp)
{
    init( txt );
}


uiFileSel::uiFileSel( uiParent* p, const uiString& txt, const char* fnm )
    : uiGroup(p,"File input")
    , newSelection(this)
    , acceptReq(this)
    , checked(this)
    , protocolChanged(this)
    , setup_(fnm)
{
    setup_.withexamine( true );
    init( txt );
}


void uiFileSel::init( const uiString& lbltxt )
{
    if ( setup_.checkable_ )
    {
	checkbox_ = new uiCheckBox( this, lbltxt );
	checkbox_->setChecked( true );
	checkbox_->activated.notify( mCB(this,uiFileSel,checkCB) );
    }

    const bool forread = setup_.isForRead();
    uiStringSet protnms;
    OD::FileSystemAccess::getProtocolNames( factnms_, forread );
    const int nrfactnms = factnms_.size();
    if ( nrfactnms > 1 && !setup_.onlylocal_ )
    {
	protfld_ = new uiComboBox( this, "Protocol" );
	for ( int idx=0; idx<nrfactnms; idx++ )
	{
	    const OD::FileSystemAccess& fsa = fsAccess( idx );
	    protfld_->addItem( fsa.userName() );
	    protfld_->setIcon( idx, fsa.iconName() );
	}
	if ( checkbox_ )
	    checkbox_->attach( leftOf, protfld_ );
	mAttachCB( protfld_->selectionChanged, uiFileSel::protChgCB );
	protfld_->setHSzPol( uiObject::SmallVar );
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

    mAttachCB( fnmfld_->editingFinished, uiFileSel::inputChgCB );
    mAttachCB( fnmfld_->returnPressed, uiFileSel::fnmEnteredCB );
    mAttachCB( postFinalize(), uiFileSel::finalizeCB );
}


uiFileSel::~uiFileSel()
{
    detachAllNotifiers();
}


void uiFileSel::finalizeCB( CallBacker* )
{
    setButtonStates();
    checkCB( nullptr );
}


const char* uiFileSel::protKy( int protnr ) const
{
    if ( !factnms_.validIdx(protnr) )
    {
	pErrMsg("Huh");
	protnr = 0;
    }

    return factnms_.get( protnr ).str();
}


const OD::FileSystemAccess& uiFileSel::fsAccess( int protnr ) const
{
    return OD::FileSystemAccess::getByProtocol( protKy(protnr) );
}


const char* uiFileSel::curProtKy() const
{
    return protfld_ ? protKy( protfld_->currentItem() ) : "file";
}


BufferString uiFileSel::selectedExtension() const
{
    const char* fnm = fileName();
    return BufferString( FilePath(fnm).extension() );
}


BufferString uiFileSel::selectedProtocol() const
{
    const char* fnm = fileName();
    return OD::FileSystemAccess::getProtocol( fnm );
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
    if ( StringView(s) == fnmfld_->text() )
	return;

    fnmfld_->setText( s );
    fnmfld_->end();
    inputChgCB( nullptr );
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
    {
	setSel( fnms );
	return;
    }

    const BufferString seldir =
	FilePath(setup_.initialselectiondir_).fullPath();
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
    const BufferString prot( OD::FileSystemAccess::getProtocol(fnm) );
    const BufferString newprot = factnms_.get( protfld_->currentItem() );
    if ( prot != newprot )
    {
	NotifyStopper ns( newSelection );
	const BufferString filenmonly = FilePath(fnm).fileName();
	if ( !filenmonly.isEmpty() )
	    ns.enableNotification();

	if ( filenmonly.isEmpty() )
	{
	    fnmfld_->setPlaceholderText( toUiString(newprot) );
	}
	else
	{
	    fnm = OD::FileSystemAccess::withProtocol( filenmonly, newprot );
	    setFileName( fnm );
	}
    }
    else
	fnmfld_->setPlaceholderText( uiString::empty() );

    setButtonStates();
    protocolChanged.trigger();
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

    FilePath fp( fileName() );
    fp.setExtension( setup_.defaultextension_ );
    setFileName( fp.fullPath() );

    acceptReq.trigger();
}


void uiFileSel::setSensitive( bool yn )
{
    setChildrenSensitive( yn );
    if ( yn )
	checkCB( nullptr );
}


void uiFileSel::setChecked( bool yn )
{
    if ( checkbox_ )
    {
	checkbox_->setChecked( yn );
	checkCB(nullptr);
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
    Setup fssu( setup_ );
    fssu.initialselection_.setEmpty();
    getFileNames( fssu.initialselection_ );
    if ( fssu.initialselection_.isEmpty()
      && !setup_.initialselectiondir_.isEmpty() )
	fssu.initialselection_.add( setup_.initialselectiondir_ );

    const uiString seltyp( setup_.isForDirectory()
	    ? uiStrings::sDirectory().toLower() : uiStrings::sFile().toLower());

    const BufferString prot = curProtKy();
    const uiFileSelToolProvider& fsp = uiFileSelToolProvider::get( prot );
    PtrMan<uiFileSelTool> uifs = fsp.getSelTool( this, fssu );
    if ( !uifs )
    {
	pErrMsg( BufferString("No selector for ",prot) );
	return;
    }

    uifs->caption() = setup_.isForWrite() ? tr("Specify output %1 %2" )
					  : tr("Choose input %1 %2");
    uifs->caption().arg( objtype_ ).arg( seltyp );
    if ( !uifs->go() )
	return;

    filepars_.setEmpty();
    BufferStringSet newselection;
    uifs->getSelected( newselection );
    if ( newselection.isEmpty() )
	return;

    filepars_.set( sKey::FileName(), newselection.get(0).buf() );
    ConstPtrMan<IOPar> pars = uifs->getPars();
    if ( pars )
	filepars_.merge( *pars );

    setFileNames( newselection );
}


const char* uiFileSel::protocol() const
{
    if ( !protfld_ )
	return "file";

    const int currentprotocol = protfld_->currentItem();
    return protKy( currentprotocol );
}


void uiFileSel::getFileNames( BufferStringSet& nms ) const
{
    uiFileSelTool::separateSelection( text(), nms );
    for ( int idx=0; idx<nms.size(); idx++ )
    {
	BufferString& fname = nms.get( idx );
	FilePath fp( fname );
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
    const bool tosingle = mode!=OD::SelectMultiFile && !setup_.isSingle();
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
