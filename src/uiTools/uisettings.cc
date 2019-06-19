/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:          November 2001
________________________________________________________________________

-*/

#include "uisettings.h"

#include "bufstring.h"
#include "dirlist.h"
#include "envvars.h"
#include "filepath.h"
#include "genc.h"
#include "oddirs.h"
#include "od_helpids.h"
#include "oscommand.h"
#include "odviscommon.h"
#include "posimpexppars.h"
#include "ptrman.h"
#include "pythonaccess.h"
#include "separstr.h"
#include "settingsaccess.h"
#include "survinfo.h"

#include "uibutton.h"
#include "uibuttongroup.h"
#include "uichecklist.h"
#include "uicombobox.h"
#include "uigeninput.h"
#include "uifilesel.h"
#include "uiiconsetsel.h"
#include "uilabel.h"
#include "uimsg.h"
#include "uiseparator.h"
#include "uistrings.h"
#include "uisplitter.h"
#include "uitable.h"
#include "uithemesel.h"
#include "uitreeview.h"


static const char* sKeyCommon = "<general>";


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
	if ( FixedString(underscoreptr).contains(pythonstr) )
	    continue;
	grps.add( underscoreptr + 1 );
    }
}


uiAdvSettings::uiAdvSettings( uiParent* p, const uiString& titl,
			    const char* settskey )
	: uiDialog(p,uiDialog::Setup(titl,
			tr("Browse/Edit User-settable properties and values"),
			mODHelpKey(mSettingsHelpID)) )
        , issurvdefs_(FixedString(settskey)==sKeySurveyDefs())
	, grpfld_(0)
	, sipars_(SI().getDefaultPars())
{
    setCurSetts();
    if ( issurvdefs_ )
	setHelpKey( mODHelpKey(mSurveySettingsHelpID) );
    else
    {
	BufferStringSet grps; getGrps( grps );
	grpfld_ = new uiGenInput( this, tr("Settings Group"),
				  StringListInpSpec(grps) );
	mAttachCB( grpfld_->valuechanged, uiAdvSettings::grpChg );
    }

    tbl_ = new uiTable( this, uiTable::Setup(10,2).manualresize(true),
				"Settings editor" );
    tbl_->setColumnLabel( 0, uiStrings::sKeyword() );
    tbl_->setColumnLabel( 1, uiStrings::sValue() );
    tbl_->setStretch( 2, 2 );
    tbl_->setPrefWidth( 400 );
    tbl_->setPrefHeight( 300 );
    if ( grpfld_ )
	tbl_->attach( ensureBelow, grpfld_ );

    mAttachCB( postFinalise(), uiAdvSettings::dispNewGrp );
}


uiAdvSettings::~uiAdvSettings()
{
    detachAllNotifiers();
    deepErase( chgdsetts_ );
}


int uiAdvSettings::getChgdSettIdx( const char* nm ) const
{
    for ( int idx=0; idx<chgdsetts_.size(); idx++ )
    {
	if ( chgdsetts_[idx]->hasName(nm) )
	    return idx;
    }
    return -1;
}


const IOPar& uiAdvSettings::orgPar() const
{
    if ( issurvdefs_ )
	return sipars_;

    const BufferString grp( grpfld_ ? grpfld_->text() : sKeyCommon );
    return grp == sKeyCommon ? Settings::common() : Settings::fetch( grp );
}


void uiAdvSettings::setCurSetts()
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


void uiAdvSettings::getChanges()
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

    if ( !orgPar().isEqual(*workpar,true) )
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


bool uiAdvSettings::commitSetts( const IOPar& iop )
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


void uiAdvSettings::grpChg( CallBacker* )
{
    getChanges();
    setCurSetts();
    dispNewGrp( 0 );
}


void uiAdvSettings::dispNewGrp( CallBacker* )
{
    IOPar disiop( *cursetts_ );
    disiop.sortOnKeys();
    const int sz = disiop.size();

    tbl_->clearTable();
    tbl_->setNrRows( sz + 5 );
    for ( int irow=0; irow<sz; irow++ )
    {
	tbl_->setText( RowCol(irow,0), disiop.getKey(irow) );
	tbl_->setText( RowCol(irow,1), disiop.getValue(irow) );
    }

    tbl_->resizeColumnToContents( 1 );
    tbl_->resizeColumnToContents( 2 );
}


