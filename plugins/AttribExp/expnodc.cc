/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : Oct 1999
-*/

static const char* rcsID = "$Id: expnodc.cc,v 1.4 2003-11-07 12:21:56 bert Exp $";

#define mEPSILON 1E-9

#include "expnodc.h"
#include "attribprovider.h"
#include "seistrc.h"
#include "simpnumer.h"

NoDCAttrib::NoDCAttrib( Parameters* param )
    : AttribCalc( new NoDCAttrib::Task( *this ) )
{ 
    param->fillDefStr( desc );
    delete param;
    inputspec += new AttribInputSpec;

    prepareInputs();
}


NoDCAttrib::~NoDCAttrib( )
{ }


AttribCalc::Task* NoDCAttrib::Task::clone() const
{ return new NoDCAttrib::Task(calculator); }



bool NoDCAttrib::Task::Input::set( const BinID& pos,
			     const ObjectSet<AttribProvider>& inputproviders,
			     const TypeSet<int>& inputattribs,
			     const TypeSet<float*>&)
{
    trc = inputproviders[0]->getTrc( pos.inl, pos.crl );
    attribute = inputproviders[0]->attrib2component( inputattribs[0] );

    return trc;
}


int NoDCAttrib::Task::nextStep()
{
    if ( !outp ) return 0;

    const NoDCAttrib::Task::Input* inp = 
			(const NoDCAttrib::Task::Input*) input;

    const SeisTrc* trcp = inp->trc;
    const SeisDataTrc trc( *trcp, inp->attribute );

    float sum = 0;

    const int firstidx = trc.getIndex( t1 );
    if ( mIS_ZERO( t1 - trc.getX( firstidx )) &&
	 mIS_ZERO( step - trc.step()) )
    {
	int pos = firstidx;

	for ( int idy=0; idy<nrtimes; idy++ )
	{
	   sum += trc[pos++];
	}	

	sum /= nrtimes;
	pos = firstidx;
	
	for ( int idx=0; idx<nrtimes; idx++)
	{
	    outp[idx] =  trc[pos++] - sum;
	}
    }
    else
    {
	for ( int idy=0; idy<nrtimes; idy++ )
	{
	    const float curt = t1+idy*step;
	    sum += trc.getValue( curt );
	}	

	sum /= nrtimes;

	for ( int idx=0; idx<nrtimes; idx++)
	{
	    const float curt = t1 + idx*step;
	    outp[idx] = trc.getValue( curt ) - sum;
	}
    }

    return 0;
}
