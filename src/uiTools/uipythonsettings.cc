/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uipythonsettings.h"

#include "bufstring.h"
#include "commanddefs.h"
#include "commandlaunchmgr.h"
#include "envvars.h"
#include "file.h"
#include "filepath.h"
#include "keyboardevent.h"
#include "mousecursor.h"
#include "oddirs.h"
#include "oscommand.h"
#include "od_helpids.h"
#include "ptrman.h"
#include "pythonaccess.h"
#include "settings.h"

#include "uiaction.h"
#include "uibuttongroup.h"
#include "uidesktopservices.h"
#include "uigeninput.h"
#include "uifileinput.h"
#include "uilistbox.h"
#include "uimain.h"
#include "uimenu.h"
#include "uimsg.h"
#include "uipathsel.h"
#include "uipixmap.h"
#include "uiseparator.h"
#include "uisettings.h"
#include "uistrings.h"
#include "uitextedit.h"
#include "uitoolbar.h"
#include "uitoolbarcmded.h"
#include "uitoolbutton.h"


namespace sKey {
    inline StringView PythonIDE()	{ return "PythonIDE"; }
    inline StringView PythonTerm()	{ return "PythonTerm"; }
};


mExpClass(uiTools) uiSafetyCheckDlg : public uiDialog
{
mODTextTranslationClass(uiSafetyCheckDlg)
public:
			uiSafetyCheckDlg(uiParent*);
			~uiSafetyCheckDlg();

private:
    void		finalizeCB(CallBacker*);
    void		pyupCB(CallBacker*);
    void		readSettings();
    void		saveSettings();
    void		modeCB(CallBacker*);
    void		checkCB(CallBacker*);
    void		licenseCB(CallBacker*);
    void		saveCB(CallBacker*);
    void		scan(const char* command);

    uiGenInput*		modefld_;
    uiGenInput*		apikeyfld_;
    uiTextBrowser*	outputfld_;
    uiToolButton*	savebut_;
};


uiSafetyCheckDlg::uiSafetyCheckDlg( uiParent* p )
    : uiDialog(p,Setup(tr("Scan for Vulnerabilities"),mNoDlgTitle,mTODOHelpKey))
{
    setCtrlStyle( CloseOnly );

    auto* safetybut = new uiPushButton( this, uiString::empty() );
    safetybut->setIcon( "safety" );
    safetybut->setMinimumHeight( 50 );
    safetybut->setMinimumWidth( 50 );
    safetybut->setFlat( true );
    safetybut->setToolTip( tr("For more information on safety or to request\n"
			      "an API key, click here to visit pyup.io") );
    mAttachCB( safetybut->activated, uiSafetyCheckDlg::pyupCB );

    modefld_ = new uiGenInput( this, tr("Database"),
			BoolInpSpec(true,tr("Commercial"),tr("Open-source")) );
    mAttachCB( modefld_->valueChanged, uiSafetyCheckDlg::modeCB );
    modefld_->attach( rightTo, safetybut );

    apikeyfld_ = new uiGenInput( this, tr("API key") );
    apikeyfld_->setElemSzPol( uiObject::Wide );
    apikeyfld_->attach( alignedBelow, modefld_ );

    auto* sep = new uiSeparator( this, "" );
    sep->attach( stretchedBelow, apikeyfld_ );
    sep->attach( ensureBelow, safetybut );

    auto* grp = new uiButtonGroup( this, "Buttons", OD::Horizontal );
    grp->attach( ensureBelow, sep );
    auto* checkbut = new uiPushButton( grp, tr("Check"), true );
    mAttachCB( checkbut->activated, uiSafetyCheckDlg::checkCB );

    auto* licensebut = new uiPushButton( grp, tr("License"), true );
    mAttachCB( licensebut->activated, uiSafetyCheckDlg::licenseCB );

    savebut_ = new uiToolButton( grp, "save", tr("Save Report"),
			mCB(this,uiSafetyCheckDlg,saveCB) );
    savebut_->setSensitive( false );

    outputfld_ = new uiTextBrowser( this );
    outputfld_->setPrefWidthInChar( 90 );
    outputfld_->attach( centeredBelow, grp );

    mAttachCB( postFinalize(), uiSafetyCheckDlg::finalizeCB );
}


uiSafetyCheckDlg::~uiSafetyCheckDlg()
{
    detachAllNotifiers();
}


void uiSafetyCheckDlg::finalizeCB( CallBacker* )
{
    readSettings();
}


void uiSafetyCheckDlg::pyupCB( CallBacker* )
{
    const char* url = "https://pyup.io/";
    uiDesktopServices::openUrl( url );
}


void uiSafetyCheckDlg::modeCB( CallBacker* )
{
    const bool needsapikey = modefld_->getBoolValue();
    apikeyfld_->display( needsapikey );
}


static Settings& getPythonSettings()
{
    BufferString pythonstr( sKey::Python() ); pythonstr.toLower();
    return Settings::fetch( pythonstr );
}


static const char* sKeySafety()
{ return "Safety"; }

static const char* sKeySafetyAPIKey()
{
    return IOPar::compKey( sKeySafety(), "APIKey" );
}


void uiSafetyCheckDlg::readSettings()
{
    BufferString apikey;
    const Settings& pythonsetts = getPythonSettings();
    const bool hasentry = pythonsetts.get( sKeySafetyAPIKey(), apikey );
    modefld_->setValue( !hasentry || !apikey.isEmpty() );
    apikeyfld_->setText( apikey );
}


