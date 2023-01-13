/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uisettings.h"

#include "bufstring.h"
#include "commanddefs.h"
#include "commandlaunchmgr.h"
#include "dirlist.h"
#include "envvars.h"
#include "file.h"
#include "filepath.h"
#include "keyboardevent.h"
#include "oddirs.h"
#include "oscommand.h"
#include "od_helpids.h"
#include "posimpexppars.h"
#include "ptrman.h"
#include "pythonaccess.h"
#include "separstr.h"
#include "settingsaccess.h"
#include "survinfo.h"

#include "uibutton.h"
#include "uibuttongroup.h"
#include "uicombobox.h"
#include "uigeninput.h"
#include "uifileinput.h"
#include "uilabel.h"
#include "uilistbox.h"
#include "uimain.h"
#include "uimenu.h"
#include "uimsg.h"
#include "uipathsel.h"
#include "uipixmap.h"
#include "uiseparator.h"
#include "uishortcutsmgr.h"
#include "uislider.h"
#include "uistrings.h"
#include "uitable.h"
#include "uitextedit.h"
#include "uitoolbar.h"
#include "uitoolbarcmded.h"
#include "uivirtualkeyboard.h"


static const char* sKeyCommon = "<general>";



namespace sKey {
    inline StringView PythonIDE()	{ return "PythonIDE"; }
    inline StringView PythonTerm()	{ return "PythonTerm"; }
};


mExpClass(uiTools) uiPythonSettings : public uiDialog
{ mODTextTranslationClass(uiPythonSettings);
public:
			uiPythonSettings(uiParent*, const char*);
    virtual		~uiPythonSettings();

    static CommandDefs	getPythonIDECommands();

private:
    void		initDlg(CallBacker*);
    IOPar&		curSetts();
    void		getChanges();
    void		fillPar(IOPar&) const;
    void		usePar(const IOPar&);
    bool		commitSetts(const IOPar&);
    void		sourceChgCB(CallBacker*);
    void		customEnvChgCB(CallBacker*);
    void		internalLocChgCB(CallBacker*);
    void		parChgCB(CallBacker*);
    void		setCustomEnvironmentNames();
    void		testPythonModules();
    void		testCB(CallBacker*);
    void		promptCB(CallBacker*);
    void		cloneCB(CallBacker*);
    void		cloneFinishedCB(CallBacker*);
    void		safetycheckCB(CallBacker*);
    bool		getPythonEnvBinPath(BufferString&) const;
    void		updateIDEfld();
    bool		useScreen();
    bool		rejectOK(CallBacker*) override;
    bool		acceptOK(CallBacker*) override;

    uiGenInput*		pythonsrcfld_;
    uiFileInput*	internalloc_ = nullptr;
    uiFileInput*	customloc_;
    uiGenInput*		customenvnmfld_;
    uiPathSel*		custompathfld_;
    uiToolBarCommandEditor* pyidefld_;
    uiToolBarCommandEditor* pytermfld_;
    uiButton*		clonebut_;

    IOPar*		chgdsetts_ = nullptr;
    bool		needrestore_ = false;
    IOPar		initialsetts_;
};


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
    bool res = OD::PythA().openTerminal( cmd, &progargs, workingdir );
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
	    errmsg = OD::PythA().lastOutput( true, &launchermsg );
	    if ( !errmsg.isEmpty() )
		firstmsg.appendPhraseSameLine( tr(" withpython: %1")
						.arg(errmsg) );
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

    if ( !OD::PythA().execute(mc,false) )
    {
	uiString launchermsg;
	const BufferString errmsg = OD::PythA().lastOutput(true,&launchermsg);
	uiRetVal uirv( launchermsg );
	if ( !errmsg.isEmpty() )
	    uirv.add( toUiString(errmsg) );

	uiMSG().error(tr("Error starting %1").arg(mc.toString()),
			      uirv);
	return;
    }
}


void uiSettingsMgr::doPythonSettingsCB( CallBacker* )
{
    uiDialog* dlg = uiSettings::getPythonDlg( uiMain::instance().topLevel(),
					      "Set Python Settings" );
    dlg->go();
}


