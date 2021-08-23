/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Jan 2012
________________________________________________________________________

-*/

#include "uistratlaymodtools.h"

#include "keystrs.h"
#include "propertyref.h"
#include "stratlevel.h"
#include "stratlayermodel.h"
#include "uicombobox.h"
#include "uigeninput.h"
#include "uilabel.h"
#include "uispinbox.h"
#include "uitoolbutton.h"
#include "od_helpids.h"

static int defNrModels() { return 100; }

const char* uiStratGenDescTools::sKeyNrModels()
{ return "Nr models"; }

const char* uiStratLayModEditTools::sKeyDisplayedProp()
{ return "Displayed property"; }

const char* uiStratLayModEditTools::sKeyDecimation()
{ return "Decimation"; }

const char* uiStratLayModEditTools::sKeySelectedLevel()
{ return "Selected level"; }

const char* uiStratLayModEditTools::sKeySelectedContent()
{return "Selected content";}

const char* uiStratLayModEditTools::sKeyZoomToggle()
{ return "Allow zoom"; }

const char* uiStratLayModEditTools::sKeyDispLith()
{ return "Display lithology"; }


const char* uiStratLayModEditTools::sKeyShowFlattened()
{ return "Display flattened";}


uiStratGenDescTools::uiStratGenDescTools( uiParent* p )
    : uiGroup(p,"Gen Desc Tools")
    , openReq(this)
    , saveReq(this)
    , propEdReq(this)
    , genReq(this)
    , nrModelsChanged(this)
{
    uiGroup* leftgrp = new uiGroup( this, "Left group" );
    uiToolButton* opentb = new uiToolButton( leftgrp, "open",
				tr("Open stored generation description"),
				mCB(this,uiStratGenDescTools,openCB) );
    savetb_ = new uiToolButton( leftgrp, "save",
				tr("Save generation description"),
				mCB(this,uiStratGenDescTools,saveCB) );
    savetb_->attach( rightOf, opentb );
    uiToolButton* proptb = new uiToolButton( leftgrp, "defprops",
				tr("Select layer properties"),
				mCB(this,uiStratGenDescTools,propEdCB) );
    proptb->attach( rightOf, savetb_ );

    uiGroup* rightgrp = new uiGroup( this, "Right group" );
    const CallBack gocb( mCB(this,uiStratGenDescTools,genCB) );
    nrmodlsfld_ = new uiSpinBox( rightgrp );
    nrmodlsfld_->setInterval( Interval<int>(1,mUdf(int)) );
    nrmodlsfld_->setValue( defNrModels() );
    nrmodlsfld_->setFocusChangeTrigger( false );
    nrmodlsfld_->setStretch( 0, 0 );
    nrmodlsfld_->setToolTip( tr("Number of models to generate") );
    nrmodlsfld_->valueChanging.notify(
	    mCB(this,uiStratGenDescTools,nrModelsChangedCB) );
    nrmodlsfld_->valueChanged.notify( gocb );
    uiToolButton* gotb = new uiToolButton( rightgrp, "go",
				tr("Generate this amount of models"), gocb );
    nrmodlsfld_->attach( leftOf, gotb );
    rightgrp->attach( ensureRightOf, leftgrp );
}


void uiStratGenDescTools::setNrModels( int nrmodels )
{
    NotifyStopper notstop( nrmodlsfld_->valueChanged );
    nrmodlsfld_->setValue( nrmodels );
}


int uiStratGenDescTools::nrModels() const
{
    return nrmodlsfld_->getIntValue();
}


void uiStratGenDescTools::enableSave( bool yn )
{
    savetb_->setSensitive( yn );
}

int uiStratGenDescTools::getNrModelsFromPar( const IOPar& par ) const
{
    int nrmodels = -1;
    par.get( sKeyNrModels(), nrmodels );
    return nrmodels;
}


void uiStratGenDescTools::fillPar( IOPar& par ) const
{
    par.set( sKeyNrModels(), nrModels() );
}


bool uiStratGenDescTools::usePar( const IOPar& par )
{
    int nrmodels = getNrModelsFromPar( par );
    if ( nrmodels<0 )
	nrmodels = defNrModels();

    nrmodlsfld_->setValue( nrmodels );
    return true;
}


