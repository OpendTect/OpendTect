/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Bert
 Date:          Feb 2008
________________________________________________________________________

-*/

static const char* rcsID = "$Id: uiposfiltgroup.cc,v 1.4 2008-02-26 08:55:18 cvsbert Exp $";

#include "uiposfiltgroupstd.h"
#include "posfilterstd.h"
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
    percpassfld_ = new uiSpinBox( this, 0, "Perc pass" );
    percpassfld_->setInterval( StepInterval<int>(1,99,1) );
    new uiLabel( this, "Percentage to pass", percpassfld_ );
    setHAlignObj( percpassfld_ );
}


void uiRandPosFiltGroup::usePar( const IOPar& iop )
{
    const int initialperc = percpassfld_->getValue();
    float perc = initialperc * 0.01;
    iop.get( Pos::RandomFilter::ratioStr(), perc );
    int iperc = mNINT(perc);
    if ( iperc > 0 && iperc < 100 )
	percpassfld_->setValue( iperc );
}


bool uiRandPosFiltGroup::fillPar( IOPar& iop ) const
{
    iop.set( sKey::Type, sKey::Random );
    const int perc = percpassfld_->getValue();
    iop.set( Pos::RandomFilter::ratioStr(), perc*0.01 );
    return true;
}


void uiRandPosFiltGroup::getSummary( BufferString& txt ) const
{
    txt += "Rand ["; txt += percpassfld_->getValue(); txt += "]"; 
}


void uiRandPosFiltGroup::initClass()
{
    uiPosFiltGroup::factory().addCreator( create, sKey::Random );
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
    iop.set( sKey::Type, sKey::Subsample );
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
    uiPosFiltGroup::factory().addCreator( create, sKey::Subsample );
}
