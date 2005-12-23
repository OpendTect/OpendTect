/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : Helene Payraudeau
 * DATE     : July 2005
-*/

static const char* rcsID = "$Id: referenceattrib.cc,v 1.14 2005-12-23 16:09:46 cvsnanne Exp $";


#include "referenceattrib.h"

#include "attribdataholder.h"
#include "attribdesc.h"
#include "attribfactory.h"
#include "attribparam.h"
#include "cubesampling.h"
#include "seistrcsel.h"
#include "survinfo.h"


namespace Attrib
{

void Reference::initClass()
{
    Desc* desc = new Desc( attribName(), updateDesc );
    desc->ref();

    BoolParam* is2d_ = new BoolParam( is2DStr() );
    is2d_->setDefaultValue(false);
    is2d_->setRequired(false);
    desc->addParam( is2d_ );

    desc->addInput( InputSpec("Input Data",true) );
    desc->init();

    PF().addDesc( desc, createInstance );
    desc->unRef();
}


Provider* Reference::createInstance( Desc& ds )
{
    Reference* res = new Reference( ds );
    res->ref();
    if ( !res->isOK() )
    {
        res->unRef();
        return 0;
    }

    res->unRefNoDelete();
    return res;
}


void Reference::updateDesc( Desc& desc )
{
    const bool is2Dsurvey = desc.getValParam( is2DStr() )->getBoolValue();
    if ( !is2Dsurvey )
	desc.setNrOutputs( Seis::UnknowData, 9 );
    else
	desc.setNrOutputs( Seis::UnknowData, 7 );
}


Reference::Reference( Desc& ds )
    : Provider(ds)
{
    if ( !isOK() ) return;
    
    mGetBool( is2d_, is2DStr() );
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
			     int z0, int nrsamples ) const
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