static void getGrps( BufferStringSet& grps )
{
    grps.add( sKeyCommon );
    BufferString msk( "settings*" );
    const char* dtectuser = GetSoftwareUser();
    const bool needdot = dtectuser && *dtectuser;
    if ( needdot ) msk += ".*";
    BufferString pythonstr(sKey::Python());
    pythonstr.toLower();
    DirList dl( GetSettingsDir(), File::FilesInDir, msk );
    for ( int idx=0; idx<dl.size(); idx++ )
    {
	BufferString fnm( dl.get(idx) );
	char* dotptr = fnm.find( '.' );
	if ( (needdot && !dotptr) || (!needdot && dotptr) )
	    continue;
	if ( dotptr )
	{
	    BufferString usr( dotptr + 1 );
	    if ( usr != dtectuser )
		continue;
	    *dotptr = '\0';
	}
	const char* underscoreptr = firstOcc( fnm.buf(), '_' );
	if ( !underscoreptr || !*underscoreptr )
	    continue;
	underscoreptr += 1;
	if ( StringView(underscoreptr).contains(pythonstr.buf()) )
	    continue;
	grps.add( underscoreptr );
    }
}


uiSettings::uiSettings( uiParent* p, const char* nm, const char* settskey )
	: uiDialog(p,uiDialog::Setup(mToUiStringTodo(nm),
				     tr("Set User Settings value"),
				     mODHelpKey(mSettingsHelpID)) )
	, issurvdefs_(StringView(settskey)==sKeySurveyDefs())
	, grpfld_(0)
{
    setCurSetts();
    if ( issurvdefs_ )
    {
	setTitleText( tr("Set Survey default value") );
	setHelpKey( mODHelpKey(mSurveySettingsHelpID) );
    }
    else
    {
	BufferStringSet grps; getGrps( grps );
	grpfld_ = new uiGenInput( this, tr("Settings group"),
				  StringListInpSpec(grps) );
	mAttachCB( grpfld_->valuechanged, uiSettings::grpChg );
    }

    tbl_ = new uiTable( this, uiTable::Setup(10,2).manualresize(true),
				"Settings editor" );
    tbl_->setColumnLabel( 0, tr("Keyword") );
    tbl_->setColumnLabel( 1, uiStrings::sValue() );
    // tbl_->setColumnResizeMode( uiTable::Interactive );
    tbl_->setStretch( 2, 2 );
    tbl_->setPrefWidth( 400 );
    tbl_->setPrefHeight( 300 );
    if ( grpfld_ )
	tbl_->attach( ensureBelow, grpfld_ );

    mAttachCB( postFinalize(), uiSettings::dispNewGrp );
}


uiSettings::~uiSettings()
{
    detachAllNotifiers();
    deepErase( chgdsetts_ );
}


int uiSettings::getChgdSettIdx( const char* nm ) const
{
    for ( int idx=0; idx<chgdsetts_.size(); idx++ )
    {
	if ( chgdsetts_[idx]->name() == nm )
	    return idx;
    }
    return -1;
}


const IOPar& uiSettings::orgPar() const
{
    if ( issurvdefs_ )
	return SI().getPars();

    const BufferString grp( grpfld_ ? grpfld_->text() : sKeyCommon );
    return grp == sKeyCommon ? Settings::common() : Settings::fetch( grp );
}


void uiSettings::setCurSetts()
{
    const IOPar* iop = &orgPar();
    if ( !issurvdefs_ )
    {
	const int chgdidx = getChgdSettIdx( iop->name() );
	if ( chgdidx >= 0 )
	    iop = chgdsetts_[chgdidx];
    }
    cursetts_ = iop;
}


