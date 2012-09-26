/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Feb 2008
________________________________________________________________________

-*/

static const char* rcsID mUsedVar = "$Id$";

#include "uiposfiltgroupstd.h"
#include "posfilterstd.h"
#include "uigeninput.h"
#include "uispinbox.h"
#include "uilabel.h"
#include "keystrs.h"
#include "iopar.h"

mImplFactory2Param(uiPosFiltGroup,uiParent*,const uiPosFiltGroup::Setup&,
		   uiPosFiltGroup::factory);


uiPosFiltGroup::uiPosFiltGroup( uiParent* p, const uiPosFiltGroup::Setup& su )
    : uiGroup(p,"Pos filter group")
{
}


uiRandPosFiltGroup::uiRandPosFiltGroup( uiParent* p,
					const uiPosFiltGroup::Setup& su )
    : uiPosFiltGroup(p,su)
{
    FloatInpSpec inpspec( 1 ); inpspec.setLimits( Interval<float>(0,100) );
    percpassfld_ = new uiGenInput( this, "Percentage to pass", inpspec );
    setHAlignObj( percpassfld_ );
}


void uiRandPosFiltGroup::usePar( const IOPar& iop )
{
    const float initialperc = percpassfld_->getfValue();
    float perc = mIsUdf(initialperc) ? initialperc : initialperc * 0.01f;
    iop.get( Pos::RandomFilter::ratioStr(), perc );
    if ( !mIsUdf(perc) ) perc *= 100;
    if ( perc >= 0 && perc <= 100 )
	percpassfld_->setValue( perc );
}


bool uiRandPosFiltGroup::fillPar( IOPar& iop ) const
{
    iop.set( sKey::Type(), sKey::Random() );
    const float perc = percpassfld_->getfValue();
    iop.set( Pos::RandomFilter::ratioStr(), perc*0.01 );
    return true;
}


void uiRandPosFiltGroup::getSummary( BufferString& txt ) const
{
    txt += "Rand ["; txt += percpassfld_->getfValue(); txt += "]"; 
}


void uiRandPosFiltGroup::initClass()
{
    uiPosFiltGroup::factory().addCreator( create, sKey::Random() );
}


uiSubsampPosFiltGroup::uiSubsampPosFiltGroup( uiParent* p,
					const uiPosFiltGroup::Setup& su )
    : uiPosFiltGroup(p,su)
{
    eachfld_ = new uiSpinBox( this, 0, "Take each" );
    eachfld_->setInterval( StepInterval<int>(2,999999,1) );
    new uiLabel( this, "Pass one every", eachfld_ );
    setHAlignObj( eachfld_ );
}


void uiSubsampPosFiltGroup::usePar( const IOPar& iop )
{
    int nr = eachfld_->getValue();
    iop.get( Pos::SubsampFilter::eachStr(), nr );
    eachfld_->setValue( nr );
}


bool uiSubsampPosFiltGroup::fillPar( IOPar& iop ) const
{
    iop.set( sKey::Type(), sKey::Subsample() );
    iop.set( Pos::SubsampFilter::eachStr(), eachfld_->getValue() );
    return true;
}


void uiSubsampPosFiltGroup::getSummary( BufferString& txt ) const
{
    const int nr = eachfld_->getValue();
    txt += "Each "; txt += nr; txt += getRankPostFix( nr );
}


void uiSubsampPosFiltGroup::initClass()
{
    uiPosFiltGroup::factory().addCreator( create, sKey::Subsample() );
}