void uiStratGenDescTools::nrModelsChangedCB( CallBacker* )
{
    nrModelsChanged.trigger();
}


uiStratLayModEditTools::uiStratLayModEditTools( uiParent* p )
    : uiGroup(p,"Lay Mod Edit Tools")
    , selPropChg(this)
    , selLevelChg(this)
    , selContentChg(this)
    , dispEachChg(this)
    , dispZoomedChg(this)
    , dispLithChg(this)
    , flattenChg(this)
    , mkSynthChg(this)
    , allownoprop_(false)
{
    uiGroup* leftgrp = new uiGroup( this, "Left group" );
    propfld_ = new uiComboBox( leftgrp, "Display property" );
    propfld_->setToolTip( tr("Displayed property") );
    propfld_->selectionChanged.notify(
				mCB(this,uiStratLayModEditTools,selPropCB) );

    lvlfld_ = new uiComboBox( leftgrp, "Level" );
    lvlfld_->setToolTip( tr("Selected stratigraphic level") );
    lvlfld_->attach( rightOf, propfld_ );
    lvlfld_->selectionChanged.notify(
				mCB(this,uiStratLayModEditTools,selLevelCB) );

    contfld_ = new uiComboBox( leftgrp, "Content" );
    contfld_->setToolTip( tr("Marked content") );
    contfld_->attach( rightOf, lvlfld_ );
    contfld_->setHSzPol( uiObject::Small );
    contfld_->selectionChanged.notify(
				mCB(this,uiStratLayModEditTools,selContentCB));

    eachlbl_ = new uiLabel( leftgrp, tr("each") );
    eachlbl_->attach( rightOf, contfld_ );
    eachfld_ = new uiSpinBox( leftgrp, 0, "DispEach" );
    eachfld_->setInterval( 1, 100000 );
    eachfld_->attach( rightOf, eachlbl_ );
    eachfld_->valueChanging.notify(
				mCB(this,uiStratLayModEditTools,dispEachCB) );

    uiGroup* rightgrp = new uiGroup( this, "Right group" );
    mksynthtb_ = new uiToolButton( rightgrp, "autogensynth",
			tr("Automatically create synthetics when on"),
			mCB(this,uiStratLayModEditTools,mkSynthCB) );
    mksynthtb_->setToggleButton( true );
    mksynthtb_->setOn( true );
    flattenedtb_ = new uiToolButton( rightgrp, "flattenseis",
			tr("Show flattened when on"),
			mCB(this,uiStratLayModEditTools,showFlatCB) );
    flattenedtb_->setToggleButton( true );
    flattenedtb_->setOn( false );
    flattenedtb_->attach( leftOf, mksynthtb_ );
    lithtb_ = new uiToolButton( rightgrp, "lithologies",
			tr("Show lithology colors when on"),
			mCB(this,uiStratLayModEditTools,dispLithCB) );
    lithtb_->setToggleButton( true );
    lithtb_->setOn( true );
    lithtb_->attach( leftOf, flattenedtb_ );
    zoomtb_ = new uiToolButton( rightgrp, "toggzooming",
			tr("Do not zoom into models when on"),
			mCB(this,uiStratLayModEditTools,dispZoomedCB) );
    zoomtb_->setToggleButton( true );
    zoomtb_->setOn( false );
    zoomtb_->attach( leftOf, lithtb_ );
    rightgrp->attach( rightTo, leftgrp );
    rightgrp->attach( rightBorder );
}


void uiStratLayModEditTools::setNoDispEachFld()
{
    eachlbl_->display( false ); eachfld_->display( false );
    eachfld_ = nullptr;
}


static void setFldNms( uiComboBox* cb, const BufferStringSet& nms, bool wnone,
			bool wall, int def )
{
    const BufferString selnm( cb->text() );
    cb->setEmpty();
    if ( wnone )
	cb->addItem( toUiString("---") );
    if ( nms.isEmpty() ) return;

    cb->addItems( nms );
    if ( wall )
	cb->addItem( uiStrings::sAll() );

    if ( wnone ) def++;
    if ( !selnm.isEmpty() )
    {
	def = cb->indexOf( selnm );
	if ( def < 0 )
	    def = 0;
    }
    if ( def >= cb->size() ) def = cb->size() - 1;
    if ( def >= 0 )
	cb->setCurrentItem( def );
}


