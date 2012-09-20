/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : Oct 1999
-*/

static const char* rcsID mUnusedVar = "$Id$";


#include "expdiscfilter.h"
#include "periodicvalue.h"
#include "attribprovider.h"
#include "seistrc.h"
#include "ptrman.h"
#include "arrayndutils.h"
#include "attribdescsetproc.h"


bool DiscFilterAttrib::Parameters::update()
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


DiscFilterAttrib::DiscFilterAttrib( Parameters* param )
    : sg( -param->radius, param->radius )
    , velocity( param->velocity )
    , constantvel( param->constantvel )
    , radius( param->radius )
    , fast( param->fast )
    , stepout( param->radius, param->radius )
    , AttribCalc( new DiscFilterAttrib::Task( *this ) )
{ 
    param->fillDefStr( desc );
    delete param;

    AttribInputSpec* spec = new AttribInputSpec;
    spec->setDesc( "Data to filter");
    inputspec += spec;

    spec = new AttribInputSpec;
    spec->setDesc("In-line dip");
    spec->forbiddenDts += Seis::Ampl;
    spec->forbiddenDts += Seis::Frequency;
    spec->forbiddenDts += Seis::Phase;
    spec->forbiddenDts += Seis::AVOGradient;
    spec->forbiddenDts += Seis::UnknowData;
    inputspec += spec;

    spec = new AttribInputSpec;
    spec->setDesc("Cross-line dip");
    spec->forbiddenDts += Seis::Ampl;
    spec->forbiddenDts += Seis::Frequency;
    spec->forbiddenDts += Seis::Phase;
    spec->forbiddenDts += Seis::AVOGradient;
    spec->forbiddenDts += Seis::UnknowData;
    inputspec += spec;

    if ( !constantvel )
    {
	spec = new AttribInputSpec;
	spec->setDesc( "Velocity" );
	inputspec += spec;
    }

    prepareInputs();
}


DiscFilterAttrib::~DiscFilterAttrib( ) {}


Seis::DataType DiscFilterAttrib::dataType(int val,
				const TypeSet<Seis::DataType>& dts) const
{
    if ( val == mDiscFilterStdDev || val == mDiscFilterVar )
        return Seis::UnknowData;

    return dts[0];
}


bool DiscFilterAttrib::init()
{
    inldist = common->inldist*common->stepoutstep.inl;
    crldist = common->crldist*common->stepoutstep.crl;

    return AttribCalc::init();
} 


AttribCalc::Task* DiscFilterAttrib::Task::clone() const
{ return new DiscFilterAttrib::Task(calculator); }


void DiscFilterAttrib::Task::set( float t1_, int nrtimes_, float step_,
				      const AttribCalc::Task::Input* inp,
				      const TypeSet<float*>& outp_)
{
    t1 = t1_;
    nrtimes = nrtimes_;
    step = step_;
    input = inp;
    avg = outp_[mDiscFilterAvg];
    med = outp_[mDiscFilterMed];
    stddev = outp_[mDiscFilterStdDev];
    variance = outp_[mDiscFilterVar];
    min = outp_[mDiscFilterMin];
    max = outp_[mDiscFilterMax];
    max = outp_[mDiscFilterMostFreq];

    indata.setSize( calculator.radius*2+1, calculator.radius*2+1, calculator.radius*2+1 );

}


bool DiscFilterAttrib::Task::Input::set( const BinID& pos,
			     const ObjectSet<AttribProvider>& inputproviders,
			     const TypeSet<int>& inputattribs,
			     const TypeSet<float*>&)
{
    const int radius = calculator.radius;

    if ( !trcs )
	trcs = new Array2DImpl<const SeisTrc*>( radius*2+1, radius*2+1 );

    int nrtrcs = 0;
    for ( int idx=-radius; idx<=radius; idx++ )
    { 
	for ( int idy=-radius; idy<=radius; idy++ )
	{
	    SeisTrc* trc = inputproviders[0]->getTrc( 	pos.inl + idx, 
							pos.crl + idy );

	    if ( !calculator.fast && !trc ) return false;
	    if ( trc ) nrtrcs++;
	    trcs->set(idx+radius,idy+radius,trc);
	}
    }

    if ( !nrtrcs ) return false;

    inldiptrc = inputproviders[1]->getTrc( pos.inl, pos.crl);
    crldiptrc = inputproviders[2]->getTrc( pos.inl, pos.crl);

    if ( !calculator.constantvel )
    {
	veltrc = inputproviders[3]->getTrc( pos.inl, pos.crl);
	if ( !veltrc ) return 0;

	velattrib = inputproviders[3]->attrib2component( inputattribs[3] );
    }

    dataattrib = inputproviders[0]->attrib2component( inputattribs[0] );

    if ( inldiptrc )
	inldipattrib = inputproviders[1]->attrib2component( inputattribs[1] );

    if ( crldiptrc )
	crldipattrib = inputproviders[2]->attrib2component( inputattribs[2] );

    return true;
}


