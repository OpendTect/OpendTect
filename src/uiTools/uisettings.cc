/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:          November 2001
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "uisettings.h"

#include "bufstring.h"
#include "dirlist.h"
#include "envvars.h"
#include "file.h"
#include "filepath.h"
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
#include "uimsg.h"
#include "uistrings.h"
#include "uitable.h"
#include "uivirtualkeyboard.h"


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
    DirList dl( GetSettingsDir(), DirList::FilesOnly, msk );
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
	grps.add( underscoreptr );
    }
}


uiSettings::uiSettings( uiParent* p, const char* nm, const char* settskey )
	: uiDialog(p,uiDialog::Setup(mToUiStringTodo(nm),
				     tr("Set User Settings value"),
                                     mODHelpKey(mSettingsHelpID)) )
        , issurvdefs_(FixedString(settskey)==sKeySurveyDefs())
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

    mAttachCB( postFinalise(), uiSettings::dispNewGrp );
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
    for ( int idx=0; idx<cursetts_->size(); idx++ )
    {
	keys.add( cursetts_->getKey(idx) );
	vals.add( cursetts_->getValue(idx) );
    }
    int* idxs = keys.getSortIndexes();
    keys.useIndexes(idxs); vals.useIndexes(idxs);
    delete [] idxs;

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
    usesurfshadersfld_->attach( leftAlignedBelow, shadinglbl );

    setts_.getYN( SettingsAccess::sKeyUseVolShaders(), usevolshaders_ );
    usevolshadersfld_ = new uiGenInput( this, tr("for volume rendering"),
					BoolInpSpec(usevolshaders_) );
    usevolshadersfld_->attach( leftAlignedBelow, usesurfshadersfld_, 0 );

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

    mipmappingToggled( 0 );
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
	internalloc_ = new uiFileInput( this, tr("Environment root") );
	internalloc_->setSelectMode( uiFileDialog::DirectoryOnly );
	internalloc_->attach( alignedBelow, pythonsrcfld_ );
	mAttachCB( internalloc_->valuechanged, uiPythonSettings::parChgCB );
    }

    customloc_ = new uiFileInput( this,tr("Custom Environment root"));
    customloc_->setSelectMode( uiFileDialog::DirectoryOnly );
    customloc_->attach( alignedBelow, pythonsrcfld_ );
    mAttachCB( customloc_->valuechanged, uiPythonSettings::customEnvChgCB );

    customenvnmfld_ = new uiGenInput( this, tr("Environment"),
				      StringListInpSpec() );
    customenvnmfld_->attach( alignedBelow, customloc_ );
    mAttachCB( customenvnmfld_->valuechanged, uiPythonSettings::parChgCB );

    uiButton* testbut = new uiPushButton( this, tr("Test"),
			mCB(this, uiPythonSettings, testCB), true);
    testbut->attach( ensureBelow, customenvnmfld_ );

	uiButton* cmdwinbut = new uiPushButton( this, tr("Launch Prompt"),
		 mCB(this, uiPythonSettings, promptCB), true );
	cmdwinbut->attach( rightOf, testbut );

    mAttachCB( postFinalise(), uiPythonSettings::initDlg );
}

virtual ~uiPythonSettings()
{
}

private:

void setPythonPath()
{
	const FilePath scriptbinfp( GetSoftwareDir(true), "v7", "bin" );
	const FilePath pythonmodsfp( scriptbinfp.fullPath(), "python" );
	if ( pythonmodsfp.exists() )
	{
		BufferStringSet pythonpaths;
		GetEnvVarDirList( "PYTHONPATH", pythonpaths, true );
		if ( pythonpaths.addIfNew(pythonmodsfp.fullPath()) )
			SetEnvVarDirList( "PYTHONPATH", pythonpaths, true );
	}
}

void initDlg(CallBacker*)
{
    usePar(curSetts());
    fillPar( initialsetts_ ); //Backup for restore
	setPythonPath();
    sourceChgCB(0);
}