void uiSafetyCheckDlg::saveSettings()
{
    Settings& pythonsetts = getPythonSettings();
    pythonsetts.set( sKeySafetyAPIKey(), apikeyfld_->text() );
    pythonsetts.write();
}


void uiSafetyCheckDlg::scan( const char* command )
{
    outputfld_->setText( "" );
    savebut_->setSensitive( false );

    const bool needsapikey = modefld_->getBoolValue();
    SetEnvVar( "SAFETY_SOURCE", "dGB" );
    SetEnvVar( "SAFETY_CUSTOM_INTEGRATION", "True" );

    OS::MachineCommand mc;
    OD::PythA().setForScript( "safety", mc );
    mc.addArg( command );
    mc.addKeyedArg( "output", "text" );
    if ( needsapikey )
    {
	const BufferString apikey = apikeyfld_->text();
	if ( apikey.isEmpty() )
	{
	    uiMSG().error( tr("Please provide an API key") );
	    return;
	}

// Use the db argument in Safety 2.4
//	const BufferString db = "https://pyup.io/aws/safety/dgb/2.0.0/";
//	mc.addKeyedArg( "db", db.buf() );
	mc.addKeyedArg( "key", apikey.buf() );
	saveSettings();
    }

    MouseCursorChanger mcc( MouseCursor::Wait );
    BufferString stdoutstr, stderrstr;
    uiRetVal ret;
    OD::PythA().execute( mc, stdoutstr, ret, &stderrstr );
    outputfld_->setText( stdoutstr.isEmpty()
			   ? (stderrstr.isEmpty() ? "" : stderrstr.buf())
			   : stdoutstr.buf() );

    const bool nooutput = stdoutstr.isEmpty() && stderrstr.isEmpty();
    savebut_->setSensitive( !nooutput );
}


void uiSafetyCheckDlg::checkCB( CallBacker* )
{ scan( "check" ); }


void uiSafetyCheckDlg::licenseCB( CallBacker* )
{
    const bool useos = !modefld_->getBoolValue();
    if ( useos )
    {
	uiMSG().error(
		tr("This feature can only be used with a valid API key.\n"
		   "Please send an email to info@dgbes.com for "
		   "more information.") );
	return;
    }

    scan( "license" ); }


void uiSafetyCheckDlg::saveCB( CallBacker* )
{
    const char* defaultfilenm = "safetyreport.log";
    const char* filter = "Log files (*.log)";
    FilePath fp = GetPersonalDir();
    fp.add( defaultfilenm );
    uiFileDialog dlg( this, false, fp.fullPath(), filter );
    dlg.setAllowAllExts( true );
    if ( !dlg.go() )
	return;

    outputfld_->saveToFile( dlg.fileName() );
}




uiPythonSettings::uiPythonSettings(uiParent* p, const char* nm )
		: uiDialog(p, uiDialog::Setup(toUiString(nm),
		tr("Set Python environment"),mODHelpKey(mPythonSettingsHelpID)))
{
    setIcon( uiPixmap("python") );
    pythonsrcfld_ = new uiGenInput(this, tr("Python environment"),
		StringListInpSpec(OD::PythonSourceDef().strings()));

    if ( !OD::PythonAccess::hasInternalEnvironment(false) )
    {
	internalloc_ = new uiFileInput( this, tr("Internal environment root") );
	internalloc_->setSelectMode( uiFileDialog::DirectoryOnly );
	internalloc_->attach( alignedBelow, pythonsrcfld_ );
    }

    customloc_ = new uiFileInput( this, tr("Custom environment root") );
    customloc_->setSelectMode( uiFileDialog::DirectoryOnly );
    customloc_->attach( alignedBelow, pythonsrcfld_ );

    customenvnmfld_ = new uiGenInput( this, tr("Virtual environment"),
				      StringListInpSpec() );
    customenvnmfld_->setWithCheck();
    customenvnmfld_->setElemSzPol( uiObject::WideVar );
    customenvnmfld_->attach( alignedBelow, customloc_ );

    auto* sep1 = new uiSeparator( this );
    sep1->attach( stretchedBelow, customenvnmfld_ );

    custompathfld_ = new uiPathSel( this, tr("Custom Module Path") );
    custompathfld_->attach( alignedBelow, customenvnmfld_ );
    custompathfld_->attach( ensureBelow, sep1 );

    auto* sep2 = new uiSeparator( this );
    sep2->attach( stretchedBelow, custompathfld_ );

    pyidefld_ = new uiToolBarCommandEditor( this,
					    tr("Python IDE Command"),
					    getPythonIDECommands(),
					    true, false );
    pyidefld_->attach( alignedBelow, custompathfld_ );
    pyidefld_->attach( ensureBelow, sep2 );
    pyidefld_->setChecked( false );

    auto* sep3 = new uiSeparator( this );
    sep3->attach( stretchedBelow, pyidefld_ );
    pytermfld_ = new uiToolBarCommandEditor( this,
					     tr("Python Console Command"),
			 CommandDefs::getTerminalCommands(BufferStringSet()),
					     true, false );
    pytermfld_->attach( alignedBelow, pyidefld_ );
    pytermfld_->attach( ensureBelow, sep3 );
    pytermfld_->setChecked( false );

    auto* testbut = new uiPushButton( this, tr("Test"),
			mCB(this,uiPythonSettings,testCB), true);
    testbut->setIcon( "test" );
    testbut->attach( ensureBelow, pytermfld_ );

    auto* cmdwinbut = new uiPushButton( this, tr("Launch Prompt"),
			mCB(this,uiPythonSettings,promptCB), true );
    cmdwinbut->setIcon( "terminal" );
    cmdwinbut->attach( rightOf, testbut );

    auto* safetychkbut = new uiPushButton( this, tr("Safety Check"),
			mCB(this,uiPythonSettings,safetycheckCB), true );
    safetychkbut->setIcon( "safety" );
    safetychkbut->attach( rightOf, cmdwinbut );

    clonebut_ = new uiPushButton( this, tr("Clone Environment"),
				  mCB(this,uiPythonSettings,cloneCB), true );
    clonebut_->setIcon( "copyobj" );
    clonebut_->attach( rightOf, safetychkbut );

    mAttachCB( postFinalize(), uiPythonSettings::initDlg );
}