void uiStratLayModEditTools::setProps( const BufferStringSet& nms )
{ setFldNms( propfld_, nms, allownoprop_, false, 0 ); }
void uiStratLayModEditTools::setLevelNames( const BufferStringSet& nms )
{ setFldNms( lvlfld_, nms, true, false, 0 ); }
void uiStratLayModEditTools::setContentNames( const BufferStringSet& nms )
{ setFldNms( contfld_, nms, true, true, -1 ); }


const char* uiStratLayModEditTools::selProp() const
{
    return propfld_->isEmpty() ? nullptr : propfld_->text();
}


int uiStratLayModEditTools::selPropIdx() const
{
    if ( propfld_->isEmpty() )
	return -1;
    const int selidx = propfld_->getIntValue();
    if ( selidx < 0 || (allownoprop_ && selidx == 0) )
	return -1;

    int propidx = selidx;
    if ( !allownoprop_ )
	propidx++;
    return propidx;
}


const char* uiStratLayModEditTools::selLevel() const
{
    return lvlfld_->isEmpty() ? nullptr : lvlfld_->text();
}


int uiStratLayModEditTools::selLevelIdx() const
{
    return lvlfld_->isEmpty() ? -1 : lvlfld_->currentItem() - 1;
}


const char* uiStratLayModEditTools::selContent() const
{
    return contfld_->isEmpty() ? nullptr : contfld_->text();
}


const Strat::Level* uiStratLayModEditTools::selStratLevel() const
{
    const int lvlidx = selLevelIdx();
    return lvlidx < 0 ? nullptr : Strat::LVLS().levels()[lvlidx];
}


OD::Color uiStratLayModEditTools::selLevelColor() const
{
    const Strat::Level* lvl = selStratLevel();
    return lvl ? lvl->color() : OD::Color::NoColor();
}


int uiStratLayModEditTools::dispEach() const
{
    return eachfld_ ? eachfld_->getIntValue() : 1;
}


bool uiStratLayModEditTools::dispZoomed() const
{
    return !zoomtb_->isOn();
}


bool uiStratLayModEditTools::dispLith() const
{
    return lithtb_->isOn();
}


bool uiStratLayModEditTools::showFlattened() const
{
    return flattenedtb_->isOn() && (bool)selStratLevel();
}


bool uiStratLayModEditTools::mkSynthetics() const
{
    return mksynthtb_->isOn();
}


void uiStratLayModEditTools::setSelProp( const char* sel )
{
    propfld_->setText( sel );
}


void uiStratLayModEditTools::setSelLevel( const char* sel )
{
    lvlfld_->setText( sel );
}


void uiStratLayModEditTools::setSelContent( const char* sel )
{
    contfld_->setText( sel );
}


void uiStratLayModEditTools::setDispEach( int nr )
{
    if ( !eachfld_ )
	return;

    NotifyStopper ns( eachfld_->valueChanging );
    eachfld_->setValue( nr );
}


void uiStratLayModEditTools::setDispZoomed( bool yn )
{
    zoomtb_->setOn( !yn );
}


void uiStratLayModEditTools::setDispLith( bool yn )
{
    lithtb_->setOn( yn );
}


void uiStratLayModEditTools::setFlatTBSensitive( bool yn )
{
    flattenedtb_->setSensitive( yn );
}


void uiStratLayModEditTools::setShowFlattened( bool yn )
{
    flattenedtb_->setOn( yn );
}


void uiStratLayModEditTools::setMkSynthetics( bool yn )
{
    mksynthtb_->setOn( yn );
}

#define mGetProp( func, key ) \
if ( func ) \
par.set( key, func )

void uiStratLayModEditTools::fillPar( IOPar& par ) const
{
    mGetProp( selProp(), sKeyDisplayedProp() );
    par.set( sKeyDecimation(), dispEach() );

    mGetProp( selLevel(), sKeySelectedLevel() );
    mGetProp( selContent(), sKeySelectedContent() );

    par.setYN( sKeyZoomToggle(), dispZoomed() );
    par.setYN( sKeyDispLith(), dispLith() );
    par.setYN( sKeyShowFlattened(), showFlattened() );
}

