/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Feb 2011
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "uistratseisevent.h"
#include "uibutton.h"
#include "uigeninput.h"
#include "uilabel.h"
#include "uistratlvlsel.h"
#include "uimsg.h"
#include "stratlevel.h"
#include "survinfo.h"
#include "valseriesevent.h"

#define mToSecFactorF	0.001f

uiStratSeisEvent::uiStratSeisEvent( uiParent* p,
				    const uiStratSeisEvent::Setup& su )
    : uiGroup(p,"Strat Seis Event Specification Group")
    , setup_(su)
    , extrwinfld_(0)
    , usestepfld_(0)
    , extrstepfld_(0)
    , nosteplbl_(0)
    , levelfld_(0)
    , uptolvlfld_(0)
{
    if ( !setup_.fixedlevel_ )
	levelfld_ = new uiStratLevelSel( this, false, "Reference level" );

    BufferStringSet eventnms( VSEvent::TypeNames() );
    eventnms.removeSingle(0);
    evfld_ = new uiGenInput( this, "Snap synthetics to event",
				StringListInpSpec(eventnms) );
    evfld_->setWithCheck( true );
    evfld_->checked.notify( mCB(this,uiStratSeisEvent,evSnapCheck) );
    evfld_->setValue( 1 );
    if ( levelfld_ ) evfld_->attach( alignedBelow, levelfld_ );
    evfld_->setElemSzPol( uiObject::Medium );
    setHAlignObj( evfld_ );
    snapoffsfld_ = new uiGenInput( this, "Offset (ms)", FloatInpSpec(0) );
    snapoffsfld_->attach( rightOf, evfld_ );
    snapoffsfld_->setElemSzPol( uiObject::Small );
    snapoffsfld_->setSensitive( false );

    if ( setup_.withextrwin_ )
    {
	const float defstep = SI().zIsTime() ? SI().zStep()/mToSecFactorF : 4;
	extrwinfld_ = new uiGenInput( this,
				      BufferString("Extraction window ",
				      SI().getZUnitString()),
				  FloatInpIntervalSpec(Interval<float>(0,0)) );
	extrwinfld_->attach( alignedBelow, evfld_ );

	if( setup_.allowlayerbased_ )
	{
	    extrwinfld_->setWithCheck();
	    extrwinfld_->checked.notify( mCB(this,uiStratSeisEvent,extrWinCB) );
	    extrwinfld_->setChecked( true );

	    usestepfld_ = new uiCheckBox( this, " " );
	    usestepfld_->setChecked();
	    usestepfld_->activated.notify(
					 mCB(this,uiStratSeisEvent,stepSelCB) );
	    usestepfld_->attach( rightOf, extrwinfld_ );

	    nosteplbl_ = new uiLabel( this, "Layer-based" );
	    nosteplbl_->display( false );
	    nosteplbl_->attach( rightOf, usestepfld_ );
	}

	extrstepfld_ = new uiGenInput( this, "Step", FloatInpSpec(defstep) );
	extrstepfld_->setElemSzPol( uiObject::Small );
	if ( usestepfld_ )
	    extrstepfld_->attach( rightOf, usestepfld_ );
	else
	    extrstepfld_->attach( rightOf,  extrwinfld_ );

	if ( !setup_.allowlayerbased_ )
	    return;

	BufferStringSet lvlnms; Strat::LVLS().getNames( lvlnms );
	if ( !lvlnms.isEmpty() )
	{
	    uptolvlfld_ = new uiGenInput( this, "Stop at",
					  StringListInpSpec(lvlnms) );
	    uptolvlfld_->setText( lvlnms.get(lvlnms.size()-1).buf() );
	    uptolvlfld_->setWithCheck( true );
	    uptolvlfld_->setChecked( false );
	    uptolvlfld_->checked.notify( mCB(this,uiStratSeisEvent,stopAtCB) );
	    uptolvlfld_->attach( alignedBelow, extrwinfld_ );
	}
    }
}


