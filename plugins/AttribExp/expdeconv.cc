/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : Oct 1999
-*/

static const char* rcsID mUnusedVar = "$Id$";

#include "expdeconv.h"
#include "attribprovider.h"
#include "attribdescsetproc.h"
#include "seistrc.h"


DeConvolveAttrib::DeConvolveAttrib( Parameters* param )
    : steering( param->steering )
    , gate( param->gate.start/zFactor(), param->gate.stop/zFactor() )
    , windowtype( (ArrayNDWindow::WindowType ((int) param->window)) )
    , window( 0 )
    , neighbourhood( param->neighbourhood )
    , pos1( param->pos1 )
    , common( 0 )
    , AttribCalc( new DeConvolveAttrib::Task( *this ) )
{ 
    param->fillDefStr( desc );
    delete param;

    AttribInputSpec* spec = new AttribInputSpec;
    spec->setDesc("Data on which the DeConvolve should be performed");
    inputspec += spec;

    if ( steering )
    {
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
    }

    prepareInputs();
}


DeConvolveAttrib::~DeConvolveAttrib()
{
    delete window;
}


bool DeConvolveAttrib::init()
{
    inpstep = inputproviders[0]->getStep();

    int sz = mNINT32( gate.width() / inpstep )+1;
    fftsz = FFT::_getNearBigFastSz(sz);

    const Array1DInfoImpl ai(fftsz);

    df = FFT::getDf( inpstep, fftsz );
    fft.setInputInfo(ai);
    fft.setDir(true);
    fft.init();

    ifft.setInputInfo(ai);
    ifft.setDir(false);
    ifft.init();

    window = new ArrayNDWindow(ai,true,windowtype);

    return AttribCalc::init();
}

DeConvolveAttrib::Task::Task( const DeConvolveAttrib& calculator_ )
    : out0( 0 )
    , out1( 0 )
    , tracesegments( 0 )
    , spectrum0( 0 )
    , spectrum1( 0 )
    , spectrumaverage( 0 )
    , spectrumoutput( 0 )
    , traceoutput( 0 )
    , calculator( calculator_ )
{ } 


DeConvolveAttrib::Task::~Task()
{
    if ( tracesegments )
    {
	ArrayNDIter iter( tracesegments->info() );

	do
	{
	    delete tracesegments->get( iter.getPos() );

	} while ( iter.next() );

	delete tracesegments;
    }

    delete spectrum0;
    delete spectrum1;
    delete spectrumaverage;
    delete spectrumoutput;
    delete traceoutput;
}


DeConvolveAttrib::Task::Input::~Input()
{ delete trcs; }


AttribCalc::Task* DeConvolveAttrib::Task::clone() const
{ return new DeConvolveAttrib::Task(calculator); }
    

bool DeConvolveAttrib::Task::Input::set(const BinID& pos, 
	const ObjectSet<AttribProvider>& inputproviders, const TypeSet<int>&
	inputattribs, const TypeSet<float*>&)
{
    const BinID neighbourhood = calculator.neighbourhood;

    if ( !trcs )
	trcs = new Array2DImpl<SeisTrc*>( 2*neighbourhood.inl+1,
					  2*neighbourhood.crl+1 );

    for ( int idx=-neighbourhood.inl; idx<=neighbourhood.inl; idx++ )
    {
	for ( int idy=-neighbourhood.crl; idy<=neighbourhood.crl; idy++ )
	{
	    SeisTrc* trc = inputproviders[0]->getTrc(   pos.inl + idx,
							pos.crl + idy );

	    trcs->set(idx+neighbourhood.inl, idy+neighbourhood.crl, trc );
	}
    }

    if ( !trcs->get(neighbourhood.inl,neighbourhood.crl) ) return false;

    dataattrib = inputproviders[0]->attrib2component( inputattribs[0] );

    if ( calculator.steering )
    {
	inldiptrc = inputproviders[1]->getTrc( pos.inl, pos.crl );
	if ( !inldiptrc ) return false;

	crldiptrc = inputproviders[2]->getTrc( pos.inl, pos.crl );
	if ( !crldiptrc ) return false;

	inldipattrib = inputproviders[1]->attrib2component( inputattribs[1] );
	crldipattrib = inputproviders[2]->attrib2component( inputattribs[2] );
    }

    return true;
} 