#define mSetProp( fld, key ) \
{ \
    FixedString selprop = par[key()]; \
    if ( selprop && fld->isPresent( selprop ) ) \
    { \
	fld->setCurrentItem( selprop ); \
	fld->selectionChanged.trigger(); \
    } \
}

#define mSetYN( func, key, cb ) \
{ \
    bool yn; \
    if ( par.getYN( key(), yn ) ) \
    { \
	func( yn ); \
	cb( nullptr ); \
    } \
}


bool uiStratLayModEditTools::usePar( const IOPar& par )
{
    NotifyStopper stopselprop( selPropChg );
    NotifyStopper stoplvlchg( selLevelChg );
    NotifyStopper stopcontchg( selContentChg );
    NotifyStopper stopeachchg( dispEachChg );
    NotifyStopper stopzoomchg( dispZoomedChg );
    NotifyStopper stoplithchg( dispLithChg );
    NotifyStopper stopflatchg( flattenChg );
    NotifyStopper stopsynthchg( mkSynthChg );

    mSetProp( propfld_, sKeyDisplayedProp );
    mSetProp( lvlfld_, sKeySelectedLevel );
    mSetProp( contfld_, sKeySelectedContent );

    mSetYN( setDispZoomed, sKeyZoomToggle, dispZoomedCB );
    mSetYN( setDispLith, sKeyDispLith, dispLithCB );
    mSetYN( setShowFlattened, sKeyShowFlattened, showFlatCB );

    return true;
}


//-----------------------------------------------------------------------------

