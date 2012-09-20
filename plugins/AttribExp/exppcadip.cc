/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : Oct 1999
-*/

static const char* rcsID mUnusedVar = "$Id$";


#include "exppcadip.h"
#include "sorting.h"
#include "simpnumer.h"
#include "attribprovider.h"
#include "seistrc.h"
#include "linsolv.h"
#include "ptrman.h"
#include "arrayndimpl.h"
#include "arrayndutils.h"
#include "attribdescsetproc.h"

#include <stdio.h>


PCADipAttrib::PCADipAttrib( Parameters* params )
    : sg( params->samplegate )
    , common( 0 )
    , stepout( params->stepout )
    , fraction( params->fraction )
    , AttribCalc( new PCADipAttrib::Task( *this ) )
{ 
    params->fillDefStr( desc );
    delete params;
    AttribInputSpec* spec = new AttribInputSpec;
    spec->setDesc( "Data on which the dip should be calculated");
    inputspec += spec;

    prepareInputs();
}


PCADipAttrib::~PCADipAttrib() {}


bool PCADipAttrib::init()
{
    inldist = common->inldist*common->stepoutstep.inl;
    crldist = common->crldist*common->stepoutstep.crl;

    return AttribCalc::init();
} 


AttribCalc::Task* PCADipAttrib::Task::clone()const
{ return new PCADipAttrib::Task(calculator); }



