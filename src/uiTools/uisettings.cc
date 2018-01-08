/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:          November 2001
________________________________________________________________________

-*/

#include "uisettings.h"

#include "dirlist.h"
#include "envvars.h"
#include "genc.h"
#include "oddirs.h"
#include "od_helpids.h"
#include "posimpexppars.h"
#include "ptrman.h"
#include "settingsaccess.h"
#include "survinfo.h"

#include "uichecklist.h"
#include "uicombobox.h"
#include "uigeninput.h"
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
	grpfld_->valuechanged.notify( mCB(this,uiAdvSettings,grpChg) );
    }

    tbl_ = new uiTable( this, uiTable::Setup(10,2).manualresize(true),
				"Settings editor" );
    tbl_->setColumnLabel( 0, tr("Keyword") );
    tbl_->setColumnLabel( 1, uiStrings::sValue() );
    tbl_->setStretch( 2, 2 );
    tbl_->setPrefWidth( 400 );
    tbl_->setPrefHeight( 300 );
    if ( grpfld_ )
	tbl_->attach( ensureBelow, grpfld_ );

    postFinalise().notify( mCB(this,uiAdvSettings,dispNewGrp) );
}


uiAdvSettings::~uiAdvSettings()
{
    deepErase( chgdsetts_ );
}


int uiAdvSettings::getChgdSettIdx( const char* nm ) const
{
    for ( int idx=0; idx<chgdsetts_.size(); idx++ )
    {
	if ( chgdsetts_[idx]->name() == nm )
	    return idx;
    }
    return -1;
}


