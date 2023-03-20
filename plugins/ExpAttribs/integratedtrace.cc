/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "integratedtrace.h"
#include "attribdataholder.h"
#include "attribdesc.h"
#include "attribfactory.h"


// Based on:
// https://agilescientific.com/blog/2011/12/28/i-is-for-integrated-trace.html

namespace Attrib
{

mAttrDefCreateInstance(IntegratedTrace)

void IntegratedTrace::initClass()
{
    mAttrStartInitClass
    desc->addInput( InputSpec("Input Data",true) );
    desc->addOutputDataType( Seis::UnknowData );

    desc->setLocality( Desc::SingleTrace );
    mAttrEndInitClass
}


IntegratedTrace::IntegratedTrace( Desc& desc )
    : Provider(desc)
{
    if ( !isOK() )
    {
	return;
    }

    desgate_ = Interval<int>( -2048, 2048 );
}


bool IntegratedTrace::allowParallelComputation() const
{
    return false;
}


bool IntegratedTrace::getInputData( const BinID& relpos, int zintv )
{
    inputdata_ = inputs_[0]->getData( relpos, zintv );
    dataidx_ = getDataIndex( 0 );
    return inputdata_;
}


const Interval<int>* IntegratedTrace::desZSampMargin( int input,
						      int output ) const
{
    return &desgate_;
}


bool IntegratedTrace::computeData( const DataHolder& output,
				   const BinID& relpos, int z0,
				   int nrsamples, int threadid ) const
{
    if ( !inputdata_ || inputdata_->isEmpty() ||
	 !inputdata_->series(dataidx_) )
	 return false;

    TypeSet<float> outtmp;
    float sum = 0;
    const int nrinpsamples = inputdata_->nrsamples_;
    for ( int idx=0; idx<nrinpsamples; idx++ )
    {
	const float val = inputdata_->series( dataidx_ )->value( idx );
	if ( !mIsUdf(val) )
	    sum += val;

	outtmp += sum;
    }

    int outidx = 0;

    for ( int idx=0; idx<nrsamples; idx++ )
    {
	setOutputValue( output, 0, outidx, z0,
			outtmp[((z0-inputdata_->z0_)+idx)]);
	outidx++;
    }

    return true;
}

} // namespace Attrib