uiPythonSettings::~uiPythonSettings()
{
    detachAllNotifiers();
}


void uiPythonSettings::initDlg( CallBacker* )
{
    usePar( curSetts() );
    fillPar( initialsetts_ ); //Backup for restore
    sourceChgCB(0);
    updateIDEfld();

    mAttachCB( pythonsrcfld_->valueChanged, uiPythonSettings::sourceChgCB );
    if ( internalloc_ )
	mAttachCB( internalloc_->valueChanged,
		   uiPythonSettings::internalLocChgCB );
    mAttachCB( customloc_->valueChanged, uiPythonSettings::customEnvChgCB );
    mAttachCB( customenvnmfld_->valueChanged, uiPythonSettings::parChgCB );
    mAttachCB( customenvnmfld_->checked, uiPythonSettings::parChgCB );
    mAttachCB( custompathfld_->selChange, uiPythonSettings::parChgCB );
    mAttachCB( pyidefld_->changed, uiPythonSettings::parChgCB );
    mAttachCB( pyidefld_->checked, uiPythonSettings::parChgCB );
    mAttachCB( pytermfld_->changed, uiPythonSettings::parChgCB );
    mAttachCB( pytermfld_->checked, uiPythonSettings::parChgCB );
}

IOPar& uiPythonSettings::curSetts()
{
    BufferString pythonstr( sKey::Python() );
    return Settings::fetch( pythonstr.toLower() );
}

void uiPythonSettings::getChanges()
{
    IOPar* workpar = nullptr;
    if ( chgdsetts_ )
	workpar = chgdsetts_;
    const bool alreadyedited = workpar;
    if ( alreadyedited )
	workpar->setEmpty();
    else
	workpar = new IOPar;

    fillPar( *workpar );
    if ( !curSetts().isEqual(*workpar) )
    {
	if ( !alreadyedited )
	    chgdsetts_ = workpar;
    }
    else
    {
	if ( alreadyedited )
	    chgdsetts_ = nullptr;
	delete workpar;
    }
}

void uiPythonSettings::fillPar( IOPar& par ) const
{
    BufferString pythonstr( sKey::Python() );
    par.setName( pythonstr.toLower() );

    const int sourceidx = pythonsrcfld_->getIntValue();
    const OD::PythonSource source =
		OD::PythonSourceDef().getEnumForIndex(sourceidx);
    par.set(OD::PythonAccess::sKeyPythonSrc(),
		OD::PythonSourceDef().getKey(source));

    if ( source == OD::Internal && internalloc_ )
    {
	const BufferString envroot( internalloc_->fileName() );
	if ( !envroot.isEmpty() )
	    par.set( OD::PythonAccess::sKeyEnviron(), envroot );
    }
    else if ( source == OD::Custom )
    {
	const BufferString envroot( customloc_->fileName() );
	if ( !envroot.isEmpty() )
	{
	    par.set( OD::PythonAccess::sKeyEnviron(), envroot );
	    if ( customenvnmfld_->isChecked() )
		par.set( sKey::Name(), customenvnmfld_->text() );
	    else
		par.removeWithKey( sKey::Name() );
	}
    }

    if ( custompathfld_->isChecked() )
    {
	const BufferStringSet paths = custompathfld_->getPaths();
	if ( !paths.isEmpty() )
	{
	    IOPar tmp;
	    paths.fillPar( tmp );
	    par.mergeComp( tmp, OD::PythonAccess::sKeyPythonPath() );
	}
    }

    IOPar idecmd;
    pyidefld_->fillPar( idecmd );
    if ( !idecmd.isEmpty() )
	par.mergeComp( idecmd, sKey::PythonIDE() );

    IOPar termcmd;
    pytermfld_->fillPar( termcmd );
    if ( !termcmd.isEmpty() )
	par.mergeComp( termcmd, sKey::PythonTerm() );
}

