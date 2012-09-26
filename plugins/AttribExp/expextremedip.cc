/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : Oct 1999
-*/

static const char* rcsID mUsedVar = "$Id$";


#include "expextremedip.h"
#include "simpnumer.h"
#include "attribprovider.h"
#include "seistrc.h"
#include "ptrman.h"
#include "attribdescsetproc.h"


ExtremeDipAttrib::ExtremeDipAttrib( Parameters* params )
    : AttribCalc( new ExtremeDipAttrib::Task( *this ) )
    , sg( -1, 1 )
    , common( 0 )
    , stepout( 1, 1)
    , maxdip( params->maxdip )
{ 
    params->fillDefStr( desc );
    delete params;

    AttribInputSpec* spec = new AttribInputSpec;
    spec->setDesc( "Data on which the dip should be calculated");
    inputspec += spec;

    prepareInputs();
}


ExtremeDipAttrib::~ExtremeDipAttrib( ) {}


bool ExtremeDipAttrib::init()
{
    inldist = common->inldist*common->stepoutstep.inl;
    crldist = common->crldist*common->stepoutstep.crl;

    return AttribCalc::init();
} 


void ExtremeDipAttrib::Task::set( float t1_, int nrtimes_, float step_,
				      const AttribCalc::Task::Input* inp,
				      const TypeSet<float*>& outp_)
{
    t1 = t1_;
    nrtimes = nrtimes_;
    step = step_;
    input = inp;
    inldips = outp_[0];
    crldips = outp_[1];
}

AttribCalc::Task* ExtremeDipAttrib::Task::clone() const
{ 
    return new ExtremeDipAttrib::Task( calculator );
}


bool ExtremeDipAttrib::Task::Input::set( const BinID& pos,
			     const ObjectSet<AttribProvider>& inputproviders,
			     const TypeSet<int>& inputattribs,
			     const TypeSet<float*>&)
{
    const int sz = 3;
    const int hsz = sz / 2;

    if ( !trcs )
	trcs = new Array2DImpl<SeisTrc*>( sz, sz );

    for ( int idx=-hsz; idx<=hsz; idx++ )
    { 
	for ( int idy=-hsz; idy<=hsz; idy++ )
	{
	    SeisTrc* trc = inputproviders[0]->getTrc( 	pos.inl + idx, 
							pos.crl + idy);
	    if ( !trc ) return false;
	    trcs->set( idx + hsz, idy + hsz, trc);
	}
    }

    attribute = inputproviders[0]->attrib2component( inputattribs[0] );
    
    return true;
}


