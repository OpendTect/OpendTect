/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert Bril
 Date:          Aug 2006
________________________________________________________________________

-*/
static const char* rcsID mUnusedVar = "$Id$";

#include "deltaresampleattrib.h"
#include "attribdataholder.h"
#include "attribdesc.h"
#include "attribfactory.h"
#include "attribparam.h"
#include "valseriesinterpol.h"
#include "survinfo.h"


namespace Attrib
{
    
mAttrDefCreateInstance(DeltaResample)
    
void DeltaResample::initClass()
{
    mAttrStartInitClass
	
    FloatParam* period = new FloatParam( periodStr() );
    desc->addParam( period );
    
    desc->addInput( InputSpec("Ref cube",true) );
    desc->addInput( InputSpec("Delta cube",true) );
    
    desc->setNrOutputs( Seis::UnknowData, 1 );

    desc->setLocality( Desc::SingleTrace );
    mAttrEndInitClass
}


DeltaResample::DeltaResample( Desc& desc )
    : Provider( desc )
    , dessamps_(-1000,1000)
    , period_(0)
{
    if ( !isOK() ) return;

    mGetFloat( period_, periodStr() );
}


bool DeltaResample::getInputData( const BinID& relpos, int zintv )
{
    refcubedata_ = inputs_[0]->getData( relpos, zintv );
    deltacubedata_ = inputs_[1]->getData( relpos, zintv );
    if ( !refcubedata_ || !deltacubedata_ )
	return false;

    dcdataidx_ = getDataIndex( 1 );
    return (refseries_ = refcubedata_->series( getDataIndex(0) ));
}


const Interval<int>* DeltaResample::desZSampMargin(int,int) const
{
    return &dessamps_;
}


bool DeltaResample::computeData( const DataHolder& output, const BinID& relpos, 
			      int z0, int nrsamples, int threadid ) const
{
    ValueSeriesInterpolator<float> interp( refcubedata_->nrsamples_ - 1 );
    interp.period_ = period_;
    interp.isperiodic_ = !mIsZero(interp.period_,1e-6)
		      && !mIsUdf(interp.period_);
    int refoffs = z0 - refcubedata_->z0_;

    const float inpfac = 1.f / (refstep_ * SI().zDomain().userFactor());
    for ( int idx=0; idx<nrsamples; idx++ )
    {
	const float deltaval = getInputValue( *deltacubedata_, dcdataidx_,
					      idx, z0 );
	const float refpos = idx + refoffs + deltaval * inpfac;
	const float refval = interp.value( *refseries_, refpos );
	setOutputValue( output, 0, idx, z0, refval );
    }
    return true;
}

}; //namespace