bool uiAdvSettings::acceptOK()
{
    getChanges();
    if ( chgdsetts_.isEmpty() )
	return true;

    if ( issurvdefs_ )
    {
	SI().setDefaultPars( *chgdsetts_[0], true );
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


mExpClass(uiTools) uiPythonSettings : public uiDialog
{ mODTextTranslationClass(uiPythonSettings);
public:

uiPythonSettings(uiParent* p, const char* nm)
	: uiDialog(p, uiDialog::Setup(mToUiStringTodo(nm),
		tr("Set Python default environment"),mTODOHelpKey))
{
    pythonsrcfld_ = new uiGenInput(this, tr("Python Environment"),
		StringListInpSpec(OD::PythonSourceDef().strings()));
    mAttachCB( pythonsrcfld_->valuechanged, uiPythonSettings::sourceChgCB );

    if ( !OD::PythonAccess::hasInternalEnvironment(false) )
    {
	internalloc_ = new uiFileSel( this, tr("Environment root") );
	internalloc_->setup().withexamine(false);
	internalloc_->setSelectionMode( OD::SelectDirectory );
	internalloc_->attach( alignedBelow, pythonsrcfld_ );
	mAttachCB( internalloc_->newSelection, uiPythonSettings::parChgCB );
    }

    customloc_ = new uiFileSel( this,tr("Custom Environment root"));
    customloc_->setup().withexamine(false);
    customloc_->setSelectionMode( OD::SelectDirectory );
    customloc_->attach( alignedBelow, pythonsrcfld_ );
    mAttachCB( customloc_->newSelection, uiPythonSettings::customEnvChgCB );

    customenvnmfld_ = new uiGenInput( this, tr("Environment"),
				      StringListInpSpec() );
    customenvnmfld_->attach( alignedBelow, customloc_ );
    mAttachCB( customenvnmfld_->valuechanged, uiPythonSettings::parChgCB );

    uiButton* testbut = new uiPushButton( this, tr("Test"),
			mCB(this,uiPythonSettings,testCB), true);
    testbut->setIcon( "test" );
    testbut->attach( ensureBelow, customenvnmfld_ );

    uiButton* cmdwinbut = new uiPushButton( this, tr("Launch Prompt"),
			mCB(this,uiPythonSettings,promptCB), true );
    cmdwinbut->setIcon( "launch" );
    cmdwinbut->attach( rightOf, testbut );

    mAttachCB( postFinalise(), uiPythonSettings::initDlg );
}

virtual ~uiPythonSettings()
{
}

private:

void initDlg(CallBacker*)
{
    usePar( curSetts() );
    fillPar( initialsetts_ ); //Backup for restore
    sourceChgCB(0);
}

IOPar& curSetts()
{
    BufferString pythonstr( sKey::Python() );
    return Settings::fetch( pythonstr.toLower() );
}

void getChanges()
{
    IOPar* workpar = 0;
    if ( chgdsetts_ )
	workpar = chgdsetts_;
    const bool alreadyedited = workpar;
    if ( alreadyedited )
	workpar->setEmpty();
    else
	workpar = new IOPar;

    fillPar( *workpar );
    if ( !curSetts().isEqual(*workpar,true) )
    {
	if ( !alreadyedited )
	    chgdsetts_ = workpar;
    }
    else
    {
	if ( alreadyedited )
	    chgdsetts_ = 0;
	delete workpar;
    }
}

void fillPar( IOPar& par ) const
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
	    par.set( sKey::Name(), customenvnmfld_->text() );
	}
    }
}

void usePar( const IOPar& par )
{
    OD::PythonSource source;
    if ( !OD::PythonSourceDef().parse(par,
		OD::PythonAccess::sKeyPythonSrc(), source) )
    {
	source = OD::PythonAccess::hasInternalEnvironment(false)
	       ? OD::Internal : OD::System;
    }

    pythonsrcfld_->setValue( source );
    if ( source == OD::Internal && internalloc_ )
    {
	BufferString envroot;
	if ( par.get(OD::PythonAccess::sKeyEnviron(),envroot) )
	    internalloc_->setFileName( envroot );
    }
    else if ( source == OD::Custom )
    {
	BufferString envroot;
	if ( par.get(OD::PythonAccess::sKeyEnviron(),envroot) )
	    customloc_->setFileName( envroot );
	if ( par.get(sKey::Name(),envroot) )
	    customenvnmfld_->setText( envroot );
    }
}

