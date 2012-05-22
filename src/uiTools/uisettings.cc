/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:          November 2001
________________________________________________________________________

-*/
static const char* rcsID mUnusedVar = "$Id: uisettings.cc,v 1.44 2012-05-22 11:56:28 cvsbert Exp $";

#include "uisettings.h"

#include "ptrman.h"
#include "settings.h"
#include "survinfo.h"
#include "posimpexppars.h"
#include "envvars.h"
#include "oddirs.h"
#include "dirlist.h"

#include "uibutton.h"
#include "uigeninput.h"
#include "uilistbox.h"
#include "uicombobox.h"
#include "uimsg.h"
#include "uiselsimple.h"

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
	char* dotptr = strrchr( fnm.buf(), '.' );
	if ( (needdot && !dotptr) || (!needdot && dotptr) )
	    continue;
	if ( dotptr )
	{
	    BufferString usr( dotptr + 1 );
	    if ( usr != dtectuser )
		continue;
	    *dotptr = '\0';
	}
	const char* underscoreptr = strchr( fnm.buf(), '_' );
	if ( !underscoreptr || !*underscoreptr )
	    continue;
	grps.add( underscoreptr + 1 );
    }
}


uiSettings::uiSettings( uiParent* p, const char* nm, const char* settskey )
	: uiDialog(p,uiDialog::Setup(nm,"Set User Settings value","0.2.1"))
        , issurvdefs_(settskey && !strcmp(settskey,sKeySurveyDefs()))
	, setts_(&SI().getPars())
    	, grpfld_(0)
{
    if ( issurvdefs_ )
    {
	setTitleText( "Set Survey default value" );
	setHelpID( "0.2.8" );
    }
    else
    {
	grpChg( 0 );
	BufferStringSet grps; getGrps( grps );
	grpfld_ = new uiGenInput( this, "Settings group",
				  StringListInpSpec(grps) );
	grpfld_->valuechanged.notify( mCB(this,uiSettings,grpChg) );
    }
    keyfld_ = new uiGenInput( this, "Settings keyword", StringInpSpec() );
    if ( grpfld_ )
	keyfld_->attach( alignedBelow, grpfld_ );
    uiButton* pb = new uiPushButton( this, "&Select existing",
	    			     mCB(this,uiSettings,selPush), false );
    pb->setName( "Select Keyword" );
    pb->attach( rightOf, keyfld_ );

    valfld_ = new uiGenInput( this, "Value", StringInpSpec() );
    valfld_->attach( alignedBelow, keyfld_ );
}


uiSettings::~uiSettings()
{
}


void uiSettings::grpChg( CallBacker* )
{
    const BufferString grp( grpfld_ ? grpfld_->text() : sKeyCommon );
    if ( grp == sKeyCommon )
	setts_ = &Settings::common();
    else
	setts_ = &Settings::fetch( grp );
}


void uiSettings::selPush( CallBacker* )
{
    BufferStringSet keys;
    for ( int idx=0; idx<setts_->size(); idx++ )
	keys.add( setts_->getKey(idx) );
    keys.sort();
    uiSelectFromList::Setup listsetup( "Setting selection", keys );
    listsetup.dlgtitle( keyfld_->text() );
    uiSelectFromList dlg( this, listsetup );
    dlg.selFld()->setHSzPol( uiObject::Wide );
    if ( !dlg.go() ) return;
    const int selidx = dlg.selection();
    if ( selidx < 0 ) return;

    const char* key = keys.get( selidx ).buf();
    keyfld_->setText( key );
    valfld_->setText( setts_->find(key) );
}


bool uiSettings::acceptOK( CallBacker* )
{
    const BufferString ky = keyfld_->text();
    if ( ky.isEmpty() )
    {
	uiMSG().error( "Please enter a keyword to set" );
	return false;
    }

    const BufferString val( valfld_->text() );
    if ( val.isEmpty() )
	setts_->removeWithKey( ky );
    else
	setts_->set( ky, valfld_->text() );

    if ( issurvdefs_ )
    {
	SI().savePars();
	PosImpExpPars::refresh();
    }
    else
    {
	mDynamicCastGet(Settings*,setts,setts_)
	if ( !setts->write() )
	{
	    uiMSG().error( "Cannot write user settings" );
	    return false;
	}
    }

    return true;
}


static int theiconsz = -1;
// TODO: Move these keys to a header file in Basic
#define mIconsKey		"dTect.Icons"
#define mCBarKey		"dTect.ColorBar.show vertical"
#define mShowInlProgress	"dTect.Show inl progress"
#define mShowCrlProgress	"dTect.Show crl progress"
#define mShowWheels		"dTect.Show wheels"
#define mTextureResFactor	"dTect.Default texture resolution factor"
#define mNoShading		"dTect.No shading"
#define mVolRenShading		"dTect.Use VolRen shading"

