/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : Oct 1999
-*/

static const char* rcsID mUsedVar = "$Id$";


#include "expvardip.h"
#include "periodicvalue.h"
#include "attribprovider.h"
#include "seistrc.h"
#include "ptrman.h"
#include "arrayndimpl.h"
#include "arrayndalgo.h"
#include "attribdescsetproc.h"


bool MinVarianceDipAttrib::Parameters::update()
{
    if ( constantvel && velocity.getMode() == AttribParameter::Disabled )
    {
	velocity.setMode( AttribParameter::Required );
	return true;
    }
    else if ( !constantvel && velocity.getMode() == AttribParameter::Required )
    {
	velocity.setMode( AttribParameter::Disabled );
	return true;
    }

    return false;
}


MinVarianceDipAttrib::MinVarianceDipAttrib( Parameters* params )
    : sg( -params->size/2, params->size/2 )
    , velocity( params->velocity )
    , constantvel( params->constantvel )
    , common( 0 )
    , resolution( params->resolution )
    , sz( params->size )
    , fast( params->fast )
    , stepout( params->size/2, params->size/2 )
    , AttribCalc( new MinVarianceDipAttrib::Task( *this ) )
{ 
    params->fillDefStr( desc );
    delete params;
    AttribInputSpec* spec = new AttribInputSpec;
    spec->setDesc( "Data on which the dip should be calculated");
    inputspec += spec;

    if ( !constantvel )
    {
	spec = new AttribInputSpec;
	spec->setDesc( "Velocity" );
	inputspec += spec;
    }

    prepareInputs();
}


MinVarianceDipAttrib::~MinVarianceDipAttrib() {}


bool MinVarianceDipAttrib::init()
{
    inldist = common->inldist*common->stepoutstep.inl;
    crldist = common->crldist*common->stepoutstep.crl;

    return AttribCalc::init();
} 


AttribCalc::Task* MinVarianceDipAttrib::Task::clone() const 
{ return new MinVarianceDipAttrib::Task(calculator); }


void MinVarianceDipAttrib::Task::set( float t1_, int nrtimes_, float step_,
				      const AttribCalc::Task::Input* inp,
				      const TypeSet<float*>& outp_)
{
    t1 = t1_;
    nrtimes = nrtimes_;
    step = step_;
    input = inp;
    inldips = outp_[0];
    crldips = outp_[1];

    indata.setSize( calculator.sz, calculator.sz, calculator.sz );

}


bool MinVarianceDipAttrib::Task::Input::set( const BinID& pos,
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
							pos.crl + idy );
	    if ( !trc ) return false;
	    trcs->set( idx + hsz, idy + hsz, trc);
	}
    }

    if ( !calculator.constantvel )
    {
	veltrc = inputproviders[1]->getTrc( pos.inl, pos.crl );
	if ( !veltrc ) 
	    return 0;

	velattrib = inputproviders[1]->attrib2component( inputattribs[1] );
    }
    
    dataattrib = inputproviders[0]->attrib2component( inputattribs[0] );
    return true;
}


int MinVarianceDipAttrib::Task::nextStep()
{
    const MinVarianceDipAttrib::Task::Input* inp = 
			(const MinVarianceDipAttrib::Task::Input*) input;
    const int sz = calculator.sz; 
    const int hsz = sz/2;
    const int hsz2 = hsz*hsz;
    const SeisTrc* veltrc = inp->veltrc;
    const float inldist = calculator.inldist;
    const float crldist = calculator.crldist;
    const float unitdist = inldist>crldist?crldist:inldist;

    Array2DImpl<SeisTrc*>& trcs = *inp->trcs;
    const float inpstep = ((SeisTrc*) trcs.get(0,0))->info().sampling.step;

    const int dataattrib = inp->dataattrib;
    const int velattrib = inp->velattrib;
 
    const int resolution = calculator.resolution;
    float deltaangle = M_PI / resolution;

    for ( int idx=0; idx<nrtimes; idx++ )
    {
	const float curt = t1 + idx*step;
	float velocity = veltrc ? veltrc->getValue( curt, velattrib )
				: calculator.velocity; 

        for ( int idi=0; idi<sz; idi++ )
        {
	    for ( int idc=0; idc<sz; idc++ )
	    {
		for ( int idt=0; idt<sz; idt++ )
		{
		    float val = trcs.get(idi,idc)->getValue(curt+
						  inpstep*(idt-hsz),dataattrib);
                    indata.set(idi,idc,idt,val);
                }
            }
	}

	float minangle0;
	float minangle1;
	float minvariance = mUndefValue;

	float angle1 = 0;

	for ( int id0=0; id0<resolution; id0++ )
	{
	    float angle0 = 0;

	    for ( int id1=0; id1<(id0?resolution:1); id1++ )
	    {
		float sum = 0;
		float sqsum = 0;
		int nrsamples = 0;

		for ( int pos0=-hsz; pos0<hsz; pos0++ )
		{
		    for ( int pos1=-hsz; pos1<hsz; pos1++ )
		    {
			if ( pos0*pos0+pos1*pos1 > hsz2 )
			    continue;

			float p = pos0 * unitdist;
			float q = pos1 * unitdist;
			float qz = q * sin(angle1);
			float qic = q * cos(angle1);


			float rho = atan2( qic, p ); 
			float r = Math::Sqrt( qic*qic + p*p );

			float crl = r*cos(angle0+rho);
			float inl = r*sin(angle0+rho);

			float val;

			if ( !calculator.fast )
			    val = Array3DInterpolate( indata,
						      hsz+inl/inldist,
					 	      hsz+crl/crldist,
						      hsz+qz/velocity/inpstep);
			else
			    val = trcs.get(hsz+mNINT32(inl/inldist),
					hsz+mNINT32(crl/crldist))
					->getValue(curt+qz/velocity,dataattrib);

			sum += val;
			sqsum += val * val;

			nrsamples ++;
		    }
		}	

		float var = (sqsum - sum * sum / nrsamples)/ (nrsamples -1);

		if ( var < minvariance )
		{
		    minvariance = var;
		    minangle1 = angle1;
		    minangle0 = angle0;
		}

		angle0 += deltaangle;
	    }

	    angle1 += deltaangle;
	}

	float vcrl = cos( minangle0 );
	float vinl = sin( minangle0 );
	float vz = 0;

	float cosa1 = cos(minangle1);
	float wcrl = -cosa1*vinl;	// cos( angle0+M_PI/2) = - sin(angle0)
	float winl = cosa1*vcrl;	// sin( angle0+M_PI/2) = cos(angle0)
	float wz = sin(minangle1);

	float ncrl = (vinl*wz-winl*vz); 	// The planes normal vector
	float ninl = (-vcrl*wz+wcrl*vz);
	float nz = (vcrl*winl-wcrl*vinl); 

	const float dipfact = calculator.dipFactor();
	if ( inldips ) inldips[idx] = ninl/-nz/velocity*dipfact;	
	if ( crldips ) crldips[idx] = ncrl/-nz/velocity*dipfact;
    }
		
    return 0;
}
