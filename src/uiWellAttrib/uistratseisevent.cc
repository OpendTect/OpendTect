/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uistratseisevent.h"

#include "uibutton.h"
#include "uigeninput.h"
#include "uiioobjsel.h"
#include "uilabel.h"
#include "uistratlvlsel.h"
#include "uimsg.h"
#include "uistrings.h"
#include "emsurfacetr.h"
#include "emioobjinfo.h"
#include "stratlevel.h"
#include "survinfo.h"
#include "valseriesevent.h"

#define mToSecFactorF	0.001f


// uiStratLevelHorSel

uiStratLevelHorSel::uiStratLevelHorSel( uiParent* p,
					const Strat::LevelID& deflvlid )
    : uiGroup(p,"Strat Level With Horizon Group")
    , lvlid_(deflvlid)
    , levelSel(this)
    , horSel(this)
{
    lvlsel_ = new uiStratLevelSel( this, false, tr("Reference level"));

    uiIOObjSel::Setup hsu( tr("Horizon at that level") );
    horsel3d_ = new uiIOObjSel( this, mIOObjContext(EMHorizon3D), hsu );
    horsel3d_->attach( alignedBelow, lvlsel_ );
    if ( SI().has2D() )
    {
	horsel2d_ = new uiIOObjSel( this, mIOObjContext(EMHorizon2D), hsu );
	horsel2d_->attach( alignedBelow, lvlsel_ );
    }

    setHAlignObj( lvlsel_ );
    mAttachCB( postFinalize(), uiStratLevelHorSel::initGrp );
}


uiStratLevelHorSel::~uiStratLevelHorSel()
{
    detachAllNotifiers();
}


void uiStratLevelHorSel::initGrp( CallBacker* )
{
    if ( lvlid_.isValid() )
    {
	lvlsel_->setID( lvlid_ );
	setHorFromLvl();
    }

    if ( horsel2d_ )
	set2D( is2d_ );

    mAttachCB( lvlsel_->selChange, uiStratLevelHorSel::lvlSelCB );
    mAttachCB( horsel3d_->selectionDone, uiStratLevelHorSel::horSelCB );
    if ( horsel2d_ )
	mAttachCB( horsel2d_->selectionDone, uiStratLevelHorSel::horSelCB );
}


void uiStratLevelHorSel::setHorFromLvl()
{
    const Strat::LevelID lvlid = levelID();
    if ( !lvlid.isValid() )
	return;

    TypeSet<MultiID> dbkys;
    EM::IOObjInfo::getTiedToLevelID( lvlid, dbkys, false );
    if ( dbkys.isEmpty() )
	return;

    const MultiID curky = horsel3d_->key( true );
    if ( !dbkys.isPresent(curky) )
	horsel3d_->setInput( dbkys.first() );
}


void uiStratLevelHorSel::set2D( bool yn )
{
    is2d_ = yn && horsel2d_;
    horsel3d_->display( !is2d_ );
    if ( horsel2d_ )
	horsel2d_->display( is2d_ );
}


Strat::LevelID uiStratLevelHorSel::levelID() const
{
    return lvlsel_->getID();
}


MultiID uiStratLevelHorSel::horID() const
{
    return (is2D() ? horsel2d_ : horsel3d_)->key();
}


void uiStratLevelHorSel::lvlSelCB( CallBacker* )
{
    setHorFromLvl();
    levelSel.trigger();
}


void uiStratLevelHorSel::horSelCB( CallBacker* )
{
    horSel.trigger();
}


// uiStratSeisEvent::Setup

uiStratSeisEvent::Setup::Setup( bool wew )
    : sellevel_(true)
    , withextrwin_(wew)
    , allowlayerbased_(false)
{
}


uiStratSeisEvent::Setup::~Setup()
{
}


// uiStratSeisEvent