bool commitSetts( const IOPar& iop )
{
    Settings& setts = Settings::fetch( iop.name() );
    setts.IOPar::operator =(iop);
    if ( !setts.write(false) )
    {
	uiMSG().error(tr("Cannot write %1").arg(setts.name()));
	return false;
    }

    return true;
}

void sourceChgCB( CallBacker* )
{
    const int sourceidx = pythonsrcfld_->getIntValue();
    const OD::PythonSource source =
		OD::PythonSourceDef().getEnumForIndex(sourceidx);

    if ( internalloc_ )
	internalloc_->display( source == OD::Internal );

    customloc_->display( source == OD::Custom );
    customenvnmfld_->display( source == OD::Custom );
    parChgCB( nullptr );
}

void customEnvChgCB( CallBacker* )
{
    setCustomEnvironmentNames();
    parChgCB( nullptr );
}

void parChgCB( CallBacker* )
{
    getChanges();
}


void setCustomEnvironmentNames()
{
    const BufferString envroot( customloc_->fileName() );
    const File::Path fp( envroot, "envs" );
    if ( !fp.exists() )
	return;

    BufferStringSet envnames;
    const DirList dl( fp.fullPath(), File::DirsInDir );
    for ( int idx=0; idx<dl.size(); idx++ )
	envnames.add( File::Path(dl.fullPath(idx)).baseName() );
    customenvnmfld_->setEmpty();
    customenvnmfld_->newSpec( StringListInpSpec(envnames), 0 );
}

void testPythonModules()
{
    ManagedObjectSet<OD::PythonAccess::ModuleInfo> modules;
    const uiRetVal uirv( OD::PythA().getModules(modules) );
    if ( !uirv.isOK() )
    {
	gUiMsg( this ).error( uirv );
	return;
    }

    BufferStringSet modstrs;
    for ( auto module : modules )
	modstrs.add( module->displayStr() );

    gUiMsg( this ).message( tr("Detected list of Python modules:\n%1")
				.arg( modstrs.cat()) );
}

void testCB(CallBacker*)
{
    needrestore_ = chgdsetts_;
    if ( !useScreen() )
	return;

    uiUserShowWait usw( this, tr("Retrieving Python testing") );
    if ( !OD::PythA().isUsable(true) )
    {
	uiString launchermsg;
	uiRetVal uirv( tr("Cannot detect python version:\n%1")
			.arg(OD::PythA().lastOutput(true,&launchermsg)) );
	uirv.add( tr("Python environment not usable") )
	    .add( launchermsg );
	gUiMsg( this ).error( uirv );
	return;
    }

    BufferString versionstr( OD::PythA().lastOutput(false,nullptr) );
    if ( versionstr.isEmpty() )
	versionstr.set( OD::PythA().lastOutput(true,nullptr) );
    gUiMsg( this ).message( tr("Detected Python version: %1").arg(versionstr));

    usw.setMessage( tr("Retrieving list of installed Python modules") );
    testPythonModules();
}

void promptCB( CallBacker* )
{
    const BufferString termem = SettingsAccess().getTerminalEmulator();
#ifdef __win__
    OS::MachineCommand cmd( "start" );
    cmd.addArg( termem );
#else
    OS::MachineCommand cmd( termem );
#endif
    OD::PythA().execute( cmd, false );
}

bool useScreen()
{
    const int sourceidx = pythonsrcfld_->getIntValue();
    const OD::PythonSource source =
		OD::PythonSourceDef().getEnumForIndex(sourceidx);

    uiString envrootstr = tr("Evironment root" );
    if ( source == OD::Internal && internalloc_ )
    {
	const BufferString envroot( internalloc_->fileName() );
	if ( !File::exists(envroot) || !File::isDirectory(envroot) )
	{
	    gUiMsg(this).error( uiStrings::phrSelect(envrootstr) );
	    return false;
	}

	const File::Path envrootfp( envroot );
	if ( !OD::PythonAccess::validInternalEnvironment(envrootfp) )
	{
	    gUiMsg(this).error( tr("Invalid %1").arg(envrootstr) );
	    return false;
	}
    }
    else if ( source == OD::Custom )
    {
	const BufferString envroot( customloc_->fileName() );
	if ( !File::exists(envroot) || !File::isDirectory(envroot) )
	{
	    gUiMsg(this).error( uiStrings::phrSelect(envrootstr) );
	    return false;
	}

	const File::Path envrootfp( envroot, "envs" );
	if ( !envrootfp.exists() )
	{
	    gUiMsg(this).error( tr("%1 does not contain a folder called %2")
				.arg(envroot).arg("'envs'") );
	    return false;
	}
    }

    if ( !chgdsetts_ )
	return true;

    if ( commitSetts(*chgdsetts_) )
	deleteAndZeroPtr(chgdsetts_);

    if ( chgdsetts_ )
	return false;

    return true;
}