int DeConvolveAttrib::Task::nextStep()
{
    const float inpstep = calculator.inpstep;
    const BinID neighbourhood = calculator.neighbourhood;
    const BinID size = BinID(neighbourhood.inl*2+1,neighbourhood.crl*2+1);
    const bool steering = calculator.steering;
    const float inldist = calculator.common->inldist;
    const float crldist = calculator.common->crldist;
    const Interval<float> gate = calculator.gate;
    const FFT& fft = calculator.fft;
    const FFT& ifft = calculator.ifft;
    const int fftsz = calculator.fftsz;
    const BinID pos1 = calculator.pos1;
    const int midpos = -mNINT32(gate.start/inpstep);

    bool calcaverage = out1;

    if ( !tracesegments )
    {
	tracesegments = new Array2DImpl<Array1D<float_complex>*>
				(size.inl,size.crl);

	spectrum0 = new Array1DImpl<float_complex>( fftsz );
	spectrum1 = new Array1DImpl<float_complex>( fftsz );
	spectrumaverage = new Array1DImpl<float_complex>( fftsz );
	spectrumoutput = new Array1DImpl<float_complex>( fftsz );
	traceoutput = new Array1DImpl<float_complex>( fftsz );

	for ( int idx=-neighbourhood.inl; idx<=neighbourhood.inl; idx++ )
	{
	    for ( int idy=-neighbourhood.crl; idy<=neighbourhood.crl; idy++ )
	    {
		Array1D<float_complex>* segm =
					new Array1DImpl<float_complex>( fftsz );

		tracesegments->set(idx+neighbourhood.inl,
				   idy+neighbourhood.crl, segm );
	    }
	}
    }

    DeConvolveAttrib::Task::Input* inp = (DeConvolveAttrib::Task::Input*)input;
    const Array2DImpl<SeisTrc*>& trcs = *inp->trcs;
    const SeisTrc* inldiptrc = inp->inldiptrc;
    const SeisTrc* crldiptrc = inp->crldiptrc;

    const int dataattrib = inp->dataattrib;
    const int inldipattrib = inp->inldipattrib;
    const int crldipattrib = inp->crldipattrib;

    for ( int pos=0; pos<nrtimes; pos++ )
    {
	const float curt = t1+pos*step;
	for ( int idx=-neighbourhood.inl; idx<=neighbourhood.inl; idx++ )
	{
	    for ( int idy=-neighbourhood.crl; idy<=neighbourhood.crl; idy++ )
	    {
		if ( !calcaverage && (idx || idy) &&
				 !(idx==pos1.inl && idy==pos1.crl) ) continue;

		Array1D<float_complex>* segm =
		    tracesegments->get(idx+neighbourhood.inl,
				       idy+neighbourhood.crl);

		SeisTrc* trc = trcs.get(idx+neighbourhood.inl,
					idy+neighbourhood.crl);

		if ( trc )
		{	
		    float steeringoff = 0;

		    if ( steering )
		    {
			float inldip = inldiptrc->getValue(curt, inldipattrib);
			float crldip = crldiptrc->getValue(curt, crldipattrib);

			steeringoff = idx*inldist*inldip+
					idy*crldist*crldip;
		    }

		    for ( int idz=0; idz<fftsz; idz++ )
		    {
			float t = curt + gate.start + idz*inpstep + steeringoff;
			float val = trc->getValue(t, dataattrib);

			segm->set( idz, float_complex( val,0 ));
		    }

		    calculator.window->apply( segm );
		}
		else
		{	
		    for ( int idz=0; idz<fftsz; idz++ )
			segm->set( idz, float_complex( 0,0 ));
		}

	    }
	}

	if ( calcaverage )
	{
	    for ( int idz=0; idz<fftsz; idz++ )
		spectrumaverage->set( idz, float_complex( 0,0 ));

	    for ( int idx=0; idx<size.inl; idx++ )
	    {
		for ( int idy=0; idy<size.crl; idy++ )
		{
		    fft.transform( *tracesegments->get(idx,idy), *spectrum0 );

		    for ( int idz=0; idz<fftsz; idz++ )
		    {
			float_complex nv = spectrumaverage->get( idz ) +
					   spectrum0->get( idz );

			spectrumaverage->set( idz, nv );
		    }
		}
	    }
	}

	fft.transform( *tracesegments->get(neighbourhood.inl,neighbourhood.crl),
		 *spectrum0 );

	if ( out0 )
	{
	    fft.transform( *tracesegments->get(neighbourhood.inl+pos1.inl,
					 neighbourhood.crl+pos1.crl),
					 *spectrum1 );

	    for ( int idz=0; idz<fftsz; idz++ )
	    {
		float_complex numerator =
				spectrum0->get(idz) - spectrum1->get(idz);
		float_complex denominator =
				spectrum0->get(idz) + spectrum1->get(idz);

		spectrumoutput->set( idz, numerator/denominator );
	    }

	    ifft.transform( *spectrumoutput, *traceoutput );

	    out0[pos] = traceoutput->get(midpos).real();
	}

	if ( out1 )
	{
	    for ( int idz=0; idz<fftsz; idz++ )
	    {
		float_complex numerator = spectrum0->get(idz);
		float_complex denominator = spectrumaverage->get(idz);

		spectrumoutput->set( idz, numerator/denominator );
	    }

	    ifft.transform( *spectrumoutput, *spectrumoutput );

	    out1[pos] = spectrumoutput->get(midpos).real();
	}

    }
    
    return 0;
}


