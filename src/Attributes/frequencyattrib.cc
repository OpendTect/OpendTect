/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : Oct 1999
-*/

static const char* rcsID = "$Id: frequencyattrib.cc,v 1.14 2007-10-18 14:08:05 cvshelene Exp $";

#include "frequencyattrib.h"
#include "arrayndimpl.h"
#include "arrayndutils.h"
#include "attribdataholder.h"
#include "attribdesc.h"
#include "attribfactory.h"
#include "attribparam.h"
#include "filepath.h"
#include "genericnumer.h"
#include "oddirs.h"
#include "samplfunc.h"
#include "simpnumer.h"
#include "statrand.h"
#include "strmprov.h"

#include <iostream>
#include <stdio.h>


namespace Attrib
{

mAttrDefCreateInstance(Frequency)
    
void Frequency::initClass()
{
    mAttrStartInitClass

    ZGateParam* gate = new ZGateParam( gateStr() );
    gate->setLimits( Interval<float>(-mLargestZGate,mLargestZGate) );
    gate->setDefaultValue( Interval<float>(-28, 28) );
    desc->addParam( gate );

    BoolParam* normalize = new BoolParam( normalizeStr() );
    normalize->setDefaultValue( false );
    desc->addParam( normalize );

    EnumParam* window = new EnumParam( windowStr() );
    window->addEnums( ArrayNDWindow::WindowTypeNames );
    window->setDefaultValue( ArrayNDWindow::CosTaper5 );
    desc->addParam( window );

    BoolParam* dumptofile = new BoolParam( dumptofileStr() );
    dumptofile->setDefaultValue( false );
    dumptofile->setValue( false );
    dumptofile->setRequired( false );
    desc->addParam( dumptofile );

    desc->addInput( InputSpec("Real data",true) );
    desc->addInput( InputSpec("Imag data",true) );
    desc->setNrOutputs( Seis::UnknowData, 8 );

    mAttrEndInitClass
}


Frequency::Frequency( Desc& ds )
    : Provider(ds)
    , fftisinit(false)
    , fftsz(-1)
    , fft(false)
    , window(0)
    , signal(0)
    , timedomain(0)
    , freqdomain(0)
{
    if ( !isOK() ) return;

    mGetFloatInterval( gate, gateStr() );
    gate.scale( 1/zFactor() );

    mGetBool( normalize, normalizeStr() );
    
    int wtype;
    mGetEnum( wtype, windowStr() );
    windowtype = (ArrayNDWindow::WindowType)wtype;

    mGetBool( dumptofile, dumptofileStr() );
}


Frequency::~Frequency()
{
    if ( dumptofile )
    {
	for ( int idx=0; idx<dumpset.size(); idx++ )
	{
	    const char* data = (dumpset.get(idx)).buf();
	    if ( data && *data )
	    {
		FilePath fp( GetDataDir() );
		BufferString filename( "frequency." );
		filename += Stats::RandGen::getIndex(mUdf(int));
		filename = fp.add( filename ).fullPath();
		StreamData sd = StreamProvider( filename ).makeOStream();
		if ( sd.usable() )
		{
		    BufferString bfstr;
		    desc.getDefStr(bfstr);
		    *sd.ostrm << bfstr << '\n';
		    *sd.ostrm << data;
		}
	    }
	}
    }

    delete window;
    delete signal;
    delete timedomain;
    delete freqdomain;
}


const Interval<float>* Frequency::reqZMargin( int inp, int ) const
{ return &gate; }


bool Frequency::getInputOutput( int input, TypeSet<int>& res ) const
{
    return Provider::getInputOutput( input, res );
}


bool Frequency::getInputData( const BinID& relpos, int zintv )
{
    redata = inputs[0]->getData( relpos, zintv );
    if ( !redata ) return false;

    imdata = inputs[1]->getData( relpos, zintv );
    if ( !imdata ) return false;

    realidx_ = getDataIndex( 0 );
    imagidx_ = getDataIndex( 1 );

    return true;
}


bool Frequency::computeData( const DataHolder& output, const BinID& relpos,
			     int z0, int nrsamples, int threadid ) const
{
    Frequency* myself = const_cast<Frequency*>(this);
    if ( !fftisinit )
    {
	myself->samplegate = 
	    Interval<int>(mNINT(gate.start/refstep),mNINT(gate.stop/refstep));

	myself->fftsz = FFT::_getNearBigFastSz((samplegate.width()+1)*3);
	myself->fft.setInputInfo(Array1DInfoImpl(fftsz));
	myself->fft.setDir(true);
	myself->fft.init();

	myself->window = 
	    new ArrayNDWindow( Array1DInfoImpl(samplegate.width()+1),
			       false, (ArrayNDWindow::WindowType)windowtype );

	myself->df = FFT::getDf( refstep, fftsz );
	myself->signal = new Array1DImpl<float_complex>( samplegate.width()+1 );
	myself->timedomain = new Array1DImpl<float_complex>( fftsz );
	myself->freqdomain = new Array1DImpl<float_complex>( fftsz );
	myself->fftisinit = true;
    }
    	
    const int sz = samplegate.width()+1;
    for ( int idx=0; idx<fftsz; idx++)
	myself->timedomain->set( idx, 0 );

    for ( int idx=0; idx<nrsamples; idx++ )
    {
	const int cursample = z0 + idx;
	int tempsamp = cursample + samplegate.start;
	for ( int idy=0; idy<sz; idy ++ )
	{
	    const float real = redata->series(realidx_)->
					value(tempsamp-redata->z0_);
	    const float imag = -imdata->series(imagidx_)->
					value(tempsamp-imdata->z0_);

	    myself->signal->set( idy, float_complex(real,imag) );
	    tempsamp++;
	}

	window->apply( myself->signal );
	removeBias( myself->signal );
	for ( int idy=0; idy<sz; idy++ )
	    myself->timedomain->set( sz+idy, signal->get(idy) );

	TypeSet<float> freqdomainpower( fftsz, 0 );
	fft.transform( *timedomain, *myself->freqdomain );

	int maxnr = -1;
	float maxval = 0;
	float sum = 0;

	for ( int idy=0; idy<=fftsz/2; idy++ )
	{
	    float val = abs( freqdomain->get(idy) );
	    float val2 = val * val;
	    freqdomainpower[idy] = val2;

	    sum += val2;

	    if ( dumptofile )
	    {
		BufferString dump;
		BinID pos = currentbid;
		dump += pos.inl; dump += " "; dump += pos.crl; dump += " ";
		dump += cursample*refstep; dump += " "; 
		dump += df*idy; dump += " "; dump += val2; dump += "\n";
		myself->dumpset.add( dump );
	    }

	    if ( val2<maxval ) continue;

	    maxval = val2;
	    maxnr = idy;
	}

	ArrayValueSeries<float,float> arr( freqdomainpower.arr(), false );
	FreqFunc func( arr, fftsz );
	float exactpos = findExtreme( func, false, maxnr-1, maxnr+1 );
	if ( !mIsUdf(exactpos) )
	    maxval = func.getValue( exactpos );
	else
	    exactpos = maxnr;

	const int outidx = z0 - output.z0_ + idx;
	if ( mIsZero(sum,mDefEps) )
	{
	    if ( outputinterest[0] ) output.series(0)->setValue( outidx, 0 );
	    if ( outputinterest[1] ) output.series(1)->setValue( outidx, 0 );
	    if ( outputinterest[2] ) output.series(2)->setValue( outidx, 0 );
	    if ( outputinterest[3] ) output.series(3)->setValue( outidx, 0 );
	    if ( outputinterest[4] ) output.series(4)->setValue( outidx, 0 );
	    if ( outputinterest[5] ) output.series(5)->setValue( outidx, 0 );
	    if ( outputinterest[6] ) output.series(6)->setValue( outidx, 0 );
	    if ( outputinterest[7] ) output.series(7)->setValue( outidx, 0 );
	    continue;
	}

	if ( normalize )
	{
	    for ( int idy=0; idy<=fftsz/2; idy++ )
		freqdomainpower[idy] /= sum;

	    maxval /= sum;
	    sum = 1;
	}

	const float halfsum = sum/2;
	float sa = 0;
	float mfsum = 0;
	bool mfisset = false;
	float aqf = 0;

	float wf = 0;
	float wf2 = 0;


	for ( int idy=0; idy<=fftsz/2; idy++ )
	{
	    float height = freqdomainpower[idy];
	    float freq = idy * df;
	    float hf = height*freq;

	    if ( idy>maxnr ) 
	    {
		sa += height;
		aqf += hf;
	    }

	    if ( !mfisset && (mfsum+=height)>halfsum )
	    {
		mfisset = true;
		if ( outputinterest[2] ) 
		    output.series(2)->setValue( outidx, idy*df );
	    }

	    wf += hf;
	    wf2 += hf*freq;
	}

	if ( outputinterest[0] ) 
	    output.series(0)->setValue( outidx, exactpos*df );
	if ( outputinterest[1] ) output.series(1)->setValue( outidx, wf/sum );
	if ( outputinterest[3] ) output.series(3)->setValue( outidx, wf2/sum );
	if ( outputinterest[4] ) output.series(4)->setValue( outidx, maxval );
	if ( outputinterest[5] ) output.series(5)->setValue( outidx, sa );
	if ( outputinterest[6] ) 
	    output.series(6)->setValue( outidx, 1+(maxval-sum)/(maxval+sum) );
	if ( outputinterest[7] ) output.series(7)->setValue( outidx, aqf );
    }

    return true;
}

}; // namespace Attrib