void uiPythonSettings::usePar( const IOPar& par )
{
    OD::PythonSource source;
    if ( !OD::PythonSourceDef().parse(par,
		OD::PythonAccess::sKeyPythonSrc(), source) )
    {
	source = OD::PythonAccess::hasInternalEnvironment(false)
	       ? OD::Internal : OD::System;
    }

    pythonsrcfld_->setValue( source );
    customenvnmfld_->setChecked( false );
    if ( source == OD::Internal && internalloc_ )
    {
	BufferString envroot;
	if ( par.get(OD::PythonAccess::sKeyEnviron(),envroot) )
	    internalloc_->setFileName( envroot );
    }
    else if ( source == OD::Custom )
    {
	BufferString envroot, envnm;
	if ( par.get(OD::PythonAccess::sKeyEnviron(),envroot) )
	{
	    customloc_->setFileName( envroot );
	    setCustomEnvironmentNames();
	}

	customenvnmfld_->setChecked( par.get(sKey::Name(),envnm) &&
				     !envnm.isEmpty() );
	if ( customenvnmfld_->isChecked() )
	    customenvnmfld_->setText( envnm );
    }
    PtrMan<IOPar> pathpar = par.subselect( OD::PythonAccess::sKeyPythonPath() );
    if ( pathpar )
    {
	BufferStringSet paths;
	paths.usePar( *pathpar );
	custompathfld_->setPaths( paths );
    }
    custompathfld_->setChecked( pathpar );

    PtrMan<IOPar> idepar = par.subselect( sKey::PythonIDE() );
    if ( idepar )
	pyidefld_->usePar( *idepar );
    pyidefld_->setChecked( idepar );
    updateIDEfld();

    PtrMan<IOPar> termpar = par.subselect( sKey::PythonTerm() );
    if ( termpar )
	pytermfld_->usePar( *termpar );
    pytermfld_->setChecked( termpar );
}

bool uiPythonSettings::commitSetts( const IOPar& iop )
{
    Settings& setts = Settings::fetch( iop.name() );
    setts.IOPar::operator =( iop );
    if ( !setts.write(false) )
    {
	uiMSG().error( tr( "Cannot write %1" ).arg( setts.name() ) );
	return false;
    }

    return true;
}

void uiPythonSettings::sourceChgCB( CallBacker* )
{
    const int sourceidx = pythonsrcfld_->getIntValue();
    const OD::PythonSource source =
		OD::PythonSourceDef().getEnumForIndex( sourceidx );

    if ( internalloc_ )
	internalloc_->display( source == OD::Internal );

    const bool iscustom = source == OD::Custom;
    customloc_->display( iscustom );
    customenvnmfld_->display( iscustom );
    clonebut_->display( source != OD::System );

    updateIDEfld();
    parChgCB( nullptr );
}

void uiPythonSettings::customEnvChgCB( CallBacker* )
{
    setCustomEnvironmentNames();
    updateIDEfld();
    parChgCB( nullptr );
}


void uiPythonSettings::internalLocChgCB( CallBacker* )
{
    updateIDEfld();
    parChgCB( nullptr );
}

void uiPythonSettings::parChgCB( CallBacker* )
{
    getChanges();
}


void uiPythonSettings::setCustomEnvironmentNames()
{
    const BufferString envroot( customloc_->fileName() );
    const FilePath fp( envroot, "envs" );
    if ( !fp.exists() )
	return;

    ManagedObjectSet<FilePath> fps;
    BufferStringSet envnames;
    const FilePath externalroot( envroot );
    OD::PythonAccess::getSortedVirtualEnvironmentLoc( fps, envnames, nullptr,
						      &externalroot );
    for ( int idx=envnames.size()-1; idx>=0; idx-- )
    {
	if ( envnames.get(idx).isEmpty() )
	    envnames.removeSingle(idx);
    }

    customenvnmfld_->setEmpty();
    customenvnmfld_->newSpec( StringListInpSpec(envnames), 0 );
    customenvnmfld_->setChecked( !envnames.isEmpty() );
    customenvnmfld_->display( !envnames.isEmpty() );
}

void uiPythonSettings::testPythonModules()
{
    uiUserShowWait usw( this,
			tr("Retrieving list of installed Python modules") );
    ManagedObjectSet<OD::PythonAccess::ModuleInfo> modules;
    const uiRetVal uirv = OD::PythA().getModules( modules );
    if ( !uirv.isOK() )
    {
	uiMSG().error( uirv );
	return;
    }

    BufferStringSet modstrs;
    for ( const auto* mod : modules )
	modstrs.add( mod->displayStr() );

    usw.readyNow();
    uiDialog dlg( this, uiDialog::Setup(tr("Python Installation"),mNoDlgTitle,
					mNoHelpKey) );
    dlg.setCtrlStyle( uiDialog::CloseOnly );
    auto* pythfld = new uiGenInput( &dlg, tr("Using") );
    pythfld->setText( OD::PythA().pyVersion() );
    pythfld->setReadOnly();
    pythfld->setElemSzPol( uiObject::Wide );
    auto* modfld = new uiLabeledListBox( &dlg, tr("Detected modules") );
    modfld->addItems( modstrs );
    modfld->attach( alignedBelow, pythfld );
    dlg.go();
}

void uiPythonSettings::testCB(CallBacker*)
{
    if ( !useScreen() )
	return;

    uiUserShowWait usw( this, tr("Retrieving Python testing") );
    const uiRetVal ret = OD::PythA().isUsable( true );
    if ( !ret.isOK() )
    {
	uiRetVal uirv = tr("Cannot detect Python version:");
	uirv.add( ret );
	uiMSG().error( uirv );
	return;
    }

    usw.readyNow();
    testPythonModules();
}


void uiPythonSettings::promptCB( CallBacker* )
{
    if ( !useScreen() )
	return;

    uiRetVal uirv = uiSettsMgr().openTerminal( false );
    if ( !uirv.isOK() )
    {
	uirv.add( tr("Python environment not usable") );
	uiMSG().error( uirv );
    }
}