bool rejectOK()
{
    return needrestore_ ? commitSetts( initialsetts_ ) : true;
}

bool acceptOK()
{
    needrestore_ = false;
    return useScreen();
}

    uiGenInput*		pythonsrcfld_;
    uiFileSel*		internalloc_ = nullptr;
    uiFileSel*		customloc_;
    uiGenInput*		customenvnmfld_;

    IOPar*		chgdsetts_ = nullptr;
    bool		needrestore_ = false;
    IOPar		initialsetts_;

};



uiDialog* uiAdvSettings::getPythonDlg( uiParent* p )
{
    uiDialog* ret = new uiPythonSettings( p, "Set Python Settings" );
    ret->setModal( false );
    ret->setDeleteOnClose( true );
    return ret;
}



static int thetbsz_ = -1;
mImplClassFactory( uiSettingsGroup, factory )


uiSettsGrp::uiSettsGrp( uiParent* p, Settings& setts,
					const char* nm )
    : uiGroup(p,nm)
    , setts_(setts)
{
}


uiSettingsSubGroup::uiSettingsSubGroup( uiSettingsGroup& p )
    : uiSettsGrp(&p,p.setts_,"Settings subgroup")
{
    uiGroup* botgrp = p.lastGroup();
    if ( botgrp )
	attach( alignedBelow, botgrp );
}


void uiSettingsSubGroup::addToParent( uiSettingsGroup& p )
{
    p.add( this );
}


uiSettingsGroup::uiSettingsGroup( uiParent* p, Settings& setts )
    : uiSettsGrp(p,setts,"Settings group")
{
}


uiString uiSettingsGroup::dispStr( Type typ )
{
    switch( typ )
    {
	case General:		return uiStrings::sGeneral();
	case LooknFeel:		return uiStrings::sLooknFeel();
	case Interaction:	return tr("Interaction");
    }

    pFreeFnErrMsg( "Missing case" );
    return uiStrings::sOther();
}


bool uiSettingsGroup::commit( uiRetVal& uirv )
{
    changed_ = needsrestart_ = needsrenewal_ = false;
    doCommit( uirv );
    if ( uirv.isOK() )
    {
	for ( int idx=0; idx<subgrps_.size(); idx++ )
	{
	    uiSettingsSubGroup& subgrp = *subgrps_[idx];
	    subgrp.commit( uirv );
	    if ( !uirv.isOK() )
		break;
	    changed_ = changed_ || subgrp.changed_;
	    needsrestart_ = needsrestart_ || subgrp.needsrestart_;
	    needsrenewal_ = needsrenewal_ || subgrp.needsrenewal_;
	}
    }
    return uirv.isOK();
}


void uiSettingsGroup::rollBack()
{
    doRollBack();
    for ( int idx=0; idx<subgrps_.size(); idx++ )
	subgrps_[idx]->rollBack();
}


void uiSettingsGroup::add( uiSettingsSubGroup* subgrp )
{
    uiGroup* lastgrp = lastGroup();
    if ( lastgrp )
	subgrp->attach( alignedBelow, lastgrp );
    subgrps_ += subgrp;
}


uiGroup* uiSettingsGroup::lastGroup()
{
    return subgrps_.isEmpty() ? bottomobj_
			      : static_cast<uiGroup*>( subgrps_.last() );
}


#define mDefUpdateSettingsFn( type, setfunc ) \
void uiSettsGrp::updateSettings( type oldval, type newval, const char* key ) \
{ \
    if ( oldval != newval ) \
    { \
	changed_ = true; \
	setts_.setfunc( key, newval ); \
    } \
}

mDefUpdateSettingsFn( bool, setYN )
mDefUpdateSettingsFn( int, set )
mDefUpdateSettingsFn( float, set )
mDefUpdateSettingsFn( const OD::String&, set )


// uiStorageSettingsGroup

uiStorageSettingsGroup::uiStorageSettingsGroup( uiParent* p, Settings& setts )
    : uiSettingsGroup(p,setts)
    , initialshstorenab_(false)
{
    setts_.getYN( SettingsAccess::sKeyEnabSharedStor(), initialshstorenab_ );
    enablesharedstorfld_ = new uiGenInput( this,
				tr("Enable shared survey data storage"),
				BoolInpSpec(initialshstorenab_) );
    bottomobj_ = enablesharedstorfld_;
}