int DiscFilterAttrib::Task::nextStep()
{
    const DiscFilterAttrib::Task::Input* inp = 
			(const DiscFilterAttrib::Task::Input*) input;

    const int radius = calculator.radius; 
    const int sz = radius*2+1;
    const int radius2 = radius*radius;
    const SeisTrc* inldiptrc = inp->inldiptrc;
    const SeisTrc* crldiptrc = inp->crldiptrc;
    const SeisTrc* veltrc = inp->veltrc;
    const float inldist = calculator.inldist;
    const float crldist = calculator.crldist;
    const float unitdist = inldist>crldist?crldist:inldist;

    int const dataattrib = inp->dataattrib;
    int const velattrib = inp->velattrib;
    int const inldipattrib = inp->inldipattrib;
    int const crldipattrib = inp->crldipattrib;

    Array2DImpl<const SeisTrc*>& trcs = *inp->trcs;

    const SeisTrc* dttrc = 0;
    for ( int idx=0; idx<sz; idx++ )
    {
	for ( int idy=0; idy<sz; idy++ )
	{
	    dttrc = trcs.get(idx,idy);
	    if ( dttrc ) break;
	}

	if ( dttrc ) break;
    }
	

    const float inpstep = dttrc->info().sampling.step;
 
    for ( int idx=0; idx<nrtimes; idx++ )
    {
	const float curt = t1 + idx*step;
	float velocity = veltrc ? veltrc->getValue( curt, velattrib )
				: calculator.velocity; 

	if ( !calculator.fast )
	{
	    for ( int idi=0; idi<sz; idi++ )
	    {
		for ( int idc=0; idc<sz; idc++ )
		{
		    for ( int idt=0; idt<sz; idt++ )
		    {
			float val = trcs.get(idi,idc)->getValue(curt+
					  inpstep* (idt-radius),dataattrib);
			indata.set(idi,idc,idt,val);
		    }
		}
	    }
	}

	float inldip = inldiptrc ? inldiptrc->getValue(curt,inldipattrib) : 0;
	float crldip = crldiptrc ? crldiptrc->getValue(curt,crldipattrib) : 0;

	float angle0 = atan2( crldip, -inldip );
	float poldip = Math::Sqrt(inldip*inldip+crldip*crldip);
	float angle1 = atan(poldip*velocity/calculator.dipFactor());

	stat.clear();

	for ( int pos0=-radius; pos0<radius; pos0++ )
	{
	    for ( int pos1=-radius; pos1<radius; pos1++ )
	    {
		if ( pos0*pos0+pos1*pos1 > radius2 )
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
		{
		    val = Array3DInterpolate( indata,
					      radius+inl/inldist,
					      radius+crl/crldist,
					      radius+qz/velocity/inpstep);
		    stat += val;
		}
		else
		{
		    const SeisTrc* trc = trcs.get(radius+mNINT32(inl/inldist),
					      radius+mNINT32(crl/crldist));

		    if ( trc )
		    {
			val = trc->getValue(curt+qz/velocity,dataattrib); 
			stat += val;
		    }
		}
	    }
	}	

	if ( avg )	avg[idx] = stat.mean();
	if ( med )	med[idx] = stat.median();
	if ( stddev )	stddev[idx] = stat.stdDev();
	if ( variance )	variance[idx] = stat.variance();
	if ( min )	min[idx] = stat.min();
	if ( max )	max[idx] = stat.max();
	if ( mostfreq )	mostfreq[idx] = stat.mostFreq();

    }
		
    return 0;
}