void uiPythonSettings::safetycheckCB( CallBacker* )
{
    if ( !useScreen() )
	return;

    uiSafetyCheckDlg dlg( this );
    dlg.go();
}


class uiCloneEnvDlg : public uiDialog
{
mODTextTranslationClass(uiCloneEnvDlg)
public:
uiCloneEnvDlg( uiParent* p, const FilePath& envpath )
    : uiDialog(p,Setup(tr("Clone Python Environment"),mNoDlgTitle,mTODOHelpKey))
{
    sourcefld_ = new uiGenInput( this, uiStrings::sSource() );
    sourcefld_->setFilename( envpath.fullPath().buf() );
    sourcefld_->setReadOnly( true );

    uiFileInput::Setup fsu;
    destfld_ = new uiFileInput( this, tr("Destination folder"), fsu );
    destfld_->setSelectMode( uiFileDialog::DirectoryOnly );
    destfld_->attach( alignedBelow, sourcefld_ );

    const BufferString defnm( envpath.baseName(), "_clone" );
    destnmfld_ = new uiGenInput( this, tr("New environment name") );
    destnmfld_->setText( defnm );
    destnmfld_->attach( alignedBelow, destfld_ );

    setOkText( tr("Clone") );
}


bool acceptOK( CallBacker* )
{
    const BufferString destdir = destfld_->fileName();
    if ( destdir.isEmpty() )
    {
	uiMSG().error( tr("Please selection destionation folder.") );
	return false;
    }

    const BufferString clonename = destnmfld_->text();
    const FilePath fp( destdir, clonename );
    destdir_ = fp.fullPath();
    if ( File::exists(destdir_) )
    {
	uiMSG().error( tr("Destination already exists:\n%1")
			.arg(destdir_) );
	return false;
    }

    if ( !File::isWritable(destdir) )
    {
	uiMSG().error( tr("Destination is not writable.") );
	return false;
    }

    return true;
}

const char* destDir() const
{
    return destdir_.buf();
}

BufferString	destdir_;

uiGenInput*	sourcefld_;
uiFileInput*	destfld_;
uiGenInput*	destnmfld_;

};

void uiPythonSettings::cloneCB( CallBacker* )
{
    if ( !useScreen() )
	return;

    FilePath envpath;
    OD::PythA().GetPythonEnvPath( envpath );
    const BufferString envname( envpath.baseName() );
    if ( envpath.isEmpty() )
    {
	uiMSG().error(
		tr("Can only clone Internal or Custom Python environments") );
	return;
    }

    uiCloneEnvDlg dlg( this, envpath );
    if ( !dlg.go() )
	return;

    const char* destdir = dlg.destDir();
    BufferStringSet mcargs;
    mcargs.add("create").add("--clone").add(envname.buf())
	  .add("-p").add(destdir);
    const OS::MachineCommand mc( "conda", mcargs );
    auto& mgr = Threads::CommandLaunchMgr::getMgr();
    CallBack cb( mCB(this,uiPythonSettings,cloneFinishedCB) );
    setButtonSensitive( OK, false );
    setButtonSensitive( CANCEL, false );
    mgr.execute( mc, true, true, &cb, true );
}


void uiPythonSettings::cloneFinishedCB( CallBacker* cb )
{
    const auto* ct = Threads::CommandLaunchMgr::getMgr().getCommandTask( cb );
    if ( ct )
    {
	const BufferString stdoutstr = ct->getStdOutput();
	const BufferString stderrstr = ct->getStdError();
	if ( !stderrstr.isEmpty() )
	{
	    uiMSG().errorWithDetails( toUiString(stderrstr),
				      tr("Error cloning conda environment") );
	    return;
	}

	uiMSG().message( tr("Cloning conda environment completed") );

	setCustomEnvironmentNames();
    }
    setButtonSensitive( OK, true );
    setButtonSensitive( CANCEL, true );
}


bool uiPythonSettings::getPythonEnvBinPath( BufferString& pybinpath ) const
{
    pybinpath.setEmpty();
    const int sourceidx = pythonsrcfld_->getIntValue();
    const OD::PythonSource source =
			OD::PythonSourceDef().getEnumForIndex(sourceidx);
    FilePath pypath;
    if ( source == OD::Internal )
    {
	if ( OD::PythonAccess::hasInternalEnvironment(false) )
	    OD::PythonAccess::GetPythonEnvPath( pypath );
	else if ( internalloc_ )
	    pypath = FilePath( internalloc_->fileName() );
    }
    else if ( source == OD::Custom )
    {
	pypath = FilePath( customloc_->fileName() );
	const BufferString envnm( customenvnmfld_->text() );
	pypath.add( "envs" ).add( envnm );
	if ( !pypath.exists() )
	{
	    ManagedObjectSet<FilePath> fps;
	    OD::PythonAccess::getCondaEnvFromTxtPath( fps );
	    for ( const auto fp : fps )
	    {
		if ( fp->fullPath() == envnm )
		{
		    pypath = *fp;
		    break;
		}
	    }
	}
    }

    if ( !pypath.isEmpty() )
    {
#ifdef __win__
	pypath.add( "Scripts" );
#else
	pypath.add( "bin" );
#endif
	pybinpath = pypath.fullPath();
	if ( !File::exists(pybinpath) || !File::isDirectory(pybinpath) )
	    return false;
    }

    return true;
}


