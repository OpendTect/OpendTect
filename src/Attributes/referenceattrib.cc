/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Helene Payraudeau
 * DATE     : July 2005
-*/



#include "referenceattrib.h"

#include "attribdataholder.h"
#include "attribdesc.h"
#include "attribdescset.h"
#include "attribfactory.h"
#include "attribparam.h"
#include "trckeyzsampling.h"
#include "survinfo.h"


namespace Attrib
{

mAttrDefCreateInstance(Reference)

void Reference::initClass()
{
    mAttrStartInitClassWithUpdate

    desc->addInput( InputSpec("Input Data",true) );

    desc->setLocality( Desc::SingleTrace );
    desc->setUsesTrcPos( true );
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

    is2d_ = is2D();
}


bool Reference::getInputOutput( int input, TypeSet<int>& res ) const
{
    return Provider::getInputOutput( input, res );
}


bool Reference::getInputData( const BinID& relpos, int zintv )
{
    inputdata_ = inputs_[0]->getData( relpos, zintv );
    return inputdata_;
}


bool Reference::computeData( const DataHolder& output, const BinID& relpos,
			     int z0, int nrsamples, int threadid ) const
{
    const float step = refstep_ ? refstep_ : SI().zStep();
    Coord coord;
    const BinID truepos = currentbid_ + relpos;
    if ( isOutputEnabled(0) || isOutputEnabled(1) )
	coord = getCoord( truepos );

    for ( int idx=0; idx<nrsamples; idx++ )
    {
	setOutputValue( output, 0, idx, z0, float(coord.x) );
	setOutputValue( output, 1, idx, z0, float(coord.y) );
	if ( outputinterest_[2] )
	{
	    if ( nrsamples==1 )
	    {
		int idi = -1;
		for ( int intv=0; intv<localcomputezintervals_.size(); intv++)
		{
		    if ( localcomputezintervals_[intv].includes( z0, true ) )
		    {
			idi = intv;
			break;
		    }
		}
		float exacttime = idi>-1 && idi<exactz_.size() ? exactz_[idi]
		    					       : (z0+idx)*step;
		setOutputValue( output, 2, idx, z0, exacttime );
	    }
	    else
		setOutputValue( output, 2, idx, z0, (z0+idx)*step );
	};

	if ( !is2d_ )
	{
	    setOutputValue( output, 3, idx, z0, float(truepos.inl()) );
	    setOutputValue( output, 4, idx, z0, float(truepos.crl()) );
	    setOutputValue( output, 5, idx, z0, float(z0+idx+1) );
	    if ( isOutputEnabled(6) )
	    {
		const int val = truepos.inl() - SI().inlRange(0).start + 1;
		setOutputValue( output, 6, idx, z0, float(val) );
	    }
	    if ( isOutputEnabled(7) )
	    {
		const int val = truepos.crl() - SI().crlRange(0).start + 1;
		setOutputValue( output, 7, idx, z0, float(val) );
	    }
	    if ( isOutputEnabled(8) )
	    {
		const int val = z0 - mNINT32(SI().zRange(0).start/step) + idx+1;
		setOutputValue( output, 8, idx, z0, float(val) );
	    }
	}
	else
	{
	    setOutputValue( output, 3, idx, z0, float(truepos.crl()) );
	    setOutputValue( output, 4, idx, z0, float(z0+idx+1) );
	    const int trc0 = desiredvolume_->hsamp_.start_.crl();
	    setOutputValue( output, 5, idx, z0,
			    sCast(float,truepos.crl()-trc0+1) );
	    if ( isOutputEnabled(6) )
	    {
		const int val = z0 - mNINT32(SI().zRange(0).start/step) + idx+1;
		setOutputValue( output, 6, idx, z0, float(val) );
	    }
	}
    }

    return true;
}

} // namespace Attrib