void uiStratSeisEvent::setLevel( const char* lvlnm )
{
    const Strat::Level* lvl = Strat::LVLS().get( lvlnm );
    if ( levelfld_ )
	levelfld_->setSelected( lvl );
    else if ( lvl )
    {
	setup_.fixedlevel_ = lvl;
	ev_.setLevel( lvl );
    }
}


const char* uiStratSeisEvent::levelName() const
{
    return levelfld_ ? levelfld_->getName() : setup_.fixedlevel_->name().buf();
}


void uiStratSeisEvent::evSnapCheck( CallBacker* )
{
    snapoffsfld_->setSensitive( evfld_->isChecked() );
}


void uiStratSeisEvent::extrWinCB( CallBacker* )
{
    if ( !extrwinfld_->isChecked() )
	uptolvlfld_->setChecked( true );
}


void uiStratSeisEvent::stopAtCB( CallBacker* )
{
    extrwinfld_->setChecked( !uptolvlfld_->isChecked() );
}


bool uiStratSeisEvent::hasExtrWin() const
{
    return extrwinfld_->isCheckable() && extrwinfld_->isChecked();
}


bool uiStratSeisEvent::hasStep() const
{
    return usestepfld_->isChecked();
}


void uiStratSeisEvent::stepSelCB( CallBacker* )
{
    const bool usestep = usestepfld_->isChecked();
    extrstepfld_->display( usestep );
    nosteplbl_->display( !usestep );
}


#define mErrRet(s) { if ( s && *s ) uiMSG().error(s); return false; }

bool uiStratSeisEvent::getFromScreen()
{
    ev_.setLevel( setup_.fixedlevel_ );
    if ( levelfld_ )
    {
	ev_.setLevel( levelfld_->selected() );
	if ( !ev_.level() )
	    mErrRet("Cannot find selected stratigraphic level")
    }

    ev_.setEvType( !evfld_->isChecked() ? VSEvent::None
		   : (VSEvent::Type)(evfld_->getIntValue()+1) );
    if ( ev_.evType() != VSEvent::None )
	ev_.setOffset( snapoffsfld_->getfValue() * mToSecFactorF );

    if ( extrwinfld_ )
    {
	Interval<float> win( extrwinfld_->getFInterval() );
	if ( win.isUdf() )
	    mErrRet("Please enter a valid time range")

	win.scale( mToSecFactorF );
	ev_.setExtrWin( win );

	float extrstep = mUdf(float);
	if ( !doAllLayers() )
	{
	    const float stepms = extrstepfld_->getfValue();
	    if ( mIsUdf(stepms) )
		mErrRet("Please enter a valid step")

	    extrstep = stepms * mToSecFactorF;
	}
	ev_.setExtrStep( extrstep );

	if ( uptolvlfld_ && uptolvlfld_->isChecked() )
	    ev_.setDownToLevel( Strat::LVLS().get( uptolvlfld_->text() ) );
    }

    return true;
}


void uiStratSeisEvent::putToScreen()
{
    if ( levelfld_ )
	levelfld_->setSelected( ev_.level() );
    evfld_->setChecked( ev_.evType() != VSEvent::None );
    if ( ev_.evType() == VSEvent::None )
	evfld_->setValue( ((int)ev_.evType())-1 );

    snapoffsfld_->setValue( ev_.offset() );
    if ( extrwinfld_ )
    {
	extrwinfld_->setValue( ev_.extrWin() );
	extrstepfld_->setChecked( !mIsUdf(ev_.extrStep()) );
	if ( !doAllLayers() )
	    extrstepfld_->setValue( ev_.extrStep() );

	if ( uptolvlfld_ )
	{
	    uptolvlfld_->setChecked( (bool)ev_.downToLevel() );
	    if ( ev_.downToLevel() )
		uptolvlfld_->setText( ev_.downToLevel()->name() );
	}
    }
}


bool uiStratSeisEvent::doAllLayers() const
{
    return usestepfld_ && !usestepfld_->isChecked();
}


const StepInterval<float> uiStratSeisEvent::getFullExtrWin() const
{
    return StepInterval<float> ( ev_.extrWin().start, ev_.extrWin().stop,
				 ev_.extrStep() );
}

