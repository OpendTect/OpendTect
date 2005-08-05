/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : Oct 1999
-*/

static const char* rcsID = "$Id: frequencyattrib.cc,v 1.3 2005-08-05 13:05:02 cvsnanne Exp $";

#include "frequencyattrib.h"
#include "attribdataholder.h"
#include "attribdesc.h"
#include "attribfactory.h"
#include "attribparam.h"
#include "datainpspec.h"
#include "simpnumer.h"
#include "samplfunc.h"
#include "genericnumer.h"
#include "strmprov.h"
#include "filepath.h"
#include "stats.h"
#include <iostream>
#include <stdio.h>


namespace Attrib
{

void Frequency::initClass()
{
    Desc* desc = new Desc( attribName() );
    desc->ref();

    ZGateParam* gate = new ZGateParam(gateStr());
    gate->setLimits( Interval<float>(-mLargestZGate,mLargestZGate) );
    desc->addParam( gate );

    BoolParam* normalize = new BoolParam( normalizeStr() );
    normalize->setDefaultValue(false);
    desc->addParam( normalize );

    EnumParam* window = new EnumParam(windowStr());
    window->addEnums(ArrayNDWindow::WindowTypeNames);
    window->setDefaultValue("CosTaper5");
    desc->addParam(window);

    BoolParam* dumptofile = new BoolParam( dumptofileStr() );
    dumptofile->setDefaultValue( false );
    dumptofile->setValue( false );
    dumptofile->setRequired( false );
    desc->addParam( dumptofile );

    desc->setNrOutputs( Seis::UnknowData, 8 );

    desc->addInput( InputSpec("Real data",true) );
    desc->addInput( InputSpec("Imag data",true) );

    PF().addDesc( desc, createInstance );
    desc->unRef();
}


Provider* Frequency::createInstance( Desc& ds )
{
    Frequency* res = new Frequency( ds );
    res->ref();
    if ( !res->isOK() )
    {
	res->unRef();
	return 0;
    }

    res->unRefNoDelete();
    return res;
}


Frequency::Frequency( Desc& desc_ )
    : Provider( desc_ )
    , fftisinit( false )
    , fftsz( -1 )
    , fft( false )
    , window( 0 )
    , signal( 0 )
    , timedomain(0)
    , freqdomain(0)
{
    if ( !isOK() ) return;

    mGetFloatInterval( gate, gateStr() );
    gate.start = gate.start / zFactor(); gate.stop = gate.stop / zFactor();

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
		filename += Stat_getIndex(mUndefIntVal);
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
}


const Interval<float>* Frequency::reqZMargin( int inp, int ) const
{ return &gate; }


bool Frequency::computeData(const DataHolder& output, const BinID& relpos,
				int t0, int nrsamples ) const
{
    if ( !fftisinit )
    {
	const_cast<Frequency*>(this)->samplegate = 
	    Interval<int>(mNINT(gate.start/refstep), mNINT(gate.stop/refstep));

	const_cast<Frequency*>(this)->fftsz = 
	    		FFT::_getNearBigFastSz((samplegate.width()+1)*3);
	const_cast<Frequency*>(this)->fft.setInputInfo(Array1DInfoImpl(fftsz));
	const_cast<Frequency*>(this)->fft.setDir(true);
	const_cast<Frequency*>(this)->fft.init();

	const_cast<Frequency*>(this)->window = 
	    new ArrayNDWindow( Array1DInfoImpl(samplegate.width()+1),
			    false, (ArrayNDWindow::WindowType) windowtype);

	const_cast<Frequency*>(this)->df = FFT::getDf( refstep, fftsz );
	const_cast<Frequency*>(this)->
	    signal.setInfo( Array1DInfoImpl( samplegate.width()+1 ) );
	const_cast<Frequency*>(this)->
	    timedomain.setInfo( Array1DInfoImpl( fftsz ));
	const_cast<Frequency*>(this)->
	    freqdomain.setInfo( Array1DInfoImpl( fftsz ));

	const_cast<Frequency*>(this)->fftisinit = true;
    }
    	
    const int sz = samplegate.width()+1;
    for ( int idx=0; idx<fftsz; idx++)
	const_cast<Frequency*>(this)->timedomain.set( idx, 0 );

    for ( int idx=0; idx<nrsamples; idx++ )
    {
	int cursample = t0 + idx;
	int tempsamp = cursample + samplegate.start;
	for ( int idy=0; idy<sz; idy ++ )
	{
	    const float real = redata->item(0)->value(tempsamp-redata->t0_);
	    const float imag = -imdata->item(0)->value(tempsamp-imdata->t0_);

	    const_cast<Frequency*>(this)->
		signal.set( idy, float_complex( real, imag ));

	    tempsamp ++;
	}

	window->apply( &const_cast<Frequency*>(this)->signal );
	removeBias( &const_cast<Frequency*>(this)->signal );
	for ( int idy=0; idy<sz; idy++ )
	    const_cast<Frequency*>(this)->
		timedomain.set( sz+idy, signal.get(idy) );

	TypeSet<float> freqdomainpower(fftsz,0);
	fft.transform( timedomain, const_cast<Frequency*>(this)->freqdomain );

	int maxnr = -1;
	float maxval = 0;
	float sum = 0;

	for ( int idy=0; idy<=fftsz/2; idy++ )
	{
	    float val = abs( freqdomain.get(idy) );
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
	    }

	    if ( val2<maxval ) continue;

	    maxval = val2;
	    maxnr = idy;
	}

	if ( mIsZero(sum,mDefEps) )
	{
	    if ( outputinterest[0] )	output.item(0)->setValue(idx,0);
	    if ( outputinterest[1] )    output.item(1)->setValue(idx,0);
	    if ( outputinterest[2] )    output.item(2)->setValue(idx,0);
	    if ( outputinterest[3] )    output.item(3)->setValue(idx,0);
	    if ( outputinterest[4] )    output.item(4)->setValue(idx,0);
	    if ( outputinterest[5] )    output.item(5)->setValue(idx,0);
	    if ( outputinterest[6] )    output.item(6)->setValue(idx,0);
	    if ( outputinterest[7] )    output.item(7)->setValue(idx,0);

	    continue;
	}

	if ( normalize )
	{
	    for ( int idy=0; idy<=fftsz/2; idy++ )
	    {
		freqdomainpower[idy] /= sum;
	    }

	    maxval /= sum;
	    sum = 1;
	}

	const float halfsum = sum / 2;
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
		    output.item(2)->setValue(idx, idy * df);
	    }

	    wf += hf;
	    wf2 += hf*freq;	    
	}

	if ( outputinterest[0] ) output.item(0)->setValue(idx, maxnr * df);
	if ( outputinterest[1] ) output.item(1)->setValue(idx, wf / sum);
	if ( outputinterest[3] ) output.item(3)->setValue(idx, wf2 / sum);
	if ( outputinterest[4] ) output.item(4)->setValue(idx, maxval);
	if ( outputinterest[5] ) output.item(5)->setValue(idx, sa);
	if ( outputinterest[6] ) 
	    output.item(6)->setValue(idx, 1+(maxval-sum)/(maxval+sum));
	if ( outputinterest[7] ) output.item(7)->setValue(idx, aqf);
    }

    return true;
}


bool Frequency::getInputOutput( int input, TypeSet<int>& res ) const
{
    return Provider::getInputOutput( input, res );
}


bool Frequency::getInputData( const BinID& relpos, int idx )
{
    redata = inputs[0]->getData( relpos, idx );
    if ( !redata ) return false;

    imdata = inputs[1]->getData( relpos, idx );
    if ( !imdata ) return false;

    return true;
}

};//namespace

 
