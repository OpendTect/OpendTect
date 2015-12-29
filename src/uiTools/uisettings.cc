/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:          November 2001
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "uisettings.h"

#include "dirlist.h"
#include "envvars.h"
#include "oddirs.h"
#include "od_helpids.h"
#include "posimpexppars.h"
#include "ptrman.h"
#include "settingsaccess.h"
#include "survinfo.h"

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
	grps.add( underscoreptr + 1 );
    }
}


uiSettings::uiSettings( uiParent* p, const char* nm, const char* settskey )
	: uiDialog(p,uiDialog::Setup(toUiString(nm),
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
	grpfld_->valuechanged.notify( mCB(this,uiSettings,grpChg) );
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

    postFinalise().notify( mCB(this,uiSettings,dispNewGrp) );
}


uiSettings::~uiSettings()
{
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
    const IOPar* iop = &SI().getPars();
    if ( !issurvdefs_ )
    {
	const BufferString grp( grpfld_ ? grpfld_->text() : sKeyCommon );
	iop = grp == sKeyCommon ? &Settings::common() : &Settings::fetch(grp);
    }
    return *iop;
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

    needsrestart_ = false;
    needsrenewal_ = false;
    for ( int idx=0; idx<grps_.size(); idx++ )
    {
	needsrestart_ = needsrestart_ || grps_[idx]->needsRestart();
	needsrenewal_ = needsrenewal_ || grps_[idx]->needsRenewal();
    }

    return true;
}
