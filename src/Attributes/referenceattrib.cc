/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : Helene Payraudeau
 * DATE     : July 2005
-*/

static const char* rcsID = "$Id: referenceattrib.cc,v 1.20 2007-11-23 11:59:06 cvsbert Exp $";


#include "referenceattrib.h"

#include "attribdataholder.h"
#include "attribdesc.h"
#include "attribdescset.h"
#include "attribfactory.h"
#include "attribparam.h"
#include "cubesampling.h"
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
    if ( isOutputEnabled(0) || isOutputEnabled(1) ) 
	coord = SI().transform( currentbid );

    for ( int idx=0; idx<nrsamples; idx++ )
    {
	const int outidx = z0 - output.z0_ + idx;
	setOutputValue( output, 0, idx, z0, coord.x );
	setOutputValue( output, 1, idx, z0, coord.y );
	if ( outputinterest[2] )
	{
	    if ( nrsamples==1 )
	    {
		int idi = localcomputezintervals.indexOf(Interval<int>(z0, z0));
		float exacttime = idi>-1 && idi<exactz_.size() ? exactz_[idi]
		    					       : (z0+idx)*step;
		setOutputValue( output, 2, idx, z0, exacttime );
	    }
	    else
		setOutputValue( output, 2, idx, z0, (z0+idx)*step );
	};

	if ( !is2d_ )
	{
	    setOutputValue( output, 3, idx, z0, currentbid.inl );
	    setOutputValue( output, 4, idx, z0, currentbid.crl );
	    setOutputValue( output, 5, idx, z0, z0+idx+1 );
	    if ( isOutputEnabled(6) )
	    {
		const int val = currentbid.inl - SI().inlRange(0).start + 1;
		setOutputValue( output, 6, idx, z0, val );
	    }
	    if ( isOutputEnabled(7) )
	    {
		const int val = currentbid.crl - SI().crlRange(0).start + 1;
		setOutputValue( output, 7, idx, z0, val );
	    }
	    if ( isOutputEnabled(8) )
	    {
		const int val = z0 - mNINT(SI().zRange(0).start/step) + idx + 1;
		setOutputValue( output, 8, idx, z0, val );
	    }
	}
	else
	{
	    setOutputValue( output, 3, idx, z0, currentbid.crl );
	    setOutputValue( output, 4, idx, z0, z0+idx+1 );
	    setOutputValue( output, 5, idx, z0,
			currentbid.crl - desiredvolume->hrg.start.crl + 1 );
	    if ( isOutputEnabled(6) )
	    {
		const int val = z0 - mNINT(SI().zRange(0).start/step) + idx + 1;
		setOutputValue( output, 6, idx, z0, val );
	    }
	}
    }

    return true;
}

}; // namespace Attrib