void uiSettings::getChanges()
{
    IOPar* workpar = 0;
    const int chgdidx = getChgdSettIdx( cursetts_->name() );
    if ( chgdidx >= 0 )
	workpar = chgdsetts_[chgdidx];
    const bool alreadyinset = workpar;
    if ( alreadyinset )
	workpar->setEmpty();
    else
	workpar = new IOPar( cursetts_->name() );

    const int sz = tbl_->nrRows();
    for ( int irow=0; irow<sz; irow++ )
    {
	BufferString kybuf = tbl_->text( RowCol(irow,0) );
	kybuf.trimBlanks();
	if ( kybuf.isEmpty() )
	    continue;
	BufferString valbuf = tbl_->text( RowCol(irow,1) );
	valbuf.trimBlanks();
	if ( valbuf.isEmpty() )
	    continue;
	workpar->set( kybuf, valbuf );
    }

    if ( !orgPar().isEqual(*workpar) )
    {
	if ( !alreadyinset )
	    chgdsetts_ += workpar;
    }
    else
    {
	if ( alreadyinset )
	    chgdsetts_ -= workpar;
	delete workpar;
    }
}


bool uiSettings::commitSetts( const IOPar& iop )
{
    Settings& setts = Settings::fetch( iop.name() );
    setts.IOPar::operator =( iop );
    if ( !setts.write(false) )
    {
	uiMSG().error( tr("Cannot write %1").arg(setts.name()) );
	return false;
    }
    return true;
}


void uiSettings::grpChg( CallBacker* )
{
    getChanges();
    setCurSetts();
    dispNewGrp( 0 );
}


void uiSettings::dispNewGrp( CallBacker* )
{
    BufferStringSet keys, vals;
    IOParIterator iter( *cursetts_ );
    BufferString key, val;
    while ( iter.next(key,val) )
    {
	keys.add( key );
	vals.add( val );
    }

    ArrPtrMan<int> idxs = keys.getSortIndexes();
    keys.useIndexes(idxs); vals.useIndexes(idxs);

    const int sz = keys.size();
    tbl_->clearTable();
    tbl_->setNrRows( sz + 5 );
    for ( int irow=0; irow<sz; irow++ )
    {
	tbl_->setText( RowCol(irow,0), keys.get(irow) );
	tbl_->setText( RowCol(irow,1), vals.get(irow) );
    }

    tbl_->resizeColumnToContents( 1 );
    tbl_->resizeColumnToContents( 2 );
}


bool uiSettings::acceptOK( CallBacker* )
{
    getChanges();
    if ( chgdsetts_.isEmpty() )
	return true;

    if ( issurvdefs_ )
    {
	SI().getPars() = *chgdsetts_[0];
	SI().savePars();
	PosImpExpPars::refresh();
    }
    else
    {
	for ( int idx=0; idx<chgdsetts_.size(); idx++ )
	{
	    IOPar* iop = chgdsetts_[idx];
	    if ( commitSetts(*iop) )
		{ chgdsetts_.removeSingle( idx ); delete iop; idx--; }
	}
	if ( !chgdsetts_.isEmpty() )
	    return false;
    }

    return true;
}


static int theiconsz = -1;


mImplFactory2Param( uiSettingsGroup, uiParent*, Settings&,
		    uiSettingsGroup::factory )

uiSettingsGroup::uiSettingsGroup( uiParent* p, const uiString& caption,
				  Settings& setts )
    : uiDlgGroup(p,caption)
    , setts_(setts)
    , changed_(false)
    , needsrestart_(false)
    , needsrenewal_(false)
{
}


uiSettingsGroup::~uiSettingsGroup()
{}


const char* uiSettingsGroup::errMsg() const
{ return errmsg_.buf(); }


#define mUpdateSettings( type, setfunc ) \
void uiSettingsGroup::updateSettings( type oldval, type newval, \
				      const char* key ) \
{ \
    if ( oldval != newval ) \
    { \
	changed_ = true; \
	setts_.setfunc( key, newval ); \
    } \
}

mUpdateSettings( bool, setYN )
mUpdateSettings( int, set )
mUpdateSettings( float, set )
mUpdateSettings( const OD::String&, set )



