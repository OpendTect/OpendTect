/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : Helene Payraudeau
 * DATE     : July 2005
-*/

static const char* rcsID = "$Id: referenceattrib.cc,v 1.17 2007-03-08 12:40:08 cvshelene Exp $";


#include "referenceattrib.h"

#include "attribdataholder.h"
#include "attribdesc.h"
#include "attribdescset.h"
#include "attribfactory.h"
#include "attribparam.h"
#include "cubesampling.h"
#include "seistrcsel.h"
#include "survinfo.h"


namespace Attrib
{

mAttrDefCreateInstance(Reference)
    
void Reference::initClass()
{
    mAttrStartInitClassWithUpdate

    desc->addInput( InputSpec("Input Data",true) );

    mAttrEndInitClass
}


void Reference::updateDesc( Desc& ds )
{
    const bool is2d = ds.descSet() ? ds.descSet()->is2D() : false;
    ds.setNrOutputs( Seis::UnknowData, is2d ? 7 : 9 );
}


Reference::Reference( Desc& ds )
    : Provider(ds)
{
    if ( !isOK() ) return;
    
    is2d_ = ds.descSet() ? ds.descSet()->is2D() : false;
}


bool Reference::getInputOutput( int input, TypeSet<int>& res ) const
{
    return Provider::getInputOutput( input, res );
}


bool Reference::getInputData( const BinID& relpos, int zintv )
{
    inputdata_ = inputs[0]->getData( relpos, zintv );
    return inputdata_;
}


bool Reference::computeData( const DataHolder& output, const BinID& relpos,
			     int z0, int nrsamples, int threadid ) const
{
    const float step = refstep ? refstep : SI().zStep();
    Coord coord;
    if ( outputinterest[0] || outputinterest[1] ) 
	coord = SI().transform( currentbid );

    for ( int idx=0; idx<nrsamples; idx++ )
    {
	const int outidx = z0 - output.z0_ + idx;
	if ( outputinterest[0] )
	    output.series(0)->setValue( outidx, coord.x );
	if ( outputinterest[1] )
	    output.series(1)->setValue( outidx, coord.y );
	if ( outputinterest[2] )
	    output.series(2)->setValue( outidx, (z0+idx)*step );

	if ( !is2d_ )
	{
	    if ( outputinterest[3] )
		output.series(3)->setValue( outidx, currentbid.inl );
	    if ( outputinterest[4] )
		output.series(4)->setValue( outidx, currentbid.crl );
	    if ( outputinterest[5] )
		output.series(5)->setValue( outidx, z0+idx+1 );
	    if ( outputinterest[6] )
	    {
		const int val = currentbid.inl - SI().inlRange(0).start + 1;
		output.series(6)->setValue( outidx, val );
	    }
	    if ( outputinterest[7] )
	    {
		const int val = currentbid.crl - SI().crlRange(0).start + 1;
		output.series(7)->setValue( outidx, val );
	    }
	    if ( outputinterest[8] )
	    {
		const int val = z0 - mNINT(SI().zRange(0).start/step) + idx + 1;
		output.series(8)->setValue( outidx, val );
	    }
	}
	else
	{
	    if ( outputinterest[3] )
		output.series(3)->setValue( outidx, currentbid.crl );
	    if ( outputinterest[4] )
		output.series(4)->setValue( outidx, z0+idx+1 );
	    if ( outputinterest[5] )
		output.series(5)->setValue( outidx,
			currentbid.crl - desiredvolume->hrg.start.crl + 1 );
	    if ( outputinterest[6] )
	    {
		const int val = z0 - mNINT(SI().zRange(0).start/step) + idx + 1;
		output.series(6)->setValue( outidx, val );
	    }
	}
    }

    return true;
}

}; // namespace Attrib
