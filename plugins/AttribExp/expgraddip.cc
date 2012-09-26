/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : Oct 1999
-*/

static const char* rcsID mUsedVar = "$Id$";


#include "expgraddip.h"
#include "sorting.h"
#include "simpnumer.h"
#include "attribprovider.h"
#include "seistrc.h"
#include "ptrman.h"
#include "attribdescsetproc.h"


GradientDipAttrib::GradientDipAttrib( Parameters* params )
    : sg( -params->size/2, params->size/2 )
    , sz( params->size )
    , common( 0 )
    , stepout( params->size/2, params->size/2 )
    , AttribCalc( new GradientDipAttrib::Task( *this ) )
{ 
    params->fillDefStr( desc );
    delete params;

    AttribInputSpec* spec = new AttribInputSpec;
    spec->setDesc( "Data on which the dip should be calculated");
    inputspec += spec;

    prepareInputs();
}


GradientDipAttrib::~GradientDipAttrib( ) {}


bool GradientDipAttrib::init()
{
    inldist = common->inldist*common->stepoutstep.inl;
    crldist = common->crldist*common->stepoutstep.crl;

    return AttribCalc::init();
} 


AttribCalc::Task* GradientDipAttrib::Task::clone() const
{ return new GradientDipAttrib::Task( calculator ); }


void GradientDipAttrib::Task::set( float t1_, int nrtimes_, float step_,
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


bool GradientDipAttrib::Task::Input::set( const BinID& pos,
			     const ObjectSet<AttribProvider>& inputproviders,
			     const TypeSet<int>& inputattribs,
			     const TypeSet<float*>&)
{
    const int sz = calculator.sz;
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


int GradientDipAttrib::Task::nextStep()
{
    const GradientDipAttrib::Task::Input* inp = 
			(const GradientDipAttrib::Task::Input*) input;

    const int sz = calculator.sz;
    const int sz2 = sz * sz;
    const int hsz = sz / 2;

    const float inpstep = inp->trcs->get(0,0)->info().sampling.step;
    const int attribute = inp->attribute;

    TypeSet<float> timescale(sz,0);
    TypeSet<float> inlscale(sz,0);
    TypeSet<float> crlscale(sz,0);

    for ( int idx=0; idx<sz; idx++ )
    {
	timescale[idx] = idx*inpstep*calculator.dipFactor();
	inlscale[idx] = idx*calculator.inldist;
	crlscale[idx] = idx*calculator.crldist;
    }

    TypeSet<float> vals(sz,0);
    TypeSet<float> statvals( sz2,0 );

    for ( int idx=0; idx<nrtimes; idx++ )
    {
	const float curt = t1 + idx*step;

	for ( int idi=0; idi<sz; idi++ )
	{
	    for ( int idc=0; idc<sz; idc++ )
	    {
		float grad;
	
		SeisTrc* trcp = inp->trcs->get(idi,idc);
		SeisDataTrc trc( *trcp, attribute );
		getGradient( timescale, trc, sz,
			     0, trc.getIndex( curt )- hsz, &grad );

		statvals[idi*sz+idc] = grad;
	    }	
	}

	sort_idxabl( statvals, sz2 );
	float timegrad = (sz2 % 2)
			? statvals[sz2/2]
			: ((statvals[sz2/2] + statvals[sz2/2+1]) / 2);
			     
	for ( int idc=0; idc<sz; idc++ )
	{
	    for ( int idt=0; idt<sz; idt++ )
	    {
		for ( int idi=0; idi<sz; idi++ )
		{
		    SeisTrc* trcp = inp->trcs->get(idi,idc);
		    SeisDataTrc trc( *trcp, attribute );
		    vals[idi] = trc[trc.getIndex(curt)+idt-hsz];
		}
 
		float grad;

		getGradient( inlscale, vals, sz, 0, 0, &grad );

		statvals[idt*sz+idc] = grad;
	    }	
	}

	sort_idxabl( statvals, sz2 );
	float inlgrad = (sz2 % 2)
			? statvals[sz2/2]
			: ((statvals[sz2/2] + statvals[sz2/2+1]) / 2);
			     
	for ( int idi=0; idi<sz; idi++ )
	{
	    for ( int idt=0; idt<sz; idt++ )
	    {
		for ( int idc=0; idc<sz; idc++ )
		{
		    SeisTrc* trcp = inp->trcs->get(idi,idc);
		    SeisDataTrc trc( *trcp, attribute );
		    vals[idc] = trc[trc.getIndex(curt)+idt-hsz];
		}
 
		float grad;

		getGradient( crlscale, vals, sz, 0, 0, &grad );

		statvals[idt*sz+idi] = grad;
	    }	
	}

	sort_idxabl( statvals, sz2 );
	float crlgrad = (sz2 % 2)
			? statvals[sz2/2]
			: ((statvals[sz2/2] + statvals[sz2/2+1]) / 2);
			    
	if ( inldips ) inldips[idx] = -inlgrad / timegrad; 
	if ( crldips ) crldips[idx] = -crlgrad / timegrad; 
    }

    return 0;
}