// uiGeneralSettingsGroup
uiGeneralSettingsGroup::uiGeneralSettingsGroup( uiParent* p, Settings& setts )
    : uiSettingsGroup(p,tr("General"),setts)
    , iconsz_(theiconsz < 0 ? uiObject::iconSize() : theiconsz)
    , showinlprogress_(true)
    , showcrlprogress_(true)
    , showrdlprogress_(true)
    , enabvirtualkeyboard_(false)
{
    iconszfld_ = new uiGenInput( this, tr("Icon Size"),
				 IntInpSpec(iconsz_,10,64) );

    setts_.getYN( uiVirtualKeyboard::sKeyEnabVirtualKeyboard(),
		  enabvirtualkeyboard_ );
    virtualkeyboardfld_ = new uiGenInput( this,
		tr("Enable Virtual Keyboard"),
		BoolInpSpec(enabvirtualkeyboard_) );
    virtualkeyboardfld_->attach( alignedBelow, iconszfld_ );

    uiLabel* lbl = new uiLabel( this,
	tr("Show progress when loading stored data on:") );
    lbl->attach( leftAlignedBelow, virtualkeyboardfld_ );

    setts_.getYN( SettingsAccess::sKeyShowInlProgress(), showinlprogress_ );
    showinlprogressfld_ = new uiGenInput( this, uiStrings::sInline(mPlural),
					  BoolInpSpec(showinlprogress_) );
    showinlprogressfld_->attach( alignedBelow, virtualkeyboardfld_ );
    showinlprogressfld_->attach( ensureBelow, lbl );

    setts_.getYN( SettingsAccess::sKeyShowCrlProgress(), showcrlprogress_ );
    showcrlprogressfld_ = new uiGenInput( this, uiStrings::sCrossline(mPlural),
					  BoolInpSpec(showcrlprogress_) );
    showcrlprogressfld_->attach( alignedBelow, showinlprogressfld_ );

    setts_.getYN( SettingsAccess::sKeyShowRdlProgress(), showrdlprogress_ );
    showrdlprogressfld_ = new uiGenInput( this, uiStrings::sRandomLine(mPlural),
					  BoolInpSpec(showrdlprogress_) );
    showrdlprogressfld_->attach( alignedBelow, showcrlprogressfld_ );
}


uiGeneralSettingsGroup::~uiGeneralSettingsGroup()
{}


bool uiGeneralSettingsGroup::acceptOK()
{
    const int newiconsz = iconszfld_->getIntValue();
    if ( newiconsz < 10 || newiconsz > 64 )
    {
	errmsg_.set( "Please specify an icon size in the range 10-64" );
	return false;
    }

    if ( newiconsz != iconsz_ )
    {
	IOPar* iopar = setts_.subselect( SettingsAccess::sKeyIcons() );
	if ( !iopar ) iopar = new IOPar;
	iopar->set( "size", newiconsz );
	setts_.mergeComp( *iopar, SettingsAccess::sKeyIcons() );
	changed_ = true;
	needsrestart_ = true;
	delete iopar;
	theiconsz = newiconsz;
    }

    updateSettings( showinlprogress_, showinlprogressfld_->getBoolValue(),
		    SettingsAccess::sKeyShowInlProgress() );
    updateSettings( showcrlprogress_, showcrlprogressfld_->getBoolValue(),
		    SettingsAccess::sKeyShowCrlProgress() );
    updateSettings( showrdlprogress_, showrdlprogressfld_->getBoolValue(),
		    SettingsAccess::sKeyShowRdlProgress() );
    updateSettings( enabvirtualkeyboard_, virtualkeyboardfld_->getBoolValue(),
		    uiVirtualKeyboard::sKeyEnabVirtualKeyboard() );

    return true;
}