void uiPythonSettings::updateIDEfld()
{
    getChanges();
    if ( !useScreen() )
	return;

    pyidefld_->updateCmdList( getPythonIDECommands() );
}


bool uiPythonSettings::useScreen()
{
    const int sourceidx = pythonsrcfld_->getIntValue();
    const OD::PythonSource source =
		OD::PythonSourceDef().getEnumForIndex(sourceidx);

    uiString envrootstr = tr("Environment root" );
    if ( source == OD::Internal && internalloc_ )
    {
	const BufferString envroot( internalloc_->fileName() );
	if ( !File::exists(envroot) || !File::isDirectory(envroot) )
	{
	    uiMSG().error( uiStrings::phrSelect(envrootstr) );
	    return false;
	}

	const FilePath envrootfp( envroot );
	if ( !OD::PythonAccess::validInternalEnvironment(envrootfp) )
	{
	    uiMSG().error( tr("Invalid %1").arg(envrootstr) );
	    return false;
	}
    }
    else if ( source == OD::Custom )
    {
	const BufferString envroot( customloc_->fileName() );
	if ( !File::exists(envroot) || !File::isDirectory(envroot) )
	{
	    uiMSG().error( uiStrings::phrSelect(envrootstr) );
	    return false;
	}

	const FilePath envrootfp( envroot, "envs" );
	if ( !envrootfp.exists() )
	{
	    uiMSG().error( tr("%1 does not contain a folder called %2")
				.arg(envrootstr).arg("'envs'") );
	    return false;
	}
    }

    if ( !chgdsetts_ )
	return true;

    needrestore_ = chgdsetts_;
    if ( commitSetts(*chgdsetts_) )
	deleteAndNullPtr( chgdsetts_ );

    return chgdsetts_ ? false : true;
}

bool uiPythonSettings::rejectOK( CallBacker* )
{
    if ( !needrestore_ )
	return true;

    if ( commitSetts(initialsetts_) )
	OD::PythA().isUsable( true );
    else
	uiMSG().warning( tr("Cannot restore the initial settings") );

    return true;
}

bool uiPythonSettings::acceptOK( CallBacker* )
{
    bool isok = true; bool ismodified = false;
    if ( chgdsetts_ )
    {
	isok = useScreen();
	ismodified = true;
    }

    needrestore_ = !isok;
    if ( isok )
    {
	if ( ismodified )
	    needrestore_ = !OD::PythA().isUsable( true ).isOK();
    }
    else
	uiMSG().warning( tr("Cannot use the new settings") );

    return isok;
}


CommandDefs uiPythonSettings::getPythonIDECommands()
{
    BufferStringSet paths;
    if ( OD::PythA().getPythonSource()!=OD::System )
    {
	FilePath pybinpath;
	OD::PythonAccess::GetPythonEnvBinPath( pybinpath );
	paths.add( pybinpath.fullPath() );
    }

    const BufferStringSet spyderargs( "--new-instance" );

    CommandDefs comms;
    comms.addCmd( "jupyter-lab", tr("Jupyter-Lab"), "jupyter-lab.png",
		  tr("Jupyter Lab"), paths );
    comms.addCmd( "jupyter-notebook", tr("Jupyter-Notebook"),
		  "jupyter-notebook.png", tr("Jupyter Notebook"), paths );
    comms.addCmd( "spyder", tr("Spyder"), "spyder.png", tr("Spyder"), paths,
		  &spyderargs );
    const BufferString idlecmd( __iswin__ ? "idle" : "idle3" );
    comms.addCmd( idlecmd, tr("Idle"), "idle.png", tr("Idle"), paths );

    return comms;
}


uiDialog* uiSettings::getPythonDlg( uiParent* p, const char* nm )
{
    auto* ret = new uiPythonSettings( p, nm );
    ret->setModal( false );
    ret->setDeleteOnClose( true );
    return ret;
}


// uiSettingsMgr
uiSettingsMgr& uiSettsMgr()
{
    mDefineStaticLocalObject( PtrMan<uiSettingsMgr>, theinst, = nullptr );
    return *theinst.createIfNull();
}


uiSettingsMgr::uiSettingsMgr()
    : terminalRequested(this)
    , toolbarUpdated(this)
{
    progargs_.setNullAllowed();
    mAttachCB( uiMain::keyboardEventHandler().keyPressed,
		uiSettingsMgr::keyPressedCB );
}


uiSettingsMgr::~uiSettingsMgr()
{
    detachAllNotifiers();
    deepErase( progargs_ );
}


void uiSettingsMgr::keyPressedCB( CallBacker* )
{
    if ( !uiMain::keyboardEventHandler().hasEvent() )
	return;

    const KeyboardEvent& kbe = uiMain::keyboardEventHandler().event();
    const OD::ButtonState bs =
	OD::ButtonState( kbe.modifier_ & OD::KeyButtonMask );
    if ( bs == OD::ControlButton && kbe.key_==OD::KB_T && !kbe.isrepeat_ )
    {
	uiMain::keyboardEventHandler().setHandled( true );
	openTerminal();
    }
}


const BufferStringSet* uiSettingsMgr::programArgs( int argidx ) const
{
     if ( !progargs_.validIdx(argidx) )
	return nullptr;

     return progargs_.get( argidx );
}


