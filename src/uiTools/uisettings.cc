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
#include "uisplitter.h"
#include "uitable.h"
#include "uithemesel.h"
#include "uitreeview.h"
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


static int theiconsz_ = -1;
mImplFactory2Param( uiSettingsGroup, uiParent*, Settings&,
		    uiSettingsGroup::factory )


uiSettingsGroup::uiSettingsGroup( uiParent* p, Settings& setts )
    : uiGroup(p,"Settings group")
    , setts_(setts)
    , needsrestart_(false)
    , needsrenewal_(false)
{
}


uiSettingsGroup::~uiSettingsGroup()
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
    doCommit( uirv );
    return uirv.isOK();
}


#define mDefUpdateSettingsFn( type, setfunc ) \
void uiSettingsGroup::updateSettings( type oldval, type newval, \
				      const char* key ) \
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
}


void uiStorageSettingsGroup::doCommit( uiRetVal& )
{
    updateSettings( initialshstorenab_, enablesharedstorfld_->getBoolValue(0),
		    SettingsAccess::sKeyEnabSharedStor() );
}


// uiGeneralLnFSettingsGroup
uiGeneralLnFSettingsGroup::uiGeneralLnFSettingsGroup( uiParent* p, Settings& s )
    : uiSettingsGroup(p,s)
    , initialiconsz_(theiconsz_ < 0 ? uiObject::iconSize() : theiconsz_)
    , initialshowinlprogress_(true)
    , initialshowcrlprogress_(true)
    , initialshowrdlprogress_(true)
    , initialenabvirtualkeyboard_(false)
{
    themesel_ = new uiThemeSel( this, true );

    iconszfld_ = new uiGenInput( this, tr("Icon Size"),
				 IntInpSpec(initialiconsz_,10,64) );
    iconszfld_->attach( alignedBelow, themesel_ );

    setts_.getYN( uiVirtualKeyboard::sKeyEnabVirtualKeyboard(),
		  initialenabvirtualkeyboard_ );
    virtualkeyboardfld_ = new uiGenInput( this,
		tr("Enable Virtual Keyboard"),
		BoolInpSpec(initialenabvirtualkeyboard_) );
    virtualkeyboardfld_->attach( alignedBelow, iconszfld_ );

    setts_.getYN( SettingsAccess::sKeyShowInlProgress(),
		  initialshowinlprogress_ );
    setts_.getYN( SettingsAccess::sKeyShowCrlProgress(),
		  initialshowcrlprogress_ );
    setts_.getYN( SettingsAccess::sKeyShowRdlProgress(),
		  initialshowrdlprogress_ );
    showprogressfld_ = new uiCheckList( this );
    showprogressfld_->setLabel( tr("Show progress when loading for") );
    showprogressfld_->addItem( uiStrings::sInline(mPlural), "cube_inl" );
    showprogressfld_->addItem( uiStrings::sCrossline(mPlural), "cube_crl" );
    showprogressfld_->addItem( uiStrings::sRandomLine(mPlural),
			       "cube_randomline" );
    showprogressfld_->setChecked( 0, initialshowinlprogress_ );
    showprogressfld_->setChecked( 1, initialshowcrlprogress_ );
    showprogressfld_->setChecked( 2, initialshowrdlprogress_ );
    showprogressfld_->attach( alignedBelow, virtualkeyboardfld_ );
}


void uiGeneralLnFSettingsGroup::doRollBack()
{
    themesel_->revert();
}


void uiGeneralLnFSettingsGroup::doCommit( uiRetVal& )
{
    if ( themesel_->putInSettings(false) )
	changed_ = true;

    const int newiconsz = iconszfld_->getIntValue();
    if ( newiconsz != initialiconsz_ )
    {
	IOPar* iopar = setts_.subselect( SettingsAccess::sKeyIcons() );
	if ( !iopar ) iopar = new IOPar;
	iopar->set( "size", newiconsz );
	setts_.mergeComp( *iopar, SettingsAccess::sKeyIcons() );
	changed_ = true;
	needsrestart_ = true;
	delete iopar;
	theiconsz_ = newiconsz;
    }

    updateSettings( initialshowinlprogress_, showprogressfld_->isChecked(0),
		    SettingsAccess::sKeyShowInlProgress() );
    updateSettings( initialshowcrlprogress_, showprogressfld_->isChecked(1),
		    SettingsAccess::sKeyShowCrlProgress() );
    updateSettings( initialshowrdlprogress_, showprogressfld_->isChecked(2),
		    SettingsAccess::sKeyShowRdlProgress() );
    updateSettings( initialenabvirtualkeyboard_,
		    virtualkeyboardfld_->getBoolValue(),
		    uiVirtualKeyboard::sKeyEnabVirtualKeyboard() );
}