class uiMaterialGroup : public uiGroup
{ mODTextTranslationClass(uiMaterialGroup)
public:
uiMaterialGroup( uiParent* p, Settings& setts )
    : uiGroup(p,"Material group")
    , setts_(setts)
{
    uiSlider::Setup ss(tr("Default Ambient reflectivity")); ss.withedit(true);
    ambslider_ = new uiSlider( this, ss, "Ambient slider" );

    ss.lbl_ = tr("Default Diffuse reflectivity");
    diffslider_ = new uiSlider( this, ss, "Diffuse slider" );
    diffslider_->attach( alignedBelow, ambslider_ );

    setts.get( SettingsAccess::sKeyDefaultAmbientReflectivity(), ambval_ );
    ambslider_->setValue( ambval_*100 );

    setts.get( SettingsAccess::sKeyDefaultDiffuseReflectivity(), diffval_ );
    diffslider_->setValue( diffval_*100 );

    setHAlignObj( ambslider_ );
}


void updateSettings( float oldval, float newval, const char* key,
		     bool& changed )
{
    if ( !mIsEqual(oldval,newval,1e-3) )
    {
	changed = true;
	setts_.set( key, newval );
    }
}


void updateSettings( bool& changed )
{
    updateSettings( ambval_, ambslider_->getFValue()/100.f,
		SettingsAccess::sKeyDefaultAmbientReflectivity(), changed );
    updateSettings( diffval_, diffslider_->getFValue()/100.f,
		SettingsAccess::sKeyDefaultDiffuseReflectivity(), changed );
}


uiSlider*	ambslider_;
uiSlider*	diffslider_;
float		ambval_		= 0.8f;
float		diffval_	= 0.8f;

Settings&	setts_;

};



// uiVisSettingsGroup
uiVisSettingsGroup::uiVisSettingsGroup( uiParent* p, Settings& setts )
    : uiSettingsGroup(p,tr("Visualization"),setts)
    , textureresindex_(0)
    , usesurfshaders_(true)
    , usevolshaders_(true)
    , enablemipmapping_(true)
    , anisotropicpower_(4)
{
    uiLabel* shadinglbl = new uiLabel( this,
				tr("Use OpenGL shading when available:") );
    setts_.getYN( SettingsAccess::sKeyUseSurfShaders(), usesurfshaders_ );
    usesurfshadersfld_ = new uiGenInput( this, tr("for surface rendering"),
					 BoolInpSpec(usesurfshaders_) );
    usesurfshadersfld_->attach( ensureBelow, shadinglbl );

    setts_.getYN( SettingsAccess::sKeyUseVolShaders(), usevolshaders_ );
    usevolshadersfld_ = new uiGenInput( this, tr("for volume rendering"),
					BoolInpSpec(usevolshaders_) );
    usevolshadersfld_->attach( alignedBelow, usesurfshadersfld_, 0 );

    uiLabeledComboBox* lcb =
	new uiLabeledComboBox( this, tr("Default texture resolution") );
    lcb->attach( alignedBelow, usevolshadersfld_ );
    textureresfactorfld_ = lcb->box();
    textureresfactorfld_->addItem( tr("Standard") );
    textureresfactorfld_->addItem( tr("Higher") );
    textureresfactorfld_->addItem( tr("Highest") );

    if ( SettingsAccess::systemHasDefaultTexResFactor() )
	textureresfactorfld_->addItem( tr("System default") );

    textureresindex_ = SettingsAccess(setts_).getDefaultTexResAsIndex( 3 );
    textureresfactorfld_->setCurrentItem( textureresindex_ );

    setts_.getYN( SettingsAccess::sKeyEnableMipmapping(), enablemipmapping_ );
    enablemipmappingfld_ = new uiGenInput( this, tr("Mipmap anti-aliasing"),
					   BoolInpSpec(enablemipmapping_) );
    enablemipmappingfld_->attach( alignedBelow, lcb );
    enablemipmappingfld_->valuechanged.notify(
			    mCB(this,uiVisSettingsGroup,mipmappingToggled) );

    anisotropicpowerfld_= new uiLabeledComboBox( this,
					    tr("Sharpen oblique textures") );
    anisotropicpowerfld_->attach( alignedBelow, enablemipmappingfld_ );
    anisotropicpowerfld_->box()->addItem( toUiString(" 0 x") );
    anisotropicpowerfld_->box()->addItem( toUiString(" 1 x") );
    anisotropicpowerfld_->box()->addItem( toUiString(" 2 x") );
    anisotropicpowerfld_->box()->addItem( toUiString(" 4 x") );
    anisotropicpowerfld_->box()->addItem( toUiString(" 8 x") );
    anisotropicpowerfld_->box()->addItem( toUiString("16 x") );
    anisotropicpowerfld_->box()->addItem( toUiString("32 x") );

    setts_.get( SettingsAccess::sKeyAnisotropicPower(), anisotropicpower_ );
    if ( anisotropicpower_ < -1 )
	anisotropicpower_ = -1;
    if ( anisotropicpower_ > 5 )
	anisotropicpower_ = 5;

    anisotropicpowerfld_->box()->setCurrentItem( anisotropicpower_+1 );

    matgrp_ = new uiMaterialGroup( this, setts_ );
    matgrp_->attach( alignedBelow, anisotropicpowerfld_ );

    mipmappingToggled( nullptr );
}