const IOPar& uiAdvSettings::orgPar() const
{
    const IOPar* iop = &sipars_;
    if ( !issurvdefs_ )
    {
	const BufferString grp( grpfld_ ? grpfld_->text() : sKeyCommon );
	iop = grp == sKeyCommon ? &Settings::common() : &Settings::fetch(grp);
    }
    return *iop;
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


static int theiconsz = -1;
static BoolTypeSet settgrpislnf_;


mImplFactory2Param( uiSettingsGroup, uiParent*, Settings&,
		    uiSettingsGroup::factory )

void uiSettingsGroup::setIsLooknFeelGroup( const char* nm, bool yn )
{
    for ( int idx=0; idx<factory().size(); idx++ )
    {
	if ( factory().getNames().get(idx) == nm )
	{
	    for ( int iflag=settgrpislnf_.size(); iflag<=idx; iflag++ )
		settgrpislnf_ += true;

	    settgrpislnf_[idx] = yn;
	    return;;
	}
    }
}


bool uiSettingsGroup::isLooknFeelGroup( const char* nm )
{
    const int idx = factory().getNames().indexOf( nm );
    return settgrpislnf_.validIdx(idx) ? (bool)settgrpislnf_[idx] : true;
}


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


uiString uiSettingsGroup::errMsg() const
{ return errmsg_; }


#define mDefUpdateSettings( type, setfunc ) \
void uiSettingsGroup::updateSettings( type oldval, type newval, \
				      const char* key ) \
{ \
    if ( oldval != newval ) \
    { \
	changed_ = true; \
	setts_.setfunc( key, newval ); \
    } \
}

mDefUpdateSettings( bool, setYN )
mDefUpdateSettings( int, set )
mDefUpdateSettings( float, set )
mDefUpdateSettings( const OD::String&, set )


// uiGeneralSettingsGroup
uiGeneralSettingsGroup::uiGeneralSettingsGroup( uiParent* p, Settings& setts )
    : uiSettingsGroup(p,uiStrings::sGeneral(),setts)
    , enabsharedstor_(false)
{
    setts_.getYN( SettingsAccess::sKeyEnabSharedStor(), enabsharedstor_ );
    enablesharedstorfld_ = new uiGenInput( this,
				tr("Enable shared survey data storage"),
				BoolInpSpec(enabsharedstor_) );
}


bool uiGeneralSettingsGroup::acceptOK()
{
    updateSettings( enabsharedstor_, enablesharedstorfld_->getBoolValue(0),
		    SettingsAccess::sKeyEnabSharedStor() );
    return true;
}


// uiGeneralLnFSettingsGroup
uiGeneralLnFSettingsGroup::uiGeneralLnFSettingsGroup( uiParent* p,
							Settings& setts )
    : uiSettingsGroup(p,uiStrings::sGeneral(),setts)
    , iconsz_(theiconsz < 0 ? uiObject::iconSize() : theiconsz)
    , showinlprogress_(true)
    , showcrlprogress_(true)
    , showrdlprogress_(true)
    , enabvirtualkeyboard_(false)
{
    iconszfld_ = new uiGenInput( this, tr("Icon Size"),
				 IntInpSpec(iconsz_,12,128) );

    setts_.getYN( uiVirtualKeyboard::sKeyEnabVirtualKeyboard(),
		  enabvirtualkeyboard_ );
    virtualkeyboardfld_ = new uiGenInput( this,
		tr("Enable Virtual Keyboard"),
		BoolInpSpec(enabvirtualkeyboard_) );
    virtualkeyboardfld_->attach( alignedBelow, iconszfld_ );

    setts_.getYN( SettingsAccess::sKeyShowInlProgress(), showinlprogress_ );
    setts_.getYN( SettingsAccess::sKeyShowCrlProgress(), showcrlprogress_ );
    setts_.getYN( SettingsAccess::sKeyShowRdlProgress(), showrdlprogress_ );
    showprogressfld_ = new uiCheckList( this );
    showprogressfld_->setLabel( tr("Show progress when loading for") );
    showprogressfld_->addItem( uiStrings::sInline(mPlural), "cube_inl" );
    showprogressfld_->addItem( uiStrings::sCrossline(mPlural), "cube_crl" );
    showprogressfld_->addItem( uiStrings::sRandomLine(mPlural),
			       "cube_randomline" );
    showprogressfld_->setChecked( 0, showinlprogress_ );
    showprogressfld_->setChecked( 1, showcrlprogress_ );
    showprogressfld_->setChecked( 2, showrdlprogress_ );
    showprogressfld_->attach( alignedBelow, virtualkeyboardfld_ );
}


bool uiGeneralLnFSettingsGroup::acceptOK()
{
    const int newiconsz = iconszfld_->getIntValue();
    if ( newiconsz < 10 || newiconsz > 64 )
    {
	errmsg_ = uiStrings::phrSpecify(tr("an icon size in the range 10-64"));
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

    updateSettings( showinlprogress_, showprogressfld_->isChecked(0),
		    SettingsAccess::sKeyShowInlProgress() );
    updateSettings( showcrlprogress_, showprogressfld_->isChecked(1),
		    SettingsAccess::sKeyShowCrlProgress() );
    updateSettings( showrdlprogress_, showprogressfld_->isChecked(2),
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
uiSettingsDlg::uiSettingsDlg( uiParent* p, bool islooknfeel )
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
	const char* nm = nms.get( idx ).buf();
	if ( uiSettingsGroup::isLooknFeelGroup(nm) != islooknfeel )
	    continue;

	uiSettingsGroup* grp = uiSettingsGroup::factory().create(
				    nm, tabstack_->tabGroup(), setts_ );
	grp->attach( hCentered );
	addGroup( grp );
	grps_ += grp;
    }
}


uiSettingsDlg::~uiSettingsDlg()
{
}


void uiSettingsDlg::handleRestart()
{
    if ( uiMSG().askGoOn(tr("Your new settings will become active"
		       "\nthe next time OpendTect is started."
		       "\n\nDo you want to restart now?")) )
	RestartProgram();
}


bool uiSettingsDlg::acceptOK()
{
    if ( !uiTabStackDlg::acceptOK() )
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
	handleRestart();
    else if ( needsrenewal_ )
	uiMSG().message(tr("Your new settings will become active\n"
			   "only for newly launched objects."));

    return true;
}
