/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Feb 2008
________________________________________________________________________

-*/


#include "uiposfiltgroupstd.h"
#include "posfilterstd.h"
#include "uigeninput.h"
#include "uispinbox.h"
#include "uilabel.h"
#include "keystrs.h"
#include "iopar.h"

mImplClassFactory(uiPosFiltGroup,factory);


uiPosFiltGroup::uiPosFiltGroup( uiParent* p, const uiPosFiltGroup::Setup& su )
    : uiGroup(p,"Pos filter group")
{
}


uiRandPosFiltGroup::uiRandPosFiltGroup( uiParent* p,
					const uiPosFiltGroup::Setup& su )
    : uiPosFiltGroup(p,su)
{
    FloatInpSpec inpspec( 1 ); inpspec.setLimits( Interval<float>(0,100) );
    percpassfld_ = new uiGenInput( this, tr("Percentage to select"), inpspec );
    setHAlignObj( percpassfld_ );
}


void uiRandPosFiltGroup::usePar( const IOPar& iop )
{
    const float initialperc = percpassfld_->getFValue();
    float perc = mIsUdf(initialperc) ? initialperc : initialperc * 0.01f;
    iop.get( Pos::RandomFilter::ratioStr(), perc );
    if ( !mIsUdf(perc) ) perc *= 100;
    if ( perc >= 0 && perc <= 100 )
	percpassfld_->setValue( perc );
}


bool uiRandPosFiltGroup::fillPar( IOPar& iop ) const
{
    iop.set( sKey::Type(), sKey::Random() );
    const float perc = percpassfld_->getFValue();
    iop.set( Pos::RandomFilter::ratioStr(), perc*0.01 );
    return true;
}


void uiRandPosFiltGroup::getSummary( uiString& txt ) const
{
    txt.appendPhrase( tr("Random [%1]").arg(percpassfld_->getFValue()),
				    uiString::Space, uiString::OnSameLine );
}


void uiRandPosFiltGroup::initClass()
{
    uiPosFiltGroup::factory().addCreator( create, sKey::Random(),
					  uiStrings::sRandom() );
}


uiSubsampPosFiltGroup::uiSubsampPosFiltGroup( uiParent* p,
					const uiPosFiltGroup::Setup& su )
    : uiPosFiltGroup(p,su)
{
    eachfld_ = new uiSpinBox( this, 0, "Take each" );
    eachfld_->setInterval( StepInterval<int>(2,999999,1) );
    new uiLabel( this, tr("Select one every"), eachfld_ );
    setHAlignObj( eachfld_ );
}


void uiSubsampPosFiltGroup::usePar( const IOPar& iop )
{
    int nr = eachfld_->getIntValue();
    iop.get( Pos::SubsampFilter::eachStr(), nr );
    eachfld_->setValue( nr );
}


bool uiSubsampPosFiltGroup::fillPar( IOPar& iop ) const
{
    iop.set( sKey::Type(), sKey::Subsample() );
    iop.set( Pos::SubsampFilter::eachStr(), eachfld_->getIntValue() );
    return true;
}


void uiSubsampPosFiltGroup::getSummary( uiString& txt ) const
{
    const int nr = eachfld_->getIntValue();
    txt.appendPhrase(tr("Pass Every %1%2","Pass every n(th)").arg(nr)
       .arg(toUiString(getRankPostFix( nr ))),
       uiString::Space, uiString::OnSameLine);
}


void uiSubsampPosFiltGroup::initClass()
{
    uiPosFiltGroup::factory().addCreator( create, sKey::Subsample(),
							    tr("Sub Sample") );
}