int ExtremeDipAttrib::Task::nextStep()
{
    const ExtremeDipAttrib::Task::Input* inp = 
			(const ExtremeDipAttrib::Task::Input*) input;

    const int sz = 3;
    const int sz2 = sz * sz;
    const int hsz = sz / 2;

    const float dipfact = calculator.dipFactor();
    const float inpstep = inp->trcs->get(0,0)->info().sampling.step;
    const int attribute = inp->attribute;

    TypeSet<float> vals(sz,0);
    TypeSet<float> maxpositions(sz,0);

    for ( int idx=0; idx<nrtimes; idx++ )
    {
	const float curt = t1 + idx*step;
	if ( !curt )
	{
	    if ( inldips ) inldips[idx] = 0;
	    if ( crldips ) crldips[idx] = 0;
	    continue;
	}

	float timegrad;
	bool havetimegrad = false;
	bool ispeak = true;
	SeisTrc* ctrc = inp->trcs->get( hsz, hsz );
	vals[0] = ctrc->getValue( curt - inpstep, attribute );
	vals[1] = ctrc->getValue( curt, attribute );
	vals[2] = ctrc->getValue( curt + inpstep, attribute );
	if ( (vals[0] <= vals[1] && vals[1] <= vals[2]) ||
	     (vals[0] >= vals[1] && vals[1] >= vals[2]) )
	{
	    if ( inldips ) inldips[idx] = mUndefValue;
	    if ( crldips ) crldips[idx] = mUndefValue;
	    continue;
	}
	else
	{
	    maxpositions[1] = getMaxPos( vals[0], vals[1], vals[2] );
	    ispeak = vals[1] > vals[0] && vals[1] > vals[2];
	}


	float inldip = mUndefValue;
	float crldip = mUndefValue;

	for ( int idi=0; idi<sz; idi++ )
	{
	    if ( idi == 1 ) continue;
	    SeisTrc* trcp = inp->trcs->get(idi,1);
	    maxpositions[idi] = getRealMaxPos( trcp, curt, inpstep, ispeak );
	}

	float centralmaxpos = maxpositions[1];
	float previnlmaxpos = maxpositions[0];
	float nextinlmaxpos = maxpositions[2];
	    
	int nrinlvals = 0;

	if ( !mIsUndefined( previnlmaxpos ) )
	{
	    inldip = (centralmaxpos-previnlmaxpos)*inpstep*dipfact/
						calculator.inldist;
	    nrinlvals ++;
	}

	if ( !mIsUndefined( nextinlmaxpos ) )
	{
	    inldip += (nextinlmaxpos-centralmaxpos)*inpstep*dipfact/
						calculator.inldist;
	    nrinlvals ++;
	    inldip /= nrinlvals;
	}

	for ( int idc=0; idc<sz; idc++ )
	{
	    if ( idc == 1 ) continue;
	    SeisTrc* trcp = inp->trcs->get(1,idc);
	    maxpositions[idc] = getRealMaxPos( trcp, curt, inpstep, ispeak );
	}

	centralmaxpos = maxpositions[1];
	float prevcrlmaxpos = maxpositions[0];
	float nextcrlmaxpos = maxpositions[2];
	
	int nrcrlvals = 0;

	if ( !mIsUndefined( prevcrlmaxpos ) )
	{
	    crldip = (centralmaxpos-prevcrlmaxpos)*inpstep*dipfact/
						calculator.crldist;
	    nrcrlvals ++;
	}

	if ( !mIsUndefined( nextcrlmaxpos ) )
	{
	    crldip += (nextcrlmaxpos-centralmaxpos)*inpstep*dipfact/
						calculator.crldist;
	    nrcrlvals ++;
	    crldip /= nrcrlvals;
	}


	if ( inldips ) inldips[idx] = inldip;
	if ( crldips ) crldips[idx] = crldip;
    }

    float maxdip = calculator.maxdip;
    maxdip = 10000;
    int idx0 = -1;
    int idx1 = -1;
    float val0, val1;
    bool first = true;
    for ( int idx=0; idx<nrtimes; idx++ )
    {
	if ( inldips[idx] < maxdip )
	{
	    if ( first )
	    {
		idx0 = idx;
		val0 = inldips[idx];
		first = false;
	    }
	    else
	    {
		idx1 = idx;
		val1 = inldips[idx];
	    }
	}

	if ( idx0>=0 && idx1>=0 )
	{
	    for ( int pol=idx0+1; pol<idx1; pol++ )
	    {
		float grad = (val1-val0) / (idx1-idx0);
		inldips[pol] = val0 + grad * (pol-idx0);
	    }

	    idx0 = idx1; idx1 = -1;
	    val0 = val1;
	    continue;
	}
    }

    idx0 = -1;
    idx1 = -1;
    first = true;
    for ( int idx=0; idx<nrtimes; idx++ )
    {
	if ( crldips[idx] < maxdip )
	{
	    if ( first )
	    {
		idx0 = idx;
		val0 = crldips[idx];
		first = false;
	    }
	    else
	    {
		idx1 = idx;
		val1 = crldips[idx];
	    }
	}

	if ( idx0>=0 && idx1>=0 )
	{
	    for ( int pol=idx0+1; pol<idx1; pol++ )
	    {
		float grad = (val1-val0) / (idx1-idx0);
		crldips[pol] = val0 + grad * (pol-idx0);
	    }

	    idx0 = idx1; idx1 = -1;
	    val0 = val1;
	    continue;
	}
    }

    return 0;
}


float ExtremeDipAttrib::Task::getMaxPos( float val1, float val2,
					     	     float val3 ) const
{
    // y(x) = ax2 + bx + c
    // y'(x) = 0;  =>  2ax + b = 0   => x = -b / 2a;

    float a = ( val1 - 2*val2 + val3 ) / 2;
    float b = ( val3 - val1 ) / 2;

    return a ? -b / (2*a) : mUndefValue;
}


float ExtremeDipAttrib::Task::getRealMaxPos( SeisTrc* trc, float curt,
						 float inpstep, bool ispeak )
{
    float time0 = curt - inpstep;
    float time1 = curt;
    float time2 = curt + inpstep;
    float shift = 0;
    for ( int ids=0; ids<7; ids++ )
    {
	float vals0 = trc->getValue( time0, 0 );
	float vals1 = trc->getValue( time1, 0 );
	float vals2 = trc->getValue( time2, 0 );
	if ( (vals1 > vals0 && vals1 > vals2) || 
		(vals1 < vals0 && vals1 < vals2) )
	    return getMaxPos( vals0, vals1, vals2 )+shift;
	else if ( vals0 < vals1 && vals1 < vals2 )
	{
	    time0 += ispeak ? inpstep : -inpstep;
	    time1 += ispeak ? inpstep : -inpstep;
	    time2 += ispeak ? inpstep : -inpstep;
	    shift += ispeak ? 1 : -1;
	}
	else
	{
	    time0 += ispeak ? -inpstep : inpstep;
	    time1 += ispeak ? -inpstep : inpstep;
	    time2 += ispeak ? -inpstep : inpstep;
	    shift += ispeak ? -1 : 1;
	}
    }

    return mUndefValue;
}