struct LooknFeelSettings
{
    		LooknFeelSettings()
		    : iconsz(theiconsz < 0 ? uiObject::iconSize() : theiconsz)
		    , isvert(true)
		    , showwheels(true)
		    , showinlprogress(true)
		    , showcrlprogress(true)
		    , textureresfactor(0)
		    , noshading(false)
		    , volrenshading(false)		{}

    int		iconsz;
    bool	isvert;
    bool	showwheels;
    bool	showinlprogress;
    bool	showcrlprogress;
    int		textureresfactor;	
    		  // -1: system default, 0 - standard, 1 - higher, 2 - highest
    bool	noshading;
    bool	volrenshading;
};



uiLooknFeelSettings::uiLooknFeelSettings( uiParent* p, const char* nm )
	: uiDialog(p,uiDialog::Setup(nm,"Look and Feel Settings","0.2.3"))
	, setts_(Settings::common())
    	, lfsetts_(*new LooknFeelSettings)
	, changed_(false)
{
    iconszfld_ = new uiGenInput( this, "Icon Size",
	    			 IntInpSpec(lfsetts_.iconsz) );

    setts_.getYN( mCBarKey, lfsetts_.isvert );
    colbarhvfld_ = new uiGenInput( this, "Color bar orientation",
			BoolInpSpec(lfsetts_.isvert,"Vertical","Horizontal") );
    colbarhvfld_->attach( alignedBelow, iconszfld_ );

    setts_.getYN( mShowInlProgress, lfsetts_.showinlprogress );
    showinlprogressfld_ = new uiGenInput( this,
	    "Show progress when loading stored data on inlines",
	    BoolInpSpec(lfsetts_.showinlprogress) );
    showinlprogressfld_->attach( alignedBelow, colbarhvfld_ );

    setts_.getYN( mShowCrlProgress, lfsetts_.showcrlprogress );
    showcrlprogressfld_ = new uiGenInput( this,
	    "Show progress when loading stored data on crosslines",
	    BoolInpSpec(lfsetts_.showcrlprogress) );
    showcrlprogressfld_->attach( alignedBelow, showinlprogressfld_ );

    setts_.getYN( mShowWheels, lfsetts_.showwheels );
    showwheelsfld_ = new uiGenInput( this, "Show Zoom/Rotation tools",
	    			    BoolInpSpec(lfsetts_.showwheels) );
    showwheelsfld_->attach( alignedBelow, showcrlprogressfld_ );

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
    textureresfactorfld_->attach( alignedBelow, showwheelsfld_ );
	
    setts_.getYN( mNoShading, lfsetts_.noshading );
    useshadingfld_ = new uiGenInput( this, "Use OpenGL shading when available",
				    BoolInpSpec(!lfsetts_.noshading) );
    useshadingfld_->attach( alignedBelow, textureresfactorfld_ );
    useshadingfld_->valuechanged.notify(
	    		mCB(this,uiLooknFeelSettings,shadingChange) );
    setts_.getYN( mVolRenShading, lfsetts_.volrenshading );
    volrenshadingfld_ = new uiGenInput( this, "Also for volume rendering?",
				    BoolInpSpec(lfsetts_.volrenshading) );
    volrenshadingfld_->attach( alignedBelow, useshadingfld_ );

    shadingChange(0);
}


uiLooknFeelSettings::~uiLooknFeelSettings()
{
    delete &lfsetts_;
}


void uiLooknFeelSettings::shadingChange( CallBacker* )
{
    volrenshadingfld_->display( useshadingfld_->getBoolValue() );
}


void uiLooknFeelSettings::updateSettings( bool oldval, bool newval,
					  const char* key )
{
    if ( oldval != newval )
    {
	changed_ = true;
	setts_.setYN( key, newval );
    }
}


bool uiLooknFeelSettings::acceptOK( CallBacker* )
{
    const int newiconsz = iconszfld_->getIntValue();
    if ( newiconsz < 10 || newiconsz > 64 )
    {
	uiMSG().setNextCaption( "Yeah right" );
	uiMSG().error( "Please specify an icon size in the range 10-64" );
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

    updateSettings( lfsetts_.isvert, colbarhvfld_->getBoolValue(), mCBarKey );
    const bool newnoshading = !useshadingfld_->getBoolValue();
    updateSettings( lfsetts_.noshading, newnoshading, mNoShading );

    bool newvolrenshading = !newnoshading;
    if ( newvolrenshading )
	newvolrenshading = volrenshadingfld_->getBoolValue();
    updateSettings( lfsetts_.volrenshading, newvolrenshading, mVolRenShading );

    updateSettings( lfsetts_.showwheels, showwheelsfld_->getBoolValue(),
	    	    mShowWheels );
    updateSettings( lfsetts_.showinlprogress,
		    showinlprogressfld_->getBoolValue(),
	    	    mShowInlProgress );
    updateSettings( lfsetts_.showcrlprogress,
		    showcrlprogressfld_->getBoolValue(),
	    	    mShowCrlProgress );

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

    if ( ( changed_ || textureresfacchanged ) && !setts_.write() )
    {
	changed_ = false;
	uiMSG().error( "Cannot write settings" );
	return false;
    }

    return true;
}