uiRetVal uiSettingsMgr::openTerminal( bool withfallback, const char* cmdstr,
				      const BufferStringSet* args,
				      const char* workingdir )
{
    uiRetVal uirv;
    BufferString cmd( cmdstr );
    BufferStringSet progargs;
    if ( args )
	progargs.add( *args, false );

    if ( cmd.isEmpty() )
    {
	if ( prognms_.validIdx(termcmdidx_) )
	{
	    cmd.set( prognms_.get(termcmdidx_) );
	    if ( programArgs(termcmdidx_) )
		progargs = *progargs_.get( termcmdidx_ );
	}
	else
	{
	    const CommandDefs& cmddefs =
			CommandDefs::getTerminalCommands( BufferStringSet() );
	    if ( cmddefs.isEmpty() )
	    {
		uirv = tr("No suitable terminal emulator found on this system");
		return uirv;
	    }

	    cmd.set( cmddefs.program(0) );
	    if ( cmddefs.args(0) )
		progargs = *cmddefs.args( 0 );
	}
    }

    terminalRequested.trigger();
    BufferString errmsg;
    uiString launchermsg;
    uiRetVal ret;
    bool res = OD::PythA().openTerminal( cmd, ret, &progargs, workingdir );
    if ( !res )
    {
	uiString firstmsg = tr( "Cannot launch terminal" );
	if ( withfallback )
	{
	    res = OS::CommandLauncher::openTerminal( cmd, &progargs,
					&errmsg, &launchermsg, workingdir );
	    if ( !res && !errmsg.isEmpty() )
		firstmsg.appendPhraseSameLine( tr(": %1").arg(errmsg) );
	}
	else
	{
	    if ( !ret.isOK() )
		firstmsg.appendPhraseSameLine( tr(" withpython: %1")
						.arg(ret) );
	}

	if ( !res )
	{
	    uirv.add( firstmsg );
	    if ( !launchermsg.isEmpty() )
		uirv.add( launchermsg );
	}
    }

    return uirv;
}


void uiSettingsMgr::loadToolBarCmds( uiMainWin& applwin )
{
    if ( !usercmdtb_ )
    {
	usercmdtb_ = applwin.findToolBar( "User Commands" );
	if ( !usercmdtb_ )
	    usercmdtb_ = new uiToolBar( &applwin, tr("User Commands") );
    }

    if ( !usercmdmnu_ )
    {
	uiMenuBar* mb = applwin.menuBar();
	const uiMenu* utilmnu =
	    mb ? mb->findAction( uiStrings::sUtilities())->getMenu() : nullptr;
	const uiAction* usercmdact =
	    utilmnu ? utilmnu->findAction( tr("User Commands") ) : nullptr;
	if ( usercmdact )
	    usercmdmnu_ = const_cast<uiMenu*>( usercmdact->getMenu() );
	if ( utilmnu && !usercmdmnu_ )
	{
	    usercmdmnu_ = new uiMenu( &applwin, tr("User Commands") );
	    const_cast<uiMenu*>( utilmnu )->addMenu( usercmdmnu_,
		    utilmnu->findAction( tr("Installation"))->getMenu() );
	}
    }

    updateUserCmdToolBar();
    mAttachCBIfNotAttached( OD::PythA().envChange,
			    uiSettingsMgr::updateUserCmdToolBarCB );
}


void uiSettingsMgr::updateUserCmdToolBarCB( CallBacker* )
{
    mEnsureExecutedInMainThread( uiSettingsMgr::updateUserCmdToolBarCB );
    updateUserCmdToolBar();
}