void PCADipAttrib::Task::set( float t1_, int nrtimes_, float step_,
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


float PCADipAttrib::Task::getMinEigenVector( const int* inlines,
					     const int* crlines,
				 	     const int* timepos,
					     const int* indexes,
					     int first, int last,
					     double& ev0,
					     double& ev1,
					     double& ev2)
{
    int inl_sum = 0;
    int crl_sum = 0;
    int t_sum = 0;

    int inl_inl_sum = 0;
    int inl_crl_sum = 0;
    int inl_t_sum = 0;

    int crl_crl_sum = 0;
    int crl_t_sum = 0;

    int t_t_sum = 0;


    const int nrobs = last - first +1;
    bool dprint = false;

    for ( int idy=first; idy<=last; idy++ )
    {
	const int orgpos = indexes[idy];

	int inl = inlines[orgpos];
	int crl = crlines[orgpos];
	int t = timepos[orgpos];

	if ( dprint ) printf("%d\t%d\t%d\n", inl, crl, t );
	
	inl_sum += inl;
	crl_sum += crl;
	t_sum += t;

	inl_inl_sum += inl*inl;
	inl_crl_sum += inl*crl;
	inl_t_sum += inl*t;

	crl_crl_sum += crl*crl;
	crl_t_sum += crl*t;

	t_t_sum += t*t;
    }

    float cov00 = nrobs*inl_inl_sum - inl_sum*inl_sum;
    float cov10 = nrobs*inl_crl_sum - inl_sum*crl_sum;
    float cov20 = nrobs*inl_t_sum - inl_sum*t_sum;

    float cov01 = cov10;

    float cov11 = nrobs*crl_crl_sum - crl_sum*crl_sum;
    float cov21 = nrobs*crl_t_sum - crl_sum*t_sum;
    
    float cov02 = cov20;

    float cov12 = cov21;

    float cov22 = nrobs*t_t_sum - t_sum*t_sum;

    const double c = ((double)cov02*cov11*cov20)
		    -((double)cov01*cov12*cov20)
		    -((double)cov02*cov10*cov21)
		    +((double)cov00*cov12*cov21)
		    +((double)cov01*cov10*cov22)
		    -((double)cov00*cov11*cov22);

    const double b = -((double)cov01*cov10)
		    +((double)cov00*cov11)
		    -((double)cov02*cov20)
		    -((double)cov12*cov21)
		    +((double)cov00*cov22)
		    +((double)cov11*cov22);

    const double a = -cov00-cov11-cov22;

    double eig0, eig1, eig2;
    solve3DPoly( a, b, c, eig0, eig1, eig2 );

    float eig0_abs = fabs(eig0);
    float eig1_abs = fabs(eig1);
    float eig2_abs = fabs(eig2);

    float eigmin = eig0_abs;
    if ( eig1_abs<eigmin ) { eigmin = eig1_abs; }
    if ( eig2_abs<eigmin ) { eigmin = eig2_abs; }

    float eigmax = eig0_abs;
    if ( eig1_abs>eigmax ) { eigmax = eig1_abs; }
    if ( eig2_abs>eigmax ) { eigmax = eig2_abs; }

    cov00 -= eigmin;
    cov11 -= eigmin;
    cov22 -= eigmin;

    ev0 = mUndefValue; ev1 = mUndefValue; ev2 = mUndefValue;

    if ( !mIsZero(cov00,mDefEps)+!mIsZero(cov10,mDefEps)+!mIsZero(cov20,mDefEps) > 1 )
    {
	float tmp;
	if ( mIsZero( cov00 ,mDefEps) )
	{
	    mSWAP( cov00, cov20, tmp );
	    mSWAP( cov01, cov21, tmp );
	    mSWAP( cov02, cov22, tmp );
	 }

	 cov11-=cov10*cov01/cov00;
	 cov12-=cov10*cov02/cov00;
	 cov10=0;

	 cov21-=cov20*cov01/cov00;
	 cov22-=cov20*cov02/cov00;
	 cov20=0;
    }

    float c1, c2;

    if ( !mIsZero(cov11,mDefEps) || !mIsZero(cov12,mDefEps) != 2 )	
    {
	c1 = cov11;
	c2 = cov12;
    }	
    else 
    {
	c1 = cov21;
	c2 = cov22;
    }
    
    if ( mIsZero(c1,mDefEps) )
    {
	ev2 = 0;
	ev1 = 1;
    }
    else if ( mIsZero( c2 ,mDefEps) )
    {
	ev1 = 0;
	ev2 = 1;
    }
    else
    {
	ev1 = 1;
	ev2 = - c1 / c2;
    }
    
    if ( !mIsZero(cov00,mDefEps) )
	ev0 = (-ev1*cov01 - ev2*cov02)/ cov00;
    else
	ev0 = 1;
   
    float length = Math::Sqrt(ev0*ev0+ev1*ev1+ev2*ev2);
    ev0 /= length;     
    ev1 /= length;     
    ev2 /= length;     

    return eigmax/(1+eigmin);
}


bool PCADipAttrib::Task::Input::set( const BinID& pos,
			     const ObjectSet<AttribProvider>& inputproviders,
			     const TypeSet<int>& inputattribs,
			     const TypeSet<float*>&)
{
    const BinID stepout = calculator.stepout;

    if ( !trcs )
	trcs = new Array2DImpl<SeisTrc*>( stepout.inl*2+1, stepout.crl*2+1 );

    for ( int idx=-stepout.inl; idx<=stepout.inl; idx++ )
    { 
	for ( int idy=-stepout.crl; idy<=stepout.crl; idy++ )
	{
	    SeisTrc* trc = inputproviders[0]->getTrc( 	pos.inl + idx, 
							pos.crl + idy );
	    trcs->set(idx+stepout.inl, idy+stepout.crl,trc);
	}
    }
   
    attribute = inputproviders[0]->attrib2component( inputattribs[0] ); 
    return true;
}


int PCADipAttrib::Task::nextStep()
{
    const PCADipAttrib::Task::Input* inp = 
			(const PCADipAttrib::Task::Input*) input;

    const BinID stepout = calculator.stepout;
    const int fraction = calculator.fraction;
    const Interval<int> sg = calculator.sg;

    const int inlsz = (stepout.inl*2 + 1);
    const int crlsz = (stepout.crl*2 + 1);

    const BinID stepout2(stepout.inl*stepout.inl,stepout.crl*stepout.crl);
    const int sg2 = sg.start*sg.start;
    const int limit = stepout2.inl*stepout2.inl*sg2;

    const int nrtraces = inlsz*crlsz; 
    const double dmaxweight = Math::Sqrt( (double)(stepout2.inl+stepout2.inl+sg2) );
    const int maxweight = mNINT32(dmaxweight);
    const int maxnrsamples = nrtraces * (sg.width()+1)*maxweight;


    Array2DImpl<SeisTrc*>& trcs = *inp->trcs;
    int attribute = inp->attribute;

    float inpstep;
    int nrtrcs = 0;

    for ( int idx=0; idx<trcs.info().getSize(0); idx++ )
    {
	for ( int idy=0; idy<trcs.info().getSize(0); idy++ )
	{
	    if ( trcs.get(idx,idy) )
	    {
		inpstep = ((SeisTrc*) trcs.get(idx,idy))->info().sampling.step;
		nrtrcs ++;
	    }
	}
    }

    const float dipfact = calculator.dipFactor();
    const float inlfactor = inpstep * dipfact / calculator.inldist;
    const float crlfactor = inpstep * dipfact / calculator.crldist;

    ArrPtrMan<float>	data =  new float[maxnrsamples];
    ArrPtrMan<int>	indexes = new int[maxnrsamples];
    ArrPtrMan<int>	inlines = new int[maxnrsamples];
    ArrPtrMan<int>	crlines = new int[maxnrsamples];
    ArrPtrMan<int>	timepos = new int[maxnrsamples];

    int pos = 0;
    for ( int idt=sg.start; idt<=sg.stop; idt++ )
    {
	for ( int idi=-stepout.inl; idi<=stepout.inl; idi++ )
	{
	    for ( int idc=-stepout.crl; idc<=stepout.crl; idc++ )
	    {
		int dist2 = idt*idt*stepout2.inl*stepout2.crl+
			     idi*idi*stepout2.crl*sg2+
			     idc*idc*stepout2.inl*sg2;

		if ( dist2 > limit )
		    continue;

		    int weight = 1+mNINT32(maxweight *
					(1 -Math::Sqrt(((double)dist2)/limit)));

		for ( int idx=0; idx<weight; idx++ )
		{
		    crlines[pos] = idc;
		    inlines[pos] = idi;
		    timepos[pos] = idt;

		    pos ++;
		}
	    }
	}
    }

    TypeSet<float> ev0s;
    TypeSet<float> ev1s;
    TypeSet<float> ev2s;
    TypeSet<float> weights;

    for ( int idx=0; idx<nrtimes; idx++ )
    {
	const float curt = t1 + idx*step;
	pos = 0;
        for ( int idt=sg.start; idt<=sg.stop; idt++ )
        {
	    for ( int idi=-stepout.inl; idi<=stepout.inl; idi++ )
	    {
		for ( int idc=-stepout.crl; idc<=stepout.crl; idc++ )
		{
		    int dist2 = idt*idt*stepout2.inl*stepout2.crl+
				 idi*idi*stepout2.crl*sg2+
				 idc*idc*stepout2.inl*sg2;

		    if ( dist2 > limit )
			continue;

		    int weight = 1+mNINT32(maxweight *
					(1 -Math::Sqrt(((double)dist2)/limit)));

		    const SeisTrc* trc = trcs.get(idi+stepout.inl,
						     idc+stepout.crl);
		    float d = trc ? trc->getValue(curt+ inpstep*(idt),attribute)
				  : 0;
		
		    for ( int idy=0; idy<weight; idy++ )
		    {
			data[pos] = d;
			indexes[pos] = pos;
			pos++;
		    }
		}
            }
	}

	quickSort( (float*)data, (int*)indexes, pos );

	ev0s.erase();
	ev1s.erase();
	ev2s.erase();
	weights.erase();

	double hev0, hev1, hev2;
	float high = getMinEigenVector(inlines,crlines,timepos,indexes,
                                        mNINT32((float)pos*(100-fraction)/100),
                                        pos-1,
                                        hev0, hev1, hev2);

	double lev0, lev1, lev2;
	float low = getMinEigenVector(inlines,crlines,timepos,indexes,
					0,
                                        mNINT32(pos*fraction/100),
                                        lev0, lev1, lev2);

	float ev0 = high > low ? hev0 : lev0;
	float ev1 = high > low ? hev1 : lev1;
	float ev2 = high > low ? hev2 : lev2;

	float inldip = mIsZero(ev2,mDefEps) ? 10e10 : ev0 * inlfactor / ev2;
	float crldip = mIsZero(ev2,mDefEps) ? 10e10 : ev1 * crlfactor / ev2;

	if ( inldips ) inldips[idx] = inldip;
	if ( crldips ) crldips[idx] = crldip;
    }

    return 0;
}
