/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Feb 2011
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uistratseisevent.cc,v 1.3 2011-02-09 12:28:16 cvsbert Exp $";

#include "uistratseisevent.h"
#include "uicombobox.h"
#include "uigeninput.h"
#include "uimsg.h"
#include "stratlevel.h"
#include "survinfo.h"
#include "valseriesevent.h"


uiStratSeisEvent::uiStratSeisEvent( uiParent* p,
				    const uiStratSeisEvent::Setup& su )
    : uiGroup(p,"Strat Seis Event Specification Group")
    , setup_(su)
    , extrwinfld_(0)
    , levelfld_(0)
{
    uiLabeledComboBox* lcb = 0;
    if ( !setup_.fixedlevel_ )
    {
	lcb = new uiLabeledComboBox( this, "Reference level" );
	levelfld_ = lcb->box();
	const Strat::LevelSet& lvls = Strat::LVLS();
	for ( int idx=0; idx<lvls.size(); idx++ )
	    levelfld_->addItem( lvls.levels()[idx]->name() );
    }

    BufferStringSet eventnms( VSEvent::TypeNames() );
    eventnms.remove(0);
    evfld_ = new uiGenInput( this, "Snap synthetics to event",
	    			StringListInpSpec(eventnms) );
    evfld_->setWithCheck( true );
    evfld_->checked.notify( mCB(this,uiStratSeisEvent,evSnapCheck) );
    evfld_->setValue( 1 );
    if ( lcb ) evfld_->attach( alignedBelow, lcb );
    evfld_->setElemSzPol( uiObject::Small );
    setHAlignObj( evfld_ );
    snapoffsfld_ = new uiGenInput( this, "Offset (ms)", FloatInpSpec(0) );
    snapoffsfld_->attach( rightOf, evfld_ );
    snapoffsfld_->setElemSzPol( uiObject::Small );
    snapoffsfld_->setSensitive( false );

    if ( setup_.withextrwin_ )
    {
	const float defstep = SI().zIsTime() ? SI().zStep() * 1000 : 4;
	extrwinfld_ = new uiGenInput( this, "Extraction window",
	      FloatInpIntervalSpec(StepInterval<float>(0,0,defstep)) );
	extrwinfld_->attach( alignedBelow, evfld_ );
    }
}


void uiStratSeisEvent::setLevel( const char* lvlnm )
{
    ev_.level_ = Strat::LVLS().get( lvlnm );
    if ( levelfld_ )
	levelfld_->setText( lvlnm );
}


void uiStratSeisEvent::evSnapCheck( CallBacker* )
{
    snapoffsfld_->setSensitive( evfld_->isChecked() );
}


#define mErrRet(s) { if ( s && *s ) uiMSG().error(s); return false; }

bool uiStratSeisEvent::getFromScreen()
{
    if ( levelfld_ )
    {
	ev_.level_ = Strat::LVLS().get( levelfld_->text() );
	if ( !ev_.level_ )
	    mErrRet("Cannot find selected stratigraphic level")
    }

    ev_.evtype_ = !evfld_->isChecked() ? VSEvent::None
		: (VSEvent::Type)(evfld_->getIntValue()+1);
    if ( ev_.evtype_ != VSEvent::None )
	ev_.offs_ = snapoffsfld_->getfValue() *.001;

    if ( extrwinfld_ )
    {
	ev_.extrwin_ = extrwinfld_->getFStepInterval();
	if ( mIsUdf(ev_.extrwin_.start) || mIsUdf(ev_.extrwin_.stop)
	  || mIsUdf(ev_.extrwin_.step) )
	    mErrRet("Please enter all extraction window parameters")
	ev_.extrwin_.start *= 0.001; ev_.extrwin_.stop *= 0.001;
	ev_.extrwin_.step *= 0.001;
    }

    return true;
}


void uiStratSeisEvent::putToScreen()
{
    if ( levelfld_ && ev_.level_ )
	levelfld_->setText( ev_.level_->name() );
    evfld_->setChecked( ev_.evtype_ != VSEvent::None );
    if ( ev_.evtype_ == VSEvent::None )
	evfld_->setValue( ((int)ev_.evtype_)-1 );
    snapoffsfld_->setValue( ev_.offs_ );
    if ( extrwinfld_ )
	extrwinfld_->setValue( ev_.extrwin_ );
}