uiStratSeisEvent::uiStratSeisEvent( uiParent* p, const Setup& su )
    : uiGroup(p,"Strat Seis Event Specification Group")
    , setup_(su)
    , evtype_( VSEvent::TypeDef() )
    , anyChange(this)
{
    if ( setup_.sellevel_ )
	levelfld_ = new uiStratLevelSel( this, false, tr("Reference level" ) );

    evtype_.remove( evtype_.getKeyForIndex(0) );
    evfld_ = new uiGenInput( this, tr("Snap synthetics to event"),
				StringListInpSpec(evtype_) );
    evfld_->setWithCheck( true );
    evfld_->setValue( 1 );
    if ( levelfld_ )
	evfld_->attach( alignedBelow, levelfld_ );

    evfld_->setElemSzPol( uiObject::Medium );
    setHAlignObj( evfld_ );
    snapoffsfld_ = new uiGenInput( this, tr("Offset (ms)"), FloatInpSpec(0) );
    snapoffsfld_->attach( rightOf, evfld_ );
    snapoffsfld_->setElemSzPol( uiObject::Small );
    snapoffsfld_->setSensitive( false );

    if ( setup_.withextrwin_ )
    {
	const float defstep = SI().zIsTime() ? SI().zStep()/mToSecFactorF : 4;
	extrwinfld_ = new uiGenInput( this, tr("Extraction window %1")
					  .arg(SI().getZUnitString()),
				  FloatInpIntervalSpec(Interval<float>(0,0)) );
	extrwinfld_->attach( alignedBelow, evfld_ );

	if( setup_.allowlayerbased_ )
	{
	    extrwinfld_->setWithCheck();
	    extrwinfld_->setChecked( true );
	    layerbasedfld_ = new uiCheckBox( this, tr("Layer-based") );
	    layerbasedfld_->attach( rightOf, extrwinfld_ );
	    layerbasedfld_->setChecked( true );
	}

	extrstepfld_ = new uiGenInput( this, uiStrings::sStep(),
				       FloatInpSpec(defstep) );
	extrstepfld_->setElemSzPol( uiObject::Small );
	if ( layerbasedfld_ )
	    extrstepfld_->attach( rightOf, layerbasedfld_ );
	else
	    extrstepfld_->attach( rightOf,  extrwinfld_ );

	if ( setup_.allowlayerbased_ )
	{
	    BufferStringSet lvlnms; Strat::LVLS().getNames( lvlnms );
	    if ( !lvlnms.isEmpty() )
	    {
		uptolvlfld_ = new uiGenInput( this, tr("Stop at"),
					      StringListInpSpec(lvlnms) );
		uptolvlfld_->setText( lvlnms.last()->buf() );
		uptolvlfld_->setWithCheck( true );
		uptolvlfld_->setChecked( false );
		uptolvlfld_->attach( alignedBelow, extrwinfld_ );
	    }
	}
    }

    mAttachCB( postFinalize(), uiStratSeisEvent::initGrp );
}


uiStratSeisEvent::~uiStratSeisEvent()
{
    detachAllNotifiers();
}


void uiStratSeisEvent::initGrp( CallBacker* )
{
    mAttachCB( evfld_->checked, uiStratSeisEvent::evSnapCheck );
    if ( extrwinfld_ )
	mAttachCB( extrwinfld_->checked, uiStratSeisEvent::extrWinCB );
    if ( layerbasedfld_ )
	mAttachCB( layerbasedfld_->activated, uiStratSeisEvent::layBasedCB );
    if ( uptolvlfld_ )
	mAttachCB( uptolvlfld_->checked, uiStratSeisEvent::stopAtCB );

#define mSetAnyChg( fld, notif ) \
    if ( fld ) mAttachCB( fld->notif, uiStratSeisEvent::anyChgCB );
    mSetAnyChg( levelfld_, selChange );
    mSetAnyChg( evfld_, valueChanged );
    mSetAnyChg( evfld_, checked );
    mSetAnyChg( snapoffsfld_, valueChanged );
    mSetAnyChg( extrwinfld_, valueChanged );
    mSetAnyChg( extrwinfld_, checked );
    mSetAnyChg( extrstepfld_, valueChanged );
    mSetAnyChg( uptolvlfld_, valueChanged );
    mSetAnyChg( uptolvlfld_, checked );
    mSetAnyChg( layerbasedfld_, activated );

    layBasedCB( nullptr );
}


void uiStratSeisEvent::setLevel( const char* lvlnm )
{
    setLevel( Strat::LVLS().getIDByName( lvlnm ) );
}


