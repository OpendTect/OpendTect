/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:          November 2001
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "uisettings.h"

#include "ptrman.h"
#include "settings.h"
#include "survinfo.h"
#include "posimpexppars.h"
#include "envvars.h"
#include "oddirs.h"
#include "dirlist.h"

#include "uigeninput.h"
#include "uitable.h"
#include "uicombobox.h"
#include "uimsg.h"

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
	: uiDialog(p,uiDialog::Setup(nm,"Set User Settings value","0.2.1"))
        , issurvdefs_(FixedString(settskey)==sKeySurveyDefs())
	, grpfld_(0)
{
    setCurSetts();
    if ( issurvdefs_ )
    {
	setTitleText( "Set Survey default value" );
	setHelpKey( "0.2.8" );
    }
    else
    {
	BufferStringSet grps; getGrps( grps );
	grpfld_ = new uiGenInput( this, "Settings group",
				  StringListInpSpec(grps) );
	grpfld_->valuechanged.notify( mCB(this,uiSettings,grpChg) );
    }

    tbl_ = new uiTable( this, uiTable::Setup(10,2).manualresize(true),
				"Settings editor" );
    tbl_->setColumnLabel( 0, "Keyword" );
    tbl_->setColumnLabel( 1, "Value" );
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
	uiMSG().error( "Cannot write ", setts.name() );
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
// TODO: Move these keys to a header file in Basic
#define mIconsKey		"dTect.Icons"
#define mCBarKey		"dTect.ColorBar.show vertical"
#define mShowInlProgress	"dTect.Show inl progress"
#define mShowCrlProgress	"dTect.Show crl progress"
#define mTextureResFactor	"dTect.Default texture resolution factor"
#define mNoShading		"dTect.No shading"
#define mVolRenShading		"dTect.Use VolRen shading"

struct LooknFeelSettings
{
		LooknFeelSettings()
		    : iconsz(theiconsz < 0 ? uiObject::iconSize() : theiconsz)
		    , vertcoltab(true)
		    , showinlprogress(true)
		    , showcrlprogress(true)
		    , textureresfactor(0)
		    , noshading(false)
		    , volrenshading(false)		{}

    int		iconsz;
    bool	vertcoltab;
    bool	showinlprogress;
    bool	showcrlprogress;
    int		textureresfactor;
		  // -1: system default, 0 - standard, 1 - higher, 2 - highest
    bool	noshading;
    bool	volrenshading;
};


class uiSettingsGroup : public uiDlgGroup
{
public:
uiSettingsGroup( uiParent* p, const uiString& caption,
		 Settings& setts, LooknFeelSettings& lfsetts, bool& changed )
    : uiDlgGroup(p,caption)
    , setts_(setts)
    , lfsetts_(lfsetts)
    , changed_(changed)
{
}

const char* errMsg() const
{ return errmsg_.buf(); }

void updateSettings( bool oldval, bool newval, const char* key )
{
    if ( oldval != newval )
    {
	changed_ = true;
	setts_.setYN( key, newval );
    }
}

    BufferString	errmsg_;
    Settings&		setts_;
    LooknFeelSettings&	lfsetts_;
    bool&		changed_;
};


class uiGeneralSettingsGroup : public uiSettingsGroup
{
public:
uiGeneralSettingsGroup( uiParent* p, Settings& setts,
			LooknFeelSettings& lfsetts, bool& changed )
    : uiSettingsGroup(p,"General",setts,lfsetts,changed)
{
    iconszfld_ = new uiGenInput( this, "Icon Size",
				 IntInpSpec(lfsetts_.iconsz,10,64) );

    setts_.getYN( mCBarKey, lfsetts_.vertcoltab );
    colbarhvfld_ = new uiGenInput( this, "Color bar orientation",
		BoolInpSpec(lfsetts_.vertcoltab,"Vertical","Horizontal") );
    colbarhvfld_->attach( alignedBelow, iconszfld_ );

    setts_.getYN( mShowInlProgress, lfsetts_.showinlprogress );
    showinlprogressfld_ = new uiGenInput( this,
	    "Show progress when loading stored data on in-lines",
	    BoolInpSpec(lfsetts_.showinlprogress) );
    showinlprogressfld_->attach( alignedBelow, colbarhvfld_ );

    setts_.getYN( mShowCrlProgress, lfsetts_.showcrlprogress );
    showcrlprogressfld_ = new uiGenInput( this,
	    "Show progress when loading stored data on cross-lines",
	    BoolInpSpec(lfsetts_.showcrlprogress) );
    showcrlprogressfld_->attach( alignedBelow, showinlprogressfld_ );
}


bool acceptOK()
{
    const int newiconsz = iconszfld_->getIntValue();
    if ( newiconsz < 10 || newiconsz > 64 )
    {
	errmsg_.set( "Please specify an icon size in the range 10-64" );
	return false;
    }

    if ( newiconsz != lfsetts_.iconsz )
    {
	IOPar* iopar = setts_.subselect( mIconsKey );
	if ( !iopar ) iopar = new IOPar;
	iopar->set( "size", newiconsz );
	setts_.mergeComp( *iopar, mIconsKey );
	changed_ = true;
	delete iopar;
	theiconsz = newiconsz;
    }

    updateSettings( lfsetts_.vertcoltab, colbarhvfld_->getBoolValue(),
		    mCBarKey );

    updateSettings( lfsetts_.showinlprogress,
		    showinlprogressfld_->getBoolValue(),
		    mShowInlProgress );
    updateSettings( lfsetts_.showcrlprogress,
		    showcrlprogressfld_->getBoolValue(),
		    mShowCrlProgress );

    return true;
}

protected:

    uiGenInput*		iconszfld_;
    uiGenInput*		colbarhvfld_;
    uiGenInput*		showinlprogressfld_;
    uiGenInput*		showcrlprogressfld_;

};


class uiVisSettingsGroup : public uiSettingsGroup
{
public:
uiVisSettingsGroup( uiParent* p, Settings& setts,
			LooknFeelSettings& lfsetts, bool& changed )
    : uiSettingsGroup(p,"Visualisation",setts,lfsetts,changed)
{
    setts_.get( mTextureResFactor, lfsetts_.textureresfactor );
    textureresfactorfld_ = new uiLabeledComboBox( this,
		"Default texture resolution factor" );
    textureresfactorfld_->box()->addItem( "Standard" );
    textureresfactorfld_->box()->addItem( "Higher" );
    textureresfactorfld_->box()->addItem( "Highest" );

    int selection = 0;

    if ( lfsetts_.textureresfactor >= 0 && lfsetts_.textureresfactor <= 2 )
	    selection = lfsetts_.textureresfactor;

    // add the System default option if the environment variable is set
    const char* envvar = GetEnvVar( "OD_DEFAULT_TEXTURE_RESOLUTION_FACTOR" );
    if ( envvar && isdigit(*envvar) )
    {
	textureresfactorfld_->box()->addItem( "System default" );
	if ( lfsetts_.textureresfactor == -1 )
	    selection = 3;
    }

    textureresfactorfld_->box()->setCurrentItem( selection );

    setts_.getYN( mNoShading, lfsetts_.noshading );
    useshadingfld_ = new uiGenInput( this, "Use OpenGL shading when available",
				    BoolInpSpec(!lfsetts_.noshading) );
    useshadingfld_->attach( alignedBelow, textureresfactorfld_ );
    useshadingfld_->valuechanged.notify(
			mCB(this,uiVisSettingsGroup,shadingChange) );
    setts_.getYN( mVolRenShading, lfsetts_.volrenshading );
    volrenshadingfld_ = new uiGenInput( this, "Also for volume rendering?",
				    BoolInpSpec(lfsetts_.volrenshading) );
    volrenshadingfld_->attach( alignedBelow, useshadingfld_ );

    shadingChange(0);
}


bool acceptOK()
{
    const bool newnoshading = !useshadingfld_->getBoolValue();
    updateSettings( lfsetts_.noshading, newnoshading, mNoShading );

    bool newvolrenshading = !newnoshading;
    if ( newvolrenshading )
	newvolrenshading = volrenshadingfld_->getBoolValue();
    updateSettings( lfsetts_.volrenshading, newvolrenshading, mVolRenShading );

    bool textureresfacchanged = false;
    // track this change separately as this will be applied with immediate
    // effect, unlike other settings
    int val = ( textureresfactorfld_->box()->currentItem() == 3 ) ? -1 :
		textureresfactorfld_->box()->currentItem();
    if ( lfsetts_.textureresfactor != val )
    {
	textureresfacchanged = true;
	setts_.set( mTextureResFactor, val );
    }

    if ( textureresfacchanged ) changed_ = true;

    return true;
}

protected:

void shadingChange( CallBacker* )
{
    volrenshadingfld_->display( useshadingfld_->getBoolValue() );
}

    uiLabeledComboBox*	textureresfactorfld_;
    uiGenInput*		useshadingfld_;
    uiGenInput*		volrenshadingfld_;

};



uiLooknFeelSettings::uiLooknFeelSettings( uiParent* p )
    : uiTabStackDlg(p,uiDialog::Setup("Look and Feel Settings",mNoDlgTitle,
				      "0.2.3"))
    , setts_(Settings::common())
    , lfsetts_(*new LooknFeelSettings)
    , changed_(false)
{
    addGroup( new uiGeneralSettingsGroup(tabstack_->tabGroup(),
					 setts_,lfsetts_,changed_) );
    addGroup( new uiVisSettingsGroup(tabstack_->tabGroup(),
					 setts_,lfsetts_,changed_) );
}


uiLooknFeelSettings::~uiLooknFeelSettings()
{
    delete &lfsetts_;
}


bool uiLooknFeelSettings::acceptOK( CallBacker* cb )
{
    if ( !uiTabStackDlg::acceptOK(cb) )
	return false;

    if ( changed_ && !setts_.write() )
    {
	changed_ = false;
	uiMSG().error( "Cannot write settings" );
	return false;
    }

    return true;
}

