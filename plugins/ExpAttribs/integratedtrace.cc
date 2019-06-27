/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Khushnood
 * DATE     : June 2019
-*/


#include "integratedtrace.h"
#include "attribdataholder.h"
#include "attribdesc.h"
#include "attribdescset.h"
#include "attribfactory.h"
#include "attribparam.h"
#include "math2.h"
#include "genericnumer.h"
#include "survinfo.h"
#include "envvars.h"
#include "od_ostream.h"


#include <math.h>



namespace Attrib
{

mAttrDefCreateInstance(IntegratedTrace)

void IntegratedTrace::initClass()
{
    mAttrStartInitClass
    desc->addInput( InputSpec("Input Data",true) );
    desc->addOutputDataType( Seis::UnknownData );
    desc->setIsSingleTrace( true );
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