// uiVisSettingsGroup
uiVisSettingsGroup::uiVisSettingsGroup( uiParent* p, Settings& setts )
    : uiSettingsGroup(p,setts)
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
    usesurfshadersfld_->attach( leftAlignedBelow, shadinglbl );

    setts_.getYN( SettingsAccess::sKeyUseVolShaders(), initialusevolshaders_ );
    usevolshadersfld_ = new uiGenInput( this, tr("for volume rendering"),
					BoolInpSpec(initialusevolshaders_) );
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

    initialtextureresindex_ = SettingsAccess(setts_)
					.getDefaultTexResAsIndex( 3 );
    textureresfactorfld_->setCurrentItem( initialtextureresindex_ );

    setts_.getYN( SettingsAccess::sKeyEnableMipmapping(),
					initialenablemipmapping_ );
    enablemipmappingfld_ = new uiGenInput( this, tr("Mipmap anti-aliasing"),
					BoolInpSpec(initialenablemipmapping_) );
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

    setts_.get( SettingsAccess::sKeyAnisotropicPower(),
		initialanisotropicpower_ );
    if ( initialanisotropicpower_ < -1 )
	initialanisotropicpower_ = -1;
    if ( initialanisotropicpower_ > 5 )
	initialanisotropicpower_ = 5;

    anisotropicpowerfld_->box()->setCurrentItem( initialanisotropicpower_+1 );

    mipmappingToggled( 0 );
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

    const int idx = textureresfactorfld_->currentItem();
    if ( initialtextureresindex_ != idx )
    {
	changed_ = true;
	SettingsAccess(setts_).setDefaultTexResAsIndex( idx, 3 );
    }

    updateSettings( initialenablemipmapping_,
		    enablemipmappingfld_->getBoolValue(),
		    SettingsAccess::sKeyEnableMipmapping() );

    const int anisotropicpower = anisotropicpowerfld_->box()->currentItem()-1;
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
    setToolTip( 0, uiString::emptyString() );
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
    setToolTip( 0, uiString::emptyString() );
}

~uiSettingsSubjectTreeItm()
{
    detachAllNotifiers();
}

    uiSettingsGroup&		grp_;
    uiSettingsTypeTreeItm&	typitm_;

};



uiSettingsDlg::uiSettingsDlg( uiParent* p )
    : uiDialog(p,uiDialog::Setup(tr("OpendTect Settings"),mNoDlgTitle,
				      mTODOHelpKey))
    , setts_(Settings::common())
    , havechanges_(false)
    , restartneeded_(false)
    , renewalneeded_(false)
    , curtreeitm_(0)
{
    uiGroup* leftgrp = new uiGroup( this, "uiSettingsGroup tree" );
    treefld_ = new uiTreeView( leftgrp, "uiSettingsGroup tree" );
    treefld_->showHeader( false );
    ObjectSet<uiSettingsTypeTreeItm> typitms;
#   define mAddTypeTreeItm(typ) \
    typitms += new uiSettingsTypeTreeItm( treefld_, uiSettingsGroup::typ )
    mAddTypeTreeItm( General );
    mAddTypeTreeItm( LooknFeel );
    mAddTypeTreeItm( Interaction );

    uiGroup* rightgrp = new uiGroup( this, "uiSettingsGroup area" );
    const BufferStringSet& nms = uiSettingsGroup::factory().getNames();
    for ( int idx=0; idx<nms.size(); idx++ )
    {
	const char* nm = nms.get( idx ).buf();
	uiSettingsGroup* grp = uiSettingsGroup::factory().create(
				    nm, rightgrp, setts_ );
	treeitms_ += new uiSettingsSubjectTreeItm( typitms[grp->type()], *grp );
    }

    uiSplitter* spl = new uiSplitter( this );
    spl->addGroup( leftgrp );
    spl->addGroup( rightgrp );

    mAttachCB( postFinalise(), uiSettingsDlg::initWin );
}


uiSettingsDlg::~uiSettingsDlg()
{
    detachAllNotifiers();
}


void uiSettingsDlg::initWin( CallBacker* )
{
    if ( treeitms_.isEmpty() )
	return;

    int defitmidx = 0;
    for ( int idx=0; idx<treeitms_.size(); idx++ )
	if ( FixedString(treeitms_[idx]->grp_.factoryKeyword())
		== uiGeneralLnFSettingsGroup::sFactoryKeyword() )
	    { defitmidx = idx; break; }
    uiSettingsSubjectTreeItm* itm = treeitms_[defitmidx];

    treefld_->expandAll();
    mAttachCB( treefld_->selectionChanged, uiSettingsDlg::selChgCB );
    treefld_->setCurrentItem( itm );
    selChgCB( itm );
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
}


void uiSettingsDlg::handleRestart()
{
    if ( uiMSG().askGoOn(tr("Your new settings will become active"
		       "\nthe next time OpendTect is started."
		       "\n\nDo you want to restart now?")) )
	RestartProgram();
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

    havechanges_ = false;
    for ( int idx=0; idx<treeitms_.size(); idx++ )
	havechanges_ = havechanges_ || treeitms_[idx]->grp_.isChanged();

    if ( havechanges_ && !setts_.write() )
	{ uiMSG().error( uiStrings::sCantWriteSettings() ); return false; }

    restartneeded_ = false; renewalneeded_ = false;
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