void uiSettingsMgr::updateUserCmdToolBar()
{
    if ( !usercmdtb_ && !usercmdmnu_ )
	return;

    if ( usercmdtb_ )
	usercmdtb_->clear();
    if ( usercmdmnu_ )
	usercmdmnu_->clear();
    commands_.erase();
    prognms_.setEmpty();
    deepErase( progargs_ );
    toolbarids_.erase();
    termcmdidx_ = -1;
    idecmdidx_ = -1;

    if ( usercmdtb_ )
    {
	usercmdtb_->addButton( "python", tr("Open Python Settings dialog"),
				mCB(this,uiSettingsMgr,doPythonSettingsCB) );
    }

// Python IDE command
    BufferString pythonstr( sKey::Python() ); pythonstr.toLower();
    const IOPar& pythonsetts = Settings::fetch( pythonstr );
    const PtrMan<IOPar> idepar = pythonsetts.subselect( sKey::PythonIDE() );

    BufferString cmd, iconnm;
    uiString tooltip, uiname;
    BufferString exenm;
    if ( idepar && idepar->get(sKey::ExeName(),exenm) && !exenm.isEmpty() )
    {
	const CommandDefs commands = uiPythonSettings::getPythonIDECommands();
	const int idx = commands.indexOf( exenm );
	if ( idx != -1 )
	{
	    cmd = commands.get( idx );
	    prognms_.add( commands.program(idx) );
	    progargs_.add( commands.args(idx)
			    ? new BufferStringSet( *commands.args(idx) )
			    : nullptr);
	    uiname = commands.getUiName( idx );
	    iconnm = commands.getIconName( idx );
	    tooltip = commands.getToolTip( idx );
	}
    }
    else if ( idepar && idepar->get(sKey::Command(),cmd) && !cmd.isEmpty() &&
	      File::isExecutable(cmd) )
    {
	prognms_.add( cmd );
	progargs_.add( nullptr );
	idepar->get( sKey::IconFile(), iconnm );
	idepar->get( sKey::ToolTip(), tooltip );
	uiname = tooltip;
    }
    else
    {
	const CommandDefs commands = uiPythonSettings::getPythonIDECommands();
	if ( !commands.isEmpty() )
	{
	    const int idx = 0;
	    cmd = commands.get( idx );
	    prognms_.add( commands.program(idx) );
	    progargs_.add( commands.args(idx)
			    ? new BufferStringSet( *commands.args(idx) )
			    : nullptr);
	    uiname = commands.getUiName( idx );
	    iconnm = commands.getIconName( idx );
	    tooltip = commands.getToolTip( idx );
	}
    }

    if ( !cmd.isEmpty() )
    {
	int id = 0;
	if ( usercmdtb_ )
	    id = usercmdtb_->addButton( iconnm, tooltip,
				mCB(this,uiSettingsMgr,doToolBarCmdCB) );
	toolbarids_ += id;
	commands_.add( cmd );
	idecmdidx_ = commands_.size()-1;
	if ( usercmdmnu_ )
	{
	    auto* newitm = new uiAction(tr("Start %1").arg(uiname),
		    mCB(this,uiSettingsMgr,doToolBarCmdCB) );
	    usercmdmnu_->insertAction( newitm, id );
	}
    }

    cmd.setEmpty(); exenm.setEmpty(); iconnm.setEmpty();
    tooltip.setEmpty(); uiname.setEmpty();

// Python Terminal command
    const PtrMan<IOPar> termpar = pythonsetts.subselect( sKey::PythonTerm() );
    if ( termpar && termpar->get(sKey::ExeName(),exenm) && !exenm.isEmpty() )
    {
	const BufferStringSet termpath;
	const auto& commands = CommandDefs::getTerminalCommands( termpath );
	const int idx = commands.indexOf( exenm );
	if ( idx != -1 )
	{
	    cmd = commands.get( idx );
	    prognms_.add( commands.program(idx) );
	    progargs_.add( commands.args(idx )
			   ? new BufferStringSet( *commands.args(idx) )
			   : nullptr );
	    uiname = commands.getUiName( idx );
	    iconnm = commands.getIconName( idx );
	    tooltip = commands.getToolTip( idx );
	}
    }
    else if ( termpar && termpar->get(sKey::Command(),cmd) && !cmd.isEmpty() )
    {
	prognms_.add( cmd );
	progargs_.add( nullptr );
	termpar->get( sKey::IconFile(), iconnm );
	termpar->get( sKey::ToolTip(), tooltip );
	uiname = tooltip;
    }
    else
    {
	const BufferStringSet termpath;
	const auto& commands = CommandDefs::getTerminalCommands( termpath );
	if ( !commands.isEmpty() )
	{
	    const int idx = 0;
	    cmd = commands.get( idx );
	    prognms_.add( commands.program(idx) );
	    progargs_.add( commands.args(idx)
			    ? new BufferStringSet( *commands.args(idx) )
			    : nullptr );
	    uiname = commands.getUiName( idx );
	    iconnm = commands.getIconName( idx );
	    tooltip = commands.getToolTip( idx );
	}
    }

    if ( !cmd.isEmpty() )
    {
	int id = 0;
	if ( usercmdtb_ )
	    id = usercmdtb_->addButton( iconnm, tooltip,
				mCB(this,uiSettingsMgr,doTerminalCmdCB) );
	toolbarids_ += id;
	commands_.add( cmd );
	termcmdidx_ = prognms_.size()-1;
	if ( usercmdmnu_ )
	{
	    auto* newitm = new uiAction(tr("Start %1").arg(uiname),
		    mCB(this,uiSettingsMgr,doTerminalCmdCB) );
	    usercmdmnu_->insertAction( newitm, id );
	}
    }

    toolbarUpdated.trigger();
}


void uiSettingsMgr::doTerminalCmdCB( CallBacker* cb )
{
    mDynamicCastGet(uiAction*,action,cb);
    if ( !action )
	return;

    const int tbid = action->getID();
    int idx = toolbarids_.indexOf( tbid );
    if ( !toolbarids_.validIdx(idx) )
	return;

    BufferString prognm;
    if ( prognms_.validIdx(idx) )
	 prognm.set( prognms_.get(idx) );

    const uiRetVal uirv = openTerminal( true, prognm, programArgs(idx) );
    if ( !uirv.isOK() )
	uiMSG().error( uirv );
}


void uiSettingsMgr::doToolBarCmdCB( CallBacker* cb )
{
    mDynamicCastGet(uiAction*,action,cb);
    if ( !action )
	return;

    const int tbid = action->getID();
    int idx = toolbarids_.indexOf( tbid );
    if ( !toolbarids_.validIdx(idx) )
	return;

    OS::MachineCommand mc( commands_.get(idx) );
    if ( programArgs(idx) )
	mc.addArgs( *programArgs(idx) );

    uiRetVal uirv;
    if ( !OD::PythA().execute(mc,uirv,false) )
	uiMSG().error( tr("Error starting %1").arg(mc.toString()), uirv );
}


void uiSettingsMgr::doPythonSettingsCB( CallBacker* )
{
    uiDialog* dlg = uiSettings::getPythonDlg( uiMain::instance().topLevel(),
					      "Set Python Settings" );
    dlg->go();
}
