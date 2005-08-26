/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : Helene PAYRAUDEAU
 * DATE     : July 2005
-*/

static const char* rcsID = "$Id: referenceattrib.cc,v 1.4 2005-08-26 07:15:41 cvshelene Exp $";


#include "referenceattrib.h"
#include "attribdataholder.h"
#include "attribdesc.h"
#include "attribfactory.h"
#include "survinfo.h"
#include "position.h"
#include "cubesampling.h"
#include "datainpspec.h"
#include "attribparam.h"


namespace Attrib
{

void Reference::initClass()
{
    Desc* desc = new Desc( attribName() );
    desc->ref();

    desc->setNrOutputs( Seis::UnknowData, 9 );
    
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


Reference::Reference( Desc& desc_ )
        : Provider( desc_ )
{
}


bool Reference::computeData( const DataHolder& output, const BinID& relpos,
			      int t0, int nrsamples ) const
{
    float step = refstep ? refstep : SI().zStep();
    Coord coord;
    if ( outputinterest[0] || outputinterest[1] ) 
	coord = SI().transform( currentbid );
    
    for ( int idx=0; idx<nrsamples; idx++ )
    {
	if ( outputinterest[0] )
	    output.item(0)->setValue(idx, coord.x);
	if ( outputinterest[1] )
	    output.item(1)->setValue(idx, coord.y);
	if ( outputinterest[2] )
	{
	    float val;
	    if ( seldata_.type_ == Seis::Table )
	    {
		float offset = SI().zRange(0).start<0 ? SI().zRange(0).start :0;
		val = ( t0 + idx ) * step + offset;
	    }
	    else
		val = possiblevolume->zrg.start + idx * step;
	    
	    output.item(2)->setValue(idx, val);
	}
	if ( outputinterest[3] )
	    output.item(3)->setValue(idx, currentbid.inl);
	if ( outputinterest[4] )
	    output.item(4)->setValue(idx, currentbid.crl);
	if ( outputinterest[5] )
	    output.item(5)->setValue(idx, t0 + idx);
	if ( outputinterest[6] )
	{
	    int val = currentbid.inl - possiblevolume->hrg.start.inl;
	    output.item(6)->setValue(idx, val);
	}
	if ( outputinterest[7] )
	{
	    int val = currentbid.crl - possiblevolume->hrg.start.crl;
	    output.item(7)->setValue(idx, val);
	}
	if ( outputinterest[8] )
	    output.item(8)->setValue(idx, idx);
    }

    return true;
}
			
};
