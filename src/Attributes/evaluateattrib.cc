/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : H. Payraudeau
 * DATE     : Oct 2005
-*/

static const char* rcsID = "$Id: evaluateattrib.cc,v 1.5 2006-04-12 11:33:41 cvsnanne Exp $";


#include "evaluateattrib.h"
#include "attribdataholder.h"
#include "attribdesc.h"
#include "attribfactory.h"


namespace Attrib
{

void Evaluate::initClass()
{
    Desc* desc = new Desc( attribName() );
    desc->ref();

    desc->init();

    PF().addDesc( desc, createInstance );
    desc->unRef();
}


Provider* Evaluate::createInstance( Desc& desc )
{
    Evaluate* res = new Evaluate( desc );
    res->ref();

    if ( !res->isOK() )
    {
        res->unRef();
        return 0;
    }

    res->unRefNoDelete();
    return res;
}


Evaluate::Evaluate( Desc& ds )
        : Provider( ds )
{
    if ( !isOK() ) return;

    inputdata_.allowNull( true );
}


bool Evaluate::getInputOutput( int input, TypeSet<int>& res ) const
{
    return Provider::getInputOutput( input, res );
}


bool Evaluate::getInputData( const BinID& relpos, int zintv )
{
    while ( inputdata_.size() < inputs.size() )
	inputdata_ += 0;

    for ( int idx=0; idx<inputs.size(); idx++ )
    {
	const DataHolder* data = inputs[idx]->getData( relpos, zintv );
	if ( !data ) return false;

	inputdata_.replace( idx, data );
	dataidx_ += getDataIndex( idx );
    }
    
    return true;
}


bool Evaluate::computeData( const DataHolder& output, const BinID& relpos,
			    int z0, int nrsamples ) const
{
    if ( !inputdata_.size() || !output.nrSeries() ) return false;

    for ( int idx=0; idx<nrsamples; idx++ )
    {
	const int cursample = z0 + idx;
	for ( int sidx=0; sidx<output.nrSeries(); sidx++ )
	{
	    if ( !outputinterest[sidx] ) continue;

	    const ValueSeries<float>* valseries = 
			inputdata_[sidx]->series( dataidx_[sidx] );

	    const int outidx = z0 - output.z0_ + idx;
	    if ( !valseries )
		output.series(sidx)->setValue( outidx, mUdf(float) );
	    else
		output.series(sidx)->setValue( outidx,
			valseries->value(cursample-inputdata_[sidx]->z0_) );
	}
    }

    return true;
}
			
} // namespace Attrib