void uiStorageSettingsGroup::doCommit( uiRetVal& )
{
    updateSettings( initialshstorenab_, enablesharedstorfld_->getBoolValue(0),
		    SettingsAccess::sKeyEnabSharedStor() );
}


// uiGeneralLnFSettingsGroup

uiGeneralLnFSettingsGroup::uiGeneralLnFSettingsGroup( uiParent* p, Settings& s )
    : uiSettingsGroup(p,s)
    , initialtbsz_(thetbsz_ < 0 ? uiObject::toolButtonSize() : thetbsz_)
    , iconsetsel_(0)
{
    themesel_ = new uiThemeSel( this, true );

    uiGroup* hattgrp = themesel_;
    BufferStringSet icsetnms;
    uiIconSetSel::getSetNames( icsetnms );
    if ( uiIconSetSel::canSelect(icsetnms) )
    {
	iconsetsel_ = new uiIconSetSel( this, icsetnms, true );
	iconsetsel_->attach( alignedBelow, themesel_ );
	hattgrp = iconsetsel_;
    }

    tbszfld_ = new uiGenInput( this, tr("ToolButton Size"),
				 IntInpSpec(initialtbsz_,10,64) );
    tbszfld_->attach( alignedBelow, hattgrp );

    bottomobj_ = tbszfld_;
}


void uiGeneralLnFSettingsGroup::doRollBack()
{
    themesel_->revert();
    if ( iconsetsel_ )
	iconsetsel_->revert();
}


void uiGeneralLnFSettingsGroup::doCommit( uiRetVal& )
{
    if ( themesel_->putInSettings(false) )
	changed_ = true;
    if ( iconsetsel_ && iconsetsel_->newSetSelected() )
	changed_ = true;

    const int newtbsz = tbszfld_->getIntValue();
    if ( newtbsz != initialtbsz_ )
    {
	IOPar* iopar = setts_.subselect( SettingsAccess::sKeyIcons() );
	if ( !iopar ) iopar = new IOPar;
	iopar->set( "size", newtbsz );
	setts_.mergeComp( *iopar, SettingsAccess::sKeyIcons() );
	changed_ = true;
	needsrestart_ = true;
	delete iopar;
	thetbsz_ = newtbsz;
    }
}


// uiProgressSettingsGroup

uiProgressSettingsGroup::uiProgressSettingsGroup( uiParent* p, Settings& s )
    : uiSettingsGroup(p,s)
    , initialshowinlprogress_(true)
    , initialshowcrlprogress_(true)
    , initialshowrdlprogress_(true)
{
    setts_.getYN( SettingsAccess::sKeyShowInlProgress(),
		  initialshowinlprogress_ );
    setts_.getYN( SettingsAccess::sKeyShowCrlProgress(),
		  initialshowcrlprogress_ );
    setts_.getYN( SettingsAccess::sKeyShowRdlProgress(),
		  initialshowrdlprogress_ );
    showprogressfld_ = new uiCheckList( this );
    showprogressfld_->setLabel( tr("Show progress loading") );
    showprogressfld_->addItem( uiStrings::sInline(mPlural), "cube_inl" );
    showprogressfld_->addItem( uiStrings::sCrossline(mPlural), "cube_crl" );
    showprogressfld_->addItem( uiStrings::sRandomLine(mPlural),
			       "cube_randomline" );
    showprogressfld_->setChecked( 0, initialshowinlprogress_ );
    showprogressfld_->setChecked( 1, initialshowcrlprogress_ );
    showprogressfld_->setChecked( 2, initialshowrdlprogress_ );

    bottomobj_ = showprogressfld_;
}


void uiProgressSettingsGroup::doCommit( uiRetVal& )
{
    updateSettings( initialshowinlprogress_, showprogressfld_->isChecked(0),
		    SettingsAccess::sKeyShowInlProgress() );
    updateSettings( initialshowcrlprogress_, showprogressfld_->isChecked(1),
		    SettingsAccess::sKeyShowCrlProgress() );
    updateSettings( initialshowrdlprogress_, showprogressfld_->isChecked(2),
		    SettingsAccess::sKeyShowRdlProgress() );
}


// uiVisSettingsGroup