IOPar&	curSetts()
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
    if ( !curSetts().isEqual(*workpar, true) )
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
    if (!OD::PythonSourceDef().parse(par,
		OD::PythonAccess::sKeyPythonSrc(), source))
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
	{
	    customloc_->setFileName( envroot );
	    customEnvChgCB( nullptr );
	}
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
    const FilePath fp( envroot, "envs" );
    if ( !fp.exists() )
        return;

    BufferStringSet envnames;
    const DirList dl( fp.fullPath(), DirList::DirsOnly );
    for ( int idx=0; idx<dl.size(); idx++ )
        envnames.add( FilePath(dl.fullPath(idx)).baseName() );
    customenvnmfld_->setEmpty();
    customenvnmfld_->newSpec( StringListInpSpec(envnames), 0 );
}

void testPythonModules()
{
    ManagedObjectSet<OD::PythonAccess::ModuleInfo> modules;
    const uiRetVal uirv( OD::PythA().getModules(modules) );
    if ( !uirv.isOK() )
    {
        uiMSG().error( uirv );
        return;
    }

    BufferStringSet modstrs;
    for ( int idx=0; idx<modules.size(); idx++ )
        modstrs.add( modules[idx]->displayStr() );

    uiMSG().message( tr("Detected list of Python modules:\n%1")
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
        uiMSG().error( uirv );
        return;
    }

    BufferString versionstr( OD::PythA().lastOutput(false,nullptr) );
    if ( versionstr.isEmpty() )
        versionstr.set( OD::PythA().lastOutput(true,nullptr) );
    uiMSG().message( tr("Detected Python version: %1").arg(versionstr));

    usw.setMessage( tr("Retrieving list of installed Python modules") );
    testPythonModules();
}

void promptCB( CallBacker* )
{
#ifdef __win__
	const BufferString cmdstr( "start cmd.exe" );
	OS::MachineCommand oscmd( cmdstr );
	OD::PythA().execute( oscmd, OS::CommandExecPars(OS::RunInBG) );
#else
	//TODO: implement
#endif
}

bool useScreen()
{
    const int sourceidx = pythonsrcfld_->getIntValue();
    const OD::PythonSource source =
                OD::PythonSourceDef().getEnumForIndex(sourceidx);

    if ( source == OD::Internal && internalloc_ )
    {
        const BufferString envroot( internalloc_->fileName() );
        if ( !File::exists(envroot) || !File::isDirectory(envroot) )
        {
            uiMSG().error( uiStrings::phrSelect(uiStrings::sDirectory()) );
            return false;
        }

        const FilePath envrootfp( envroot );
        if ( !OD::PythonAccess::validInternalEnvironment(envrootfp) )
        {
            uiMSG().error( tr("Invalid environment root") );
            return false;
        }
    }
    else if ( source == OD::Custom )
    {
        const BufferString envroot( customloc_->fileName() );
        if ( !File::exists(envroot) || !File::isDirectory(envroot) )
        {
            uiMSG().error( uiStrings::phrSelect(uiStrings::sDirectory()) );
            return false;
        }

        const FilePath envrootfp( envroot, "envs" );
        if ( !envrootfp.exists() )
        {
            uiMSG().error( tr("%1 does not contains a directory called %2")
                                .arg(uiStrings::sDirectory()).arg("envs") );
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

bool rejectOK( CallBacker* )
{
    return needrestore_ ? commitSetts( initialsetts_ ) : true;
}

bool acceptOK( CallBacker* )
{
    needrestore_ = false;
    return useScreen();
}

    uiGenInput*		pythonsrcfld_;
    uiFileInput*	internalloc_ = nullptr;
    uiFileInput*	customloc_;
    uiGenInput*		customenvnmfld_;

    IOPar*		chgdsetts_ = nullptr;
    bool		needrestore_ = false;
    IOPar		initialsetts_;

};



uiDialog* uiSettings::getPythonDlg( uiParent* p, const char* nm )
{
    uiDialog* ret = new uiPythonSettings( p, nm );
    ret->setModal( false );
    ret->setDeleteOnClose( true );
    return ret;
}