uiVisSettingsGroup::~uiVisSettingsGroup()
{
}


void uiVisSettingsGroup::mipmappingToggled( CallBacker* )
{
    anisotropicpowerfld_->setSensitive( enablemipmappingfld_->getBoolValue() );
}


bool uiVisSettingsGroup::acceptOK()
{
    const bool usesurfshaders = usesurfshadersfld_->getBoolValue();
    updateSettings( usesurfshaders_, usesurfshaders,
		    SettingsAccess::sKeyUseSurfShaders() );
    const bool usevolshaders = usevolshadersfld_->getBoolValue();
    updateSettings( usevolshaders_, usevolshaders,
		    SettingsAccess::sKeyUseVolShaders() );

    const int idx = textureresfactorfld_->currentItem();
    if ( textureresindex_ != idx )
    {
	changed_ = true;
	SettingsAccess(setts_).setDefaultTexResAsIndex( idx, 3 );
    }

    updateSettings( enablemipmapping_, enablemipmappingfld_->getBoolValue(),
		    SettingsAccess::sKeyEnableMipmapping() );

    const int anisotropicpower = anisotropicpowerfld_->box()->currentItem()-1;
    updateSettings( anisotropicpower_, anisotropicpower,
		    SettingsAccess::sKeyAnisotropicPower() );

    matgrp_->updateSettings( changed_ );

    if ( changed_ )
	needsrenewal_ = true;

    return true;
}


// uiSettingsDlg
uiSettingsDlg::uiSettingsDlg( uiParent* p )
    : uiTabStackDlg(p,uiDialog::Setup(tr("OpendTect Settings"),mNoDlgTitle,
				      mODHelpKey(mLooknFeelSettingsHelpID)))
    , setts_(Settings::common())
    , changed_(false)
    , needsrestart_(false)
    , needsrenewal_(false)
{
    const BufferStringSet& nms = uiSettingsGroup::factory().getNames();
    for ( int idx=0; idx<nms.size(); idx++ )
    {
	uiSettingsGroup* grp =
		uiSettingsGroup::factory().create( nms.get(idx),
						   tabstack_->tabGroup(),
						   setts_ );
	grp->attach( hCentered );
	addGroup( grp );
	grps_ += grp;
    }
}


uiSettingsDlg::~uiSettingsDlg()
{
}


