/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : Helene PAYRAUDEAU
 * DATE     : July 2005
-*/

static const char* rcsID = "$Id: referenceattrib.cc,v 1.10 2005-10-28 15:09:50 cvshelene Exp $";


#include "referenceattrib.h"
#include "attribdataholder.h"
#include "attribdesc.h"
#include "attribfactory.h"
#include "survinfo.h"
#include "position.h"
#include "cubesampling.h"
#include "datainpspec.h"
#include "attribparam.h"
#include "seistrcsel.h"


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
    bool is2Dsurvey = ( (ValParam*)desc.getParam(is2DStr()) )->getBoolValue();
    if ( !is2Dsurvey )
	desc.setNrOutputs( Seis::UnknowData, 9 );
    else
	desc.setNrOutputs( Seis::UnknowData, 7 );
}


Reference::Reference( Desc& desc_ )
        : Provider( desc_ )
{
    mGetBool( is2d_, is2DStr() );
}


bool Reference::computeData( const DataHolder& output, const BinID& relpos,
			      int z0, int nrsamples ) const
{
    float step = refstep ? refstep : SI().zStep();
    Coord coord;
    if ( outputinterest[0] || outputinterest[1] ) 
	coord = SI().transform( currentbid );
    
    for ( int idx=0; idx<nrsamples; idx++ )
    {
	if ( outputinterest[0] )
	    output.series(0)->setValue(idx, coord.x);
	if ( outputinterest[1] )
	    output.series(1)->setValue(idx, coord.y);
	if ( outputinterest[2] )
	{
	    float val;
	    if ( seldata_->type_ == Seis::Table )
	    {
		float offset = SI().zRange(0).start<0 ? SI().zRange(0).start :0;
		val = ( z0 + idx ) * step + offset;
	    }
	    else
		val = possiblevolume->zrg.start + idx * step;
	    
	    output.series(2)->setValue(idx, val);
	}

	if ( !is2d_ )
	{
	    if ( outputinterest[3] )
		output.series(3)->setValue(idx, currentbid.inl);
	    if ( outputinterest[4] )
		output.series(4)->setValue(idx, currentbid.crl);
	    if ( outputinterest[5] )
		output.series(5)->setValue(idx, z0 + idx + 1);
	    if ( outputinterest[6] )
	    {
		int val = currentbid.inl - SI().inlRange(0).start + 1;
		output.series(6)->setValue(idx, val);
	    }
	    if ( outputinterest[7] )
	    {
	    int val = currentbid.crl - SI().crlRange(0).start + 1;
	    output.series(7)->setValue(idx, val);
	    }
	    if ( outputinterest[8] )
	    {
		int val = z0 - mNINT(SI().zRange(0).start/step) + idx + 1;
		output.series(8)->setValue(idx, val);
	    }
	}
	else
	{
	    if ( outputinterest[3] )
		output.series(3)->setValue(idx, currentbid.crl);
	    if ( outputinterest[4] )
		output.series(4)->setValue(idx, z0 + idx + 1);
	    if ( outputinterest[5] )
	    {
		int val = currentbid.crl - desiredvolume->hrg.start.crl + 1;
		output.series(5)->setValue(idx, val);
	    }
	    if ( outputinterest[6] )
	    {
		int val = z0 - mNINT(SI().zRange(0).start/step) + idx + 1;
		output.series(6)->setValue(idx, val);
	    }
	}
    }

    return true;
}
			
};