uiVisSettingsGroup::uiVisSettingsGroup( uiParent* p, Settings& setts )
    : uiSettingsGroup(p,setts)
    , initialdefsurfres_((int)OD::SurfaceResolution::Automatic)
    , initialtextureresindex_(0)
    , initialusesurfshaders_(true)
    , initialusevolshaders_(true)
    , initialenablemipmapping_(true)
    , initialanisotropicpower_(4)
{
    uiLabel* shadinglbl = new uiLabel( this,
				tr("Use OpenGL shading when available:") );
    setts_.getYN( SettingsAccess::sKeyUseSurfShaders(),
		  initialusesurfshaders_ );
    usesurfshadersfld_ = new uiGenInput( this, tr("for surface rendering"),
					 BoolInpSpec(initialusesurfshaders_) );
    usesurfshadersfld_->attach( ensureBelow, shadinglbl );

    setts_.getYN( SettingsAccess::sKeyUseVolShaders(), initialusevolshaders_ );
    usevolshadersfld_ = new uiGenInput( this, tr("for volume rendering"),
					BoolInpSpec(initialusevolshaders_) );
    usevolshadersfld_->attach( leftAlignedBelow, usesurfshadersfld_, 0 );

    uiStringSet itmnms;
    itmnms.add( uiStrings::sStandard() )
	  .add( uiStrings::sHigher() )
	  .add( uiStrings::sHighest() );
    if ( SettingsAccess::systemHasDefaultTexResFactor() )
	itmnms.add( tr("System default") );
    StringListInpSpec resinpspec( itmnms );
    initialtextureresindex_ = SettingsAccess(setts_)
					.getDefaultTexResAsIndex( 3 );
    resinpspec.setValue( initialtextureresindex_ );
    textureresfactorfld_ = new uiGenInput( this,
					   tr("Default texture resolution"),
					   resinpspec );
    textureresfactorfld_->attach( alignedBelow, usevolshadersfld_ );

    setts_.get( OD::sSurfaceResolutionSettingsKey(), initialdefsurfres_ );
    itmnms.setEmpty(); const int lastidx = (int)OD::cMinSurfaceResolution();
    for ( int idx=0; idx<=lastidx; idx++ )
	itmnms.add( OD::getSurfaceResolutionDispStr(
				(OD::SurfaceResolution)idx ) );
    resinpspec = StringListInpSpec( itmnms );
    resinpspec.setValue( initialdefsurfres_ );
    surfdefresfld_ = new uiGenInput( this, tr("Default Surface Resolution"),
				     resinpspec );
    surfdefresfld_->attach( alignedBelow, textureresfactorfld_ );

    setts_.getYN( SettingsAccess::sKeyEnableMipmapping(),
					initialenablemipmapping_ );
    enablemipmappingfld_ = new uiGenInput( this, tr("Mipmap anti-aliasing"),
					BoolInpSpec(initialenablemipmapping_) );
    enablemipmappingfld_->attach( alignedBelow, surfdefresfld_ );
    enablemipmappingfld_->valuechanged.notify(
			    mCB(this,uiVisSettingsGroup,mipmappingToggled) );

    setts_.get( SettingsAccess::sKeyAnisotropicPower(),
		initialanisotropicpower_ );
    if ( initialanisotropicpower_ < -1 )
	initialanisotropicpower_ = -1;
    if ( initialanisotropicpower_ > 5 )
	initialanisotropicpower_ = 5;
    itmnms.setEmpty();
    itmnms.add( toUiString(" 0 x") ).add( toUiString(" 1 x") )
	  .add( toUiString(" 2 x") ).add( toUiString(" 4 x") )
	  .add( toUiString(" 8 x") ).add( toUiString("16 x") )
	  .add( toUiString("32 x") );
    StringListInpSpec powinpspec( itmnms );
    powinpspec.setValue( initialanisotropicpower_+1 );
    anisotropicpowerfld_= new uiGenInput( this, tr("Sharpen oblique textures"),
					  powinpspec );
    anisotropicpowerfld_->attach( alignedBelow, enablemipmappingfld_ );

    mipmappingToggled( 0 );

    bottomobj_ = anisotropicpowerfld_;
}


void uiVisSettingsGroup::mipmappingToggled( CallBacker* )
{
    anisotropicpowerfld_->setSensitive( enablemipmappingfld_->getBoolValue() );
}