bool uiSettingsDlg::acceptOK( CallBacker* cb )
{
    if ( !uiTabStackDlg::acceptOK(cb) )
	return false;

    changed_ = false;
    for ( int idx=0; idx<grps_.size(); idx++ )
	changed_ = changed_ || grps_[idx]->isChanged();

    if ( changed_ && !setts_.write() )
    {
	uiMSG().error( uiStrings::sCantWriteSettings() );
	return false;
    }

    needsrestart_ = false; needsrenewal_ = false;
    for ( int idx=0; idx<grps_.size(); idx++ )
    {
	needsrestart_ = needsrestart_ || grps_[idx]->needsRestart();
	needsrenewal_ = needsrenewal_ || grps_[idx]->needsRenewal();
    }

    if ( needsrestart_ )
	uiMSG().message(tr("Your new settings will become active\n"
			   "the next time OpendTect is started."));
    else if ( needsrenewal_ )
	uiMSG().message(tr("Your new settings will become active\n"
			   "only for newly launched objects."));

    return true;
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

    mAttachCB( pythonsrcfld_->valuechanged, uiPythonSettings::sourceChgCB );
    if ( internalloc_ )
	mAttachCB( internalloc_->valuechanged,
		   uiPythonSettings::internalLocChgCB );
    mAttachCB( customloc_->valuechanged, uiPythonSettings::customEnvChgCB );
    mAttachCB( customenvnmfld_->valuechanged, uiPythonSettings::parChgCB );
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

    updateIDEfld();
    PtrMan<IOPar> idepar = par.subselect( sKey::PythonIDE() );
    if ( idepar )
	pyidefld_->usePar( *idepar );
    pyidefld_->setChecked( idepar );

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
    const uiRetVal uirv( OD::PythA().getModules(modules) );
    if ( !uirv.isOK() )
    {
	uiMSG().error( uirv );
	return;
    }

    BufferStringSet modstrs;
    for ( auto module : modules )
	modstrs.add( module->displayStr() );

    usw.readyNow();
    uiDialog dlg( this, uiDialog::Setup(tr("Python Installation"),mNoDlgTitle,
					mNoHelpKey) );
    dlg.setCtrlStyle( uiDialog::CloseOnly );
    uiGenInput* pythfld = new uiGenInput( &dlg, tr("Using") );
    pythfld->setText( OD::PythA().pyVersion() );
    pythfld->setReadOnly();
    pythfld->setElemSzPol( uiObject::Wide );
    uiLabeledListBox* modfld =
	new uiLabeledListBox( &dlg, tr("Detected modules") );
    modfld->addItems( modstrs );
    modfld->attach( alignedBelow, pythfld );
    dlg.go();
}

void uiPythonSettings::testCB(CallBacker*)
{
    if ( !useScreen() )
	return;

    uiUserShowWait usw( this, tr("Retrieving Python testing") );
    OD::PythA().istested_ = false;
    if ( !OD::PythA().retrievePythonVersionStr() )
    {
	uiString launchermsg;
	uiRetVal uirv( tr("Cannot detect Python version:\n%1\n")
		.arg(OD::PythA().lastOutput(true,&launchermsg)) );
	uirv.add( tr("Python environment not usable") )
	    .add( launchermsg );
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

    OS::MachineCommand mc;
    OD::PythA().setForScript( "safety", mc );
    mc.addArg( "check" );
    BufferString stdoutstr;
    uiString errmsg;
    uiUserShowWait usw( this, tr("Checking Python environment") );
    if ( !OD::PythA().execute(mc,stdoutstr,nullptr,&errmsg) ||
	 stdoutstr.isEmpty() )
    {
	uiMSG().error( errmsg );
	return;
    }

    uiDialog dlg( this, uiDialog::Setup(tr("Safety Check"), mNoDlgTitle,
					mNoHelpKey) );
    auto* browser = new uiTextBrowser( &dlg );
    browser->setPrefWidthInChar( 100 );
    browser->setText( stdoutstr.buf() );
    dlg.setCancelText( uiString::empty() );
    usw.readyNow();
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
	deleteAndNullPtr(chgdsetts_);

    if ( chgdsetts_ )
	return false;

    OD::PythA().istested_ = false;
    OD::PythA().updatePythonPath();
    uiSettsMgr().updateUserCmdToolBar();

    return true;
}

bool uiPythonSettings::rejectOK( CallBacker* )
{
    if ( !needrestore_ )
	return true;

    if ( commitSetts(initialsetts_) )
    {
	OD::PythA().istested_ = false;
	OD::PythA().envChangeCB( nullptr );
	OD::PythA().updatePythonPath();
	uiSettsMgr().updateUserCmdToolBar();
    }
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
	{
	    NotifyStopper ns( OD::PythA().envChange );
	    OD::PythA().istested_ = false;
	    OD::PythA().envChangeCB( nullptr );
	    OD::PythA().updatePythonPath();
	    ns.enableNotification();
	    OD::PythA().envChange.trigger();
	    needrestore_ = false;

	    uiSettsMgr().updateUserCmdToolBar();
	}
    }
    else
	uiMSG().warning( tr("Cannot use the new settings") );

    return isok;
}


CommandDefs uiPythonSettings::getPythonIDECommands()
{
    BufferStringSet paths;
    FilePath pybinpath;
    OD::PythonAccess::GetPythonEnvBinPath( pybinpath );
    paths.add( pybinpath.fullPath() );

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