void uiStratSeisEvent::setLevel( const Strat::LevelID& lvlid )
{
    setup_.levelid_ = lvlid;
    ev_.setLevelID( lvlid );
    if ( levelfld_ )
	levelfld_->setID( lvlid );
}


Strat::LevelID uiStratSeisEvent::levelID() const
{
    return levelfld_ ? levelfld_->getID() : setup_.levelid_;
}


BufferString uiStratSeisEvent::levelName() const
{
    return Strat::levelNameOf( levelID() );
}


bool uiStratSeisEvent::snapToEvent() const
{
    return evfld_->isChecked();
}


void uiStratSeisEvent::evSnapCheck( CallBacker* )
{
    snapoffsfld_->setSensitive( evfld_->isChecked() );
}


void uiStratSeisEvent::extrWinCB( CallBacker* )
{
    if ( uptolvlfld_ && !extrwinfld_->isChecked() )
	uptolvlfld_->setChecked( true );
}


void uiStratSeisEvent::stopAtCB( CallBacker* )
{
    if ( uptolvlfld_ )
	extrwinfld_->setChecked( !uptolvlfld_->isChecked() );
}


bool uiStratSeisEvent::hasExtrWin() const
{
    return extrwinfld_ && extrwinfld_->isCheckable()
		       && extrwinfld_->isChecked();
}


bool uiStratSeisEvent::hasStep() const
{
    return !layerbasedfld_ || !layerbasedfld_->isChecked();
}


void uiStratSeisEvent::layBasedCB( CallBacker* )
{
    if ( extrstepfld_ )
	extrstepfld_->display( hasStep() );
}


#define mErrRet(s) { if ( !s.isEmpty() ) uiMSG().error(s); return false; }

bool uiStratSeisEvent::getFromScreen()
{
    ev_.setLevelID( levelID() );
    ev_.setEvType( !evfld_->isChecked() ? VSEvent::None
		   : evtype_.getEnumForIndex(evfld_->getIntValue()) );
    if ( ev_.evType() != VSEvent::None )
	ev_.setOffset( snapoffsfld_->getFValue() * mToSecFactorF );

    if ( extrwinfld_ )
    {
	Interval<float> win( extrwinfld_->getFInterval() );
	if ( win.isUdf() )
	    mErrRet(uiStrings::phrEnter(tr("a valid time range")))

	win.scale( mToSecFactorF );
	ev_.setExtrWin( win );

	float extrstep = mUdf(float);
	if ( hasStep() )
	{
	    const float stepms = extrstepfld_->getFValue();
	    if ( mIsUdf(stepms) )
		mErrRet(uiStrings::phrEnter(tr("a valid step")))

	    extrstep = stepms * mToSecFactorF;
	}
	ev_.setExtrStep( extrstep );

	if ( uptolvlfld_ && uptolvlfld_->isChecked() )
	    ev_.setDownToLevelID( Strat::LVLS().getIDByName(
						uptolvlfld_->text() ) );
    }

    return true;
}


void uiStratSeisEvent::putToScreen()
{
    if ( levelfld_ )
	levelfld_->setID( ev_.levelID() );

    evfld_->setChecked( ev_.evType() != VSEvent::None );
    if ( ev_.evType() == VSEvent::None )
	evfld_->setValue( evtype_.getKey(ev_.evType()) );

    snapoffsfld_->setValue( ev_.offset() );
    if ( extrwinfld_ )
    {
	extrwinfld_->setValue( ev_.extrWin() );
	extrstepfld_->setChecked( !mIsUdf(ev_.extrStep()) );
	extrstepfld_->setValue( ev_.extrStep() );

	if ( uptolvlfld_ )
	{
	    const bool havelvl = ev_.downToLevelID().isValid();
	    uptolvlfld_->setChecked( havelvl );
	    if ( havelvl )
		uptolvlfld_->setText( Strat::levelNameOf(ev_.downToLevelID()) );
	}
    }
}


const StepInterval<float> uiStratSeisEvent::getFullExtrWin() const
{
    return StepInterval<float> ( ev_.extrWin().start, ev_.extrWin().stop,
				 ev_.extrStep() );
}