void uiVisSettingsGroup::doCommit( uiRetVal& )
{
    const bool usesurfshaders = usesurfshadersfld_->getBoolValue();
    updateSettings( initialusesurfshaders_, usesurfshaders,
		    SettingsAccess::sKeyUseSurfShaders() );
    const bool usevolshaders = usevolshadersfld_->getBoolValue();
    updateSettings( initialusevolshaders_, usevolshaders,
		    SettingsAccess::sKeyUseVolShaders() );

    const int idx = textureresfactorfld_->getIntValue();
    if ( initialtextureresindex_ != idx )
    {
	changed_ = true;
	SettingsAccess(setts_).setDefaultTexResAsIndex( idx, 3 );
    }

    updateSettings( initialdefsurfres_, surfdefresfld_->getIntValue(),
		    OD::sSurfaceResolutionSettingsKey() );

    updateSettings( initialenablemipmapping_,
		    enablemipmappingfld_->getBoolValue(),
		    SettingsAccess::sKeyEnableMipmapping() );

    const int anisotropicpower = anisotropicpowerfld_->getIntValue()-1;
    updateSettings( initialanisotropicpower_, anisotropicpower,
		    SettingsAccess::sKeyAnisotropicPower() );

    if ( changed_ )
	needsrenewal_ = true;
}


class uiSettingsTypeTreeItm : public uiTreeViewItem
{
public:

uiSettingsTypeTreeItm( uiTreeView* p, uiSettingsGroup::Type typ )
    : uiTreeViewItem(p,Setup(uiSettingsGroup::dispStr(typ)))
    , type_(typ)
{
    const char* icid = 0;
    switch ( type_ )
    {
	case uiSettingsGroup::General:		icid = "settings";	break;
	case uiSettingsGroup::LooknFeel:	icid = "looknfeel";	break;
	case uiSettingsGroup::Interaction:	icid = "interaction";	break;
    }
    setIcon( 0, icid );
    setSelectable( false );
    setToolTip( 0, uiString::empty() );
}

~uiSettingsTypeTreeItm()
{
    detachAllNotifiers();
}

    const uiSettingsGroup::Type	type_;

};


class uiSettingsSubjectTreeItm : public uiTreeViewItem
{
public:

uiSettingsSubjectTreeItm( uiSettingsTypeTreeItm* typitm, uiSettingsGroup& grp )
    : uiTreeViewItem(typitm,Setup(grp.subject()))
    , typitm_(*typitm)
    , grp_(grp)
{
    setIcon( 0, grp.iconID() );
    setToolTip( 0, uiString::empty() );
}

~uiSettingsSubjectTreeItm()
{
    detachAllNotifiers();
}

    uiSettingsGroup&		grp_;
    uiSettingsTypeTreeItm&	typitm_;

};


mDefineInstanceCreatedNotifierAccess(uiSettingsDlg);

uiSettingsDlg::uiSettingsDlg( uiParent* p, const char* initialgrpky )
    : uiDialog(p,uiDialog::Setup(tr("OpendTect Settings"),mNoDlgTitle,
				      mTODOHelpKey))
    , setts_(Settings::common())
    , havechanges_(false)
    , restartneeded_(false)
    , renewalneeded_(false)
    , curtreeitm_(0)
{
    if ( !initialgrpky )
	initialgrpky = uiStorageSettingsGroup::sFactoryKeyword();

    uiGroup* leftgrp = new uiGroup( this, "uiSettingsGroup tree" );
    treefld_ = new uiTreeView( leftgrp, "uiSettingsGroup tree" );
    treefld_->showHeader( false );
    ObjectSet<uiSettingsTypeTreeItm> typitms;
#   define mAddTypeTreeItm(typ) \
    typitms += new uiSettingsTypeTreeItm( treefld_, uiSettingsGroup::typ )
    mAddTypeTreeItm( General );
    mAddTypeTreeItm( LooknFeel );
    mAddTypeTreeItm( Interaction );
    treefld_->setStretch( 1, 2 );

    uiGroup* rightgrp = new uiGroup( this, "Right group" );

    grplbl_ = new uiLabel( rightgrp, uiString::empty() );
    grplbl_->setPrefWidthInChar( 60 );
    grplbl_->setAlignment( OD::Alignment::Left );
    uiSeparator* sep = new uiSeparator( rightgrp, "Hor Sep" );
    sep->attach( stretchedBelow, grplbl_ );

    const BufferString defgrp( initialgrpky );
    uiGroup* settsgrp = new uiGroup( rightgrp, "uiSettingsGroup area" );
    const BufferStringSet& kys = uiSettingsGroup::factory().getKeys();
    for ( int idx=0; idx<kys.size(); idx++ )
    {
	const char* ky = kys.get( idx ).buf();
	uiSettingsGroup* grp = uiSettingsGroup::factory().create(
				    ky, settsgrp, setts_ );
	uiSettingsSubjectTreeItm* newitm =
		new uiSettingsSubjectTreeItm( typitms[grp->type()], *grp );
	treeitms_ += newitm;
	if ( defgrp == ky )
	    curtreeitm_ = newitm;
    }

    settsgrp->attach( ensureBelow, sep );

    uiSplitter* spl = new uiSplitter( this );
    spl->addGroup( leftgrp );
    spl->addGroup( rightgrp );
    spl->setPrefHeightInChar( 15 );

    mAttachCB( postFinalise(), uiSettingsDlg::initWin );
    mTriggerInstanceCreatedNotifier();
}


