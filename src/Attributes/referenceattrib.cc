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
#include "survinfo.h"


namespace Attrib
{

mAttrDefCreateInstance(Reference)

void Reference::initClass()
{
    mAttrStartInitClassWithUpdate

    desc->addInput( InputSpec("Input Data",true) );

    desc->setIsSingleTrace( true );
    desc->setUsesTrcPos( true );
    mAttrEndInitClass
}


void Reference::updateDesc( Desc& ds )
{
    const bool is2d = ds.descSet() ? ds.descSet()->is2D() : false;
    ds.setNrOutputs( Seis::UnknownData, is2d ? 7 : 9 );
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
    const float step = refzstep_ ? refzstep_ : SI().zStep();
    Coord coord;
    const BinID truepos = currentbid_ + relpos;
    if ( isOutputEnabled(0) || isOutputEnabled(1) )
	coord = SI().transform( truepos );

    for ( int idx=0; idx<nrsamples; idx++ )
    {
	setOutputValue( output, 0, idx, z0, (float) coord.x_ );
	setOutputValue( output, 1, idx, z0, (float) coord.y_ );
	if ( outputinterest_[2] )
	{
	    if ( nrsamples==1 )
	    {
		int idi = -1;
		for ( int index=0; index<localcomputezintervals_.size();
		      index++)
		{
		    if ( localcomputezintervals_[index].includes( z0, true ) )
		    {
			idi = index;
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
	    setOutputValue( output, 3, idx, z0, mCast(float,truepos.inl()) );
	    setOutputValue( output, 4, idx, z0, mCast(float,truepos.crl()) );
	    setOutputValue( output, 5, idx, z0, mCast(float,z0+idx+1) );
	    if ( isOutputEnabled(6) )
	    {
		const int val = truepos.inl() - SI().inlRange().start + 1;
		setOutputValue( output, 6, idx, z0, mCast(float,val) );
	    }
	    if ( isOutputEnabled(7) )
	    {
		const int val = truepos.crl() - SI().crlRange().start + 1;
		setOutputValue( output, 7, idx, z0, mCast(float,val) );
	    }
	    if ( isOutputEnabled(8) )
	    {
		const int val = z0 - mNINT32(SI().zRange().start/step) +idx +1;
		setOutputValue( output, 8, idx, z0, mCast(float,val) );
	    }
	}
	else
	{
	    setOutputValue( output, 3, idx, z0, mCast(float,truepos.crl()) );
	    setOutputValue( output, 4, idx, z0, mCast(float,z0+idx+1) );
	    setOutputValue( output, 5, idx, z0, mCast(float,
		truepos.crl() - desiredsubsel_.trcNrRange().start + 1) );
	    if ( isOutputEnabled(6) )
	    {
		const int val =
		    z0 - mNINT32(SI().zRange().start/step) + idx + 1;
		setOutputValue( output, 6, idx, z0, mCast(float,val) );
	    }
	}
    }

    return true;
}

}; // namespace Attrib