#define mCreatePropSelFld( propnm, txt, prop, prevbox ) \
    auto* lblbox##propnm = new uiLabeledComboBox( this, txt ); \
    propnm##fld_ = lblbox##propnm->box(); \
    const PropertyRefSelection subsel##propnm = proprefsel.subselect( prop );\
    for ( int idx=0; idx<subsel##propnm.size(); idx++ )\
	if ( subsel##propnm[idx] )\
	    propnm##fld_->addItem( toUiString(subsel##propnm[idx]->name()) );\
    if ( prevbox )\
	lblbox##propnm->attach( alignedBelow, (uiObject*)prevbox );


uiStratLayModFRPropSelector::uiStratLayModFRPropSelector( uiParent* p,
					const PropertyRefSelection& proprefsel,
				const uiStratLayModFRPropSelector::Setup& set )
	: uiDialog(p,uiDialog::Setup(tr("Property Selector"),
				     tr("There are multiple properties "
					"referenced with the same type. \n"
					"Please specify which one to use as: "),
		     mODHelpKey(mStratSynthLayerModFRPPropSelectorHelpID) ) )
{
    mCreatePropSelFld(den, tr("Reference for Density"), Mnemonic::Den,
			nullptr);
    mCreatePropSelFld(vp, tr("Reference for Vp"), Mnemonic::Vel, lblboxden);
    uiLabeledComboBox* prevfld = lblboxvp;
    ePROPS().ensureHasElasticProps( set.withswave_ );
    if ( set.withswave_ )
    {
	mCreatePropSelFld(vs, tr("Reference for Vs"), Mnemonic::Vel, prevfld );
	prevfld = lblboxvs;
	const PropertyRef* svelpr =
				proprefsel.getByMnemonic( Mnemonic::defSVEL() );
	if ( svelpr )
	    setVSProp( svelpr->name() );
    }

    if ( set.withinitsat_ )
    {
	mCreatePropSelFld( initialsat, tr("Reference for Initial Saturation"),
			   Mnemonic::Volum, prevfld );
	prevfld = lblboxinitialsat;
	const PropertyRef* swpr = proprefsel.getByMnemonic( Mnemonic::defSW() );
	if ( swpr )
	    setInitialSatProp( swpr->name() );
    }

    if ( set.withfinalsat_ )
    {
	mCreatePropSelFld( finalsat, tr("Reference for Final Saturation"),
			   Mnemonic::Volum, prevfld );
	prevfld = lblboxfinalsat;
	const PropertyRef* swpr = proprefsel.getByMnemonic( Mnemonic::defSW() );
	if ( swpr )
	    setFinalSatProp( swpr->name() );
    }

    if ( set.withpor_ )
    {
	mCreatePropSelFld( porosity, tr("Reference for Porosity"),
			   Mnemonic::Volum, prevfld );
	const PropertyRef* porpr = proprefsel.getByMnemonic(Mnemonic::defPHI());
	if ( porpr )
	    setPorProp( porpr->name() );
	else
	{
	    const MnemonicSelection mnsel = MnemonicSelection::getAllPorosity();
	    if ( !mnsel.isEmpty() )
	    {
		porpr = proprefsel.getByMnemonic( *mnsel.first() );
		if ( porpr )
		    setPorProp( porpr->name() );
	    }
	}
    }

    const PropertyRef* denpr = proprefsel.getByMnemonic( Mnemonic::defDEN() );
    if ( denpr )
	setDenProp( denpr->name() );
    else
	errmsg_ = tr( "No reference to density found" );

    const PropertyRef* pvelpr =
			    proprefsel.getByMnemonic( Mnemonic::defPVEL() );
    if ( pvelpr )
	setVPProp( pvelpr->name() );
    else
	errmsg_ = tr( "No reference to P wave velocity found" );
}


bool uiStratLayModFRPropSelector::isOK() const
{
    if ( !errmsg_.isEmpty() )
	return false;

    if ( denfld_->isEmpty() || vpfld_->isEmpty() ||
	 (vsfld_ && vsfld_->isEmpty()) )
	return false;

    if ( vsfld_ &&
	 BufferString(vpfld_->text()) == BufferString(vsfld_->text()) )
    {
	uiStratLayModFRPropSelector& modsel =
			const_cast<uiStratLayModFRPropSelector&>( *this );
	modsel.errmsg_ = tr( "Selected property for P wave velocity and "
			     "S wave velocity should be different." );
    }

    if ( (porosityfld_ && porosityfld_->isEmpty()) ||
	 (initialsatfld_ && initialsatfld_->isEmpty()) ||
	 (finalsatfld_ && finalsatfld_->isEmpty()) )
	return false;

    return errmsg_.isEmpty();
}


#define mSetPropFromNm(propfunc,propnm) \
    void uiStratLayModFRPropSelector::set##propfunc##Prop( const char* nm ) \
    { \
	if ( !propnm##fld_ ) \
	    return; \
	\
	if ( propnm##fld_->isPresent(nm) ) \
	    propnm##fld_->setCurrentItem( nm ); \
	else \
	    propnm##fld_->setCurrentItem(0); \
    } \

mSetPropFromNm(Den,den);
mSetPropFromNm(VP,vp);
mSetPropFromNm(VS,vs);
mSetPropFromNm(Por,porosity);
mSetPropFromNm(InitialSat,initialsat);
mSetPropFromNm(FinalSat,finalsat);


bool uiStratLayModFRPropSelector::needsDisplay() const
{
    if ( denfld_->size() > 1 )
	return true;

    if ( vpfld_->size()>1 && (vsfld_ && vsfld_->size()>1) &&
	 BufferString(getSelVPName()) == BufferString(getSelVSName()) )
	return true;

    if ( (porosityfld_ && porosityfld_->size() > 1) ||
	 (initialsatfld_ && initialsatfld_->size() > 1) ||
	 (finalsatfld_ && finalsatfld_->size() > 1) )
	return true;

    return !isOK();
}


bool uiStratLayModFRPropSelector::acceptOK( CallBacker* )
{
    return isOK();
}


const char* uiStratLayModFRPropSelector::getSelVPName() const
{ return vpfld_->text(); }

const char* uiStratLayModFRPropSelector::getSelVSName() const
{ return vsfld_ ? vsfld_->text() : nullptr; }

const char* uiStratLayModFRPropSelector::getSelDenName() const
{ return denfld_->text(); }

const char* uiStratLayModFRPropSelector::getSelPorName() const
{ return porosityfld_ ? porosityfld_->text() : nullptr; }

const char* uiStratLayModFRPropSelector::getSelInitialSatName() const
{ return initialsatfld_ ? initialsatfld_->text() : nullptr; }

const char* uiStratLayModFRPropSelector::getSelFinalSatName() const
{ return finalsatfld_ ? finalsatfld_->text() : nullptr; }