uiSettingsDlg::~uiSettingsDlg()
{
    detachAllNotifiers();
}


void uiSettingsDlg::initWin( CallBacker* )
{
    if ( treeitms_.isEmpty() )
	return;

    treefld_->expandAll();
    mAttachCB( treefld_->selectionChanged, uiSettingsDlg::selChgCB );

    // force first update
    uiSettingsSubjectTreeItm* curitm = curtreeitm_;
    curtreeitm_ = 0;
    if ( curitm )
    {
	treefld_->setCurrentItem( curitm );
	selChgCB( curitm );
    }
}


void uiSettingsDlg::selChgCB( CallBacker* )
{
    mDynamicCastGet(uiSettingsSubjectTreeItm*,curitm,treefld_->selectedItem())
    if ( !curitm || treeitms_.isEmpty() || curtreeitm_ == curitm )
	return;

    for ( int idx=0; idx<treeitms_.size(); idx++ )
    {
	uiSettingsSubjectTreeItm* itm = treeitms_[idx];
	uiSettingsGroup& grp = itm->grp_;
	grp.display( itm == curitm );
	if ( itm == curtreeitm_ )
	    grp.setActive( false );
	else if ( itm == curitm )
	    grp.setActive( true );
    }

    curtreeitm_ = curitm;
    uiString typstr = uiSettingsGroup::dispStr( curtreeitm_->typitm_.type_ );
    grplbl_->setText( toUiString("- %1: %2 Settings")
		      .arg( typstr ).arg( curtreeitm_->grp_.subject() ) );
}


void uiSettingsDlg::handleRestart()
{
    if ( uiMSG().askGoOn(tr("Your new settings will become active"
		       "\nthe next time OpendTect is started."
		       "\n\nDo you want to restart now?")) )
	RestartProgram();
}


uiSettingsGroup* uiSettingsDlg::getGroup( const char* factky )
{
    const FixedString tofind( factky );
    for ( int idx=0; idx<treeitms_.size(); idx++ )
    {
	uiSettingsGroup* grp = &treeitms_[idx]->grp_;
	if ( tofind == grp->factoryKeyword() )
	    return grp;
    }
    return 0;
}


bool uiSettingsDlg::rejectOK()
{
    for ( int idx=0; idx<treeitms_.size(); idx++ )
	treeitms_[idx]->grp_.rollBack();
    return true;
}


bool uiSettingsDlg::acceptOK()
{
    uiRetVal uirv;
    for ( int idx=0; idx<treeitms_.size(); idx++ )
	treeitms_[idx]->grp_.commit( uirv );
    if ( !uirv.isOK() )
	{ uiMSG().error( uirv ); return false; }

    havechanges_ = restartneeded_ = renewalneeded_ = false;

    for ( int idx=0; idx<treeitms_.size(); idx++ )
	havechanges_ = havechanges_ || treeitms_[idx]->grp_.isChanged();
    if ( havechanges_ && !setts_.write() )
	{ uiMSG().error( uiStrings::phrCannotWriteSettings() ); return false; }

    for ( int idx=0; idx<treeitms_.size(); idx++ )
    {
	restartneeded_ = restartneeded_ || treeitms_[idx]->grp_.needsRestart();
	renewalneeded_ = renewalneeded_ || treeitms_[idx]->grp_.needsRenewal();
    }

    if ( restartneeded_ )
	handleRestart();
    else if ( renewalneeded_ )
	uiMSG().message(tr("Your new settings will become active\n"
			   "only for newly launched objects."));

    return true;
}
