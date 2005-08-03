/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : Oct 1999
-*/

static const char* rcsID = "$Id: coherencyattrib.cc,v 1.4 2005-08-03 07:35:58 cvshelene Exp $";


#include "coherencyattrib.h"
#include "attribdataholder.h"
#include "attribdesc.h"
#include "attribfactory.h"
#include "attribparam.h"
#include "datainpspec.h"
#include "ptrman.h"
#include "simpnumer.h"


namespace Attrib
{

void Coherency::initClass()
{
    Desc* desc = new Desc( attribName(), updateDesc );
    desc->ref();

    IntParam* type = new IntParam( typeStr() );
    type->setLimits( Interval<int>(1,2) );
    type->setDefaultValue("2");
    desc->addParam( type );

    ZGateParam* gate = new ZGateParam(gateStr());
    gate->setLimits( Interval<float>(-mUndefValue, mUndefValue) );
    gate->setDefaultValue( Interval<float>(-40,40) );
    desc->addParam( gate );

    FloatParam* maxdip = new FloatParam( maxdipStr() );
    maxdip->setLimits( Interval<float>(0,mUndefValue) );
    maxdip->setDefaultValue("250");
    desc->addParam( maxdip );

    FloatParam* ddip = new FloatParam( ddipStr() );
    ddip->setLimits( Interval<float>(0,mUndefValue) );
    ddip->setDefaultValue("10");
    desc->addParam( ddip );

    BinIDParam* stepout = new BinIDParam(stepoutStr());
    stepout->setDefaultValue(BinID(1,1));
    stepout->setRequired(false);
    desc->addParam( stepout );

    desc->setNrOutputs( Seis::UnknowData, 3 );

    InputSpec inpspec( "Data on which the Coherency should be measured", true );
    desc->addInput( inpspec );

    InputSpec imagspec( "Imag data for Coherency", false );
    desc->addInput( imagspec );

    desc->init();

    PF().addDesc( desc, createInstance );
    desc->unRef();
}


Provider* Coherency::createInstance( Desc& ds )
{
    Coherency* res = new Coherency( ds );
    res->ref();

    if ( !res->isOK() )
    {
	res->unRef();
	return 0;
    }

    res->unRefNoDelete();
    return res;
}


void Coherency::updateDesc( Desc& desc )
{
    const ValParam* type = (ValParam*)desc.getParam(typeStr());
    if (type-> getIntValue() == 2)
	desc.inputSpec(1).enabled = true;
}


Coherency::Coherency( Desc& desc_ )
    : Provider( desc_ )
    , redh ( 0 )
    , imdh ( 0 )
{ 
    if ( !isOK() ) return;
    
    mGetInt( type, typeStr() );
    
    mGetFloatInterval( gate, gateStr() );
    gate.start = gate.start / zFactor(); gate.stop = gate.stop / zFactor();

    mGetFloat( maxdip, maxdipStr() );
    maxdip = maxdip/dipFactor();

    mGetFloat( ddip, ddipStr() );
    ddip = ddip/dipFactor();

    mGetBinID( stepout, stepoutStr() );
    stepout.inl = abs( stepout.inl ); stepout.crl = abs( stepout.crl );
}


float Coherency::calc1( float s1, float s2, const Interval<int>& sg,
			   const DataHolder& dh1, const DataHolder& dh2 ) const
{
    float xsum = 0;
    float sum1 = 0;
    float sum2 = 0;

    for ( int s = sg.start; s <= sg.stop; s++ )
    {
	ValueSeriesInterpolator<float> interp1(dh1.nrsamples_-1);
	ValueSeriesInterpolator<float> interp2(dh2.nrsamples_-1);
	float val1 = interp1.value( *(dh1.item(0)), s1-dh1.t0_ + s );
	float val2 = interp2.value( *(dh2.item(0)), s2-dh2.t0_ + s );
	xsum += val1 * val2;
	sum1 += val1 * val1;
	sum2 += val2 * val2;
    }

    return xsum / sqrt( sum1 * sum2 );
}


float Coherency::calc2( float s, const Interval<int>& rsg,
			      float inldip, float crldip,
			      const Array2DImpl<DataHolder*>& re,
			      const Array2DImpl<DataHolder*>& im ) const
{
    const int inlsz = re.info().getSize(0);
    const int crlsz = re.info().getSize(1);

    float numerator = 0;
    float denominator = 0;

    for ( int idx=rsg.start; idx<= rsg.stop; idx++ )
    {
	float realsum = 0;
	float imagsum = 0;

	for ( int idy=0; idy<inlsz; idy++ )
	{
	    float inlpos = (idy - (inlsz/2)) * distinl;
	    for ( int idz=0; idz<crlsz; idz++ )
	    {
		ValueSeriesInterpolator<float> 
		    	interp( re.get(idy,idz)->nrsamples_-1 );
		float crlpos = (idz - (crlsz/2)) * distcrl;
		float place = s - re.get(idy,idz)->t0_ + idx + 
		    	     (inlpos*inldip)/refstep + (crlpos*crldip)/refstep;
		    
		float real = 
		    interp.value( *(re.get(idy,idz)->item(0)), place );

		float imag =  
		   - interp.value( *(im.get(idy,idz)->item(0)), place );
		
		realsum += real;
		imagsum += imag;

		denominator += real * real + imag*imag;
	    }
	}

	numerator += realsum * realsum + imagsum * imagsum;
    }	

    return denominator? numerator / ( inlsz * crlsz * denominator ) : 0 ;	
}


bool Coherency::computeData(const DataHolder& output,
			    const BinID& relpos,
			    int t0, int nrsamples ) const
{
    bool yn;
    BinID step = inputs[0]-> getStepoutStep(yn);
	
    const_cast<Coherency*>(this)->distinl = fabs(inldist()*step.inl);
    const_cast<Coherency*>(this)->distcrl = fabs(crldist()*step.crl);
    return type == 1 ? computeData1(output, t0, nrsamples) 
		     : computeData2(output, t0, nrsamples);
}

bool Coherency::computeData1( const DataHolder& output,
				int t0, int nrsamples ) const
{
    Interval<int> samplegate( mNINT(gate.start/refstep),
				mNINT(gate.stop/refstep) );
    for ( int idx=0; idx<nrsamples; idx++ )
    {
	float cursamp = t0 + idx;
	float maxcoh = -1;
	float dipatmax;

	float curdip = -maxdip;

	while ( curdip <= maxdip )
	{
	    float coh = calc1( cursamp, cursamp + (curdip * distinl)/refstep,
				samplegate, *inputdata[0], *inputdata[1] );

	    if ( coh > maxcoh ) { maxcoh = coh; dipatmax = curdip; }
	    curdip += ddip;
	}
	
	float cohres = maxcoh;
	float inldip = dipatmax;

	maxcoh = -1;
	
	curdip = -maxdip;

	while ( curdip <= maxdip )
	{
	    float coh = calc1( cursamp, cursamp + (curdip * distcrl)/refstep,
				samplegate, *inputdata[0], *inputdata[2] );

	    if ( coh > maxcoh ) { maxcoh = coh; dipatmax = curdip; }
	    curdip += ddip;
	}

	cohres += maxcoh;
	cohres /= 2;

	if ( outputinterest[0] ) 
	    output.item(0)->setValue( idx, cohres );
	if ( outputinterest[1] )
	    output.item(1)->setValue( idx, inldip * dipFactor() );
	if ( outputinterest[2] ) 
	    output.item(2)->setValue( idx, dipatmax * dipFactor() );
    }

    return true;
}


bool Coherency::computeData2( const DataHolder& output,
			    int t0, int nrsamples ) const
{
    Interval<int> samplegate( mNINT(gate.start/refstep),
				mNINT(gate.stop/refstep) );
    for ( int idx=0; idx<nrsamples; idx++ )
    {
	float cursample = t0 + idx;
	float maxcoh = -1;
	float inldipatmax;
	float crldipatmax;

	float inldip = -maxdip;

	while ( inldip <= maxdip )
	{
	    float crldip = -maxdip;

	    while ( crldip <= maxdip )
	    {
		float coh = calc2( cursample, samplegate, inldip, 
				    crldip, *redh, *imdh );

		if ( coh > maxcoh )
		    { maxcoh = coh; inldipatmax = inldip; crldipatmax = crldip;}

		crldip += ddip;
	    }

	    inldip += ddip;
	}
	
	if ( outputinterest[0] ) 
	    output.item(0)->setValue( idx, maxcoh );
	if ( outputinterest[1] )
	    output.item(1)->setValue( idx, inldipatmax * dipFactor() );
	if ( outputinterest[2] ) 
	    output.item(2)->setValue( idx, crldipatmax * dipFactor() );
    }

    return true;
}


bool Coherency::getInputOutput( int input, TypeSet<int>& res ) const
{
    return Provider::getInputOutput( input, res );
}
	

bool Coherency::getInputData( const BinID& relpos, int idx )
{
    bool yn;
    const BinID bidstep = inputs[0]-> getStepoutStep(yn);
    if ( type==1 )
    {
	while ( inputdata.size() < 3 )
	    inputdata += 0;

	const DataHolder* datac = inputs[0]->getData( relpos, idx );
	const DataHolder* datai = 
	   inputs[0]->getData( BinID(relpos.inl+bidstep.inl, relpos.crl), idx );
	const DataHolder* datax =
	   inputs[0]->getData( BinID(relpos.inl, relpos.crl+bidstep.crl), idx );
	if ( !datac || !datai || !datax )
	    return false;
	
	inputdata.replace( 0, datac );
	inputdata.replace( 1, datai );
	inputdata.replace( 2, datax );
    }
    else
    {
	if ( !redh )
	    redh = new Array2DImpl<DataHolder*>( stepout.inl * 2 + 1,
						stepout.crl * 2 + 1 );
	if ( !imdh )
	    imdh = new Array2DImpl<DataHolder*>( stepout.inl * 2 + 1,
	                                        stepout.crl * 2 + 1 );

	for ( int idy=-stepout.inl; idy<=stepout.inl; idy++ )
	{ 
	    for ( int idz=-stepout.crl; idz<=stepout.crl; idz++ )
	    {
		BinID bid = BinID( relpos.inl + idy * bidstep.inl, 
				   relpos.crl + idz * bidstep.crl );
		const DataHolder* dh = inputs[0]->getData( bid, idx );
		if ( !dh )
		    return false;

		redh->set(idy+stepout.inl,idz+stepout.crl, 
			const_cast<DataHolder*>(dh) );

		const DataHolder* data = inputs[1]->getData( bid, idx );
		if ( !data )
		    return false;
		
		imdh->set(idy+stepout.inl,idz+stepout.crl, 
			const_cast<DataHolder*>(data) );
	    }
	}
    } 
    return true;
}


const BinID* Coherency::reqStepout( int inp, int out ) const
{ return &stepout; }


const Interval<float>* Coherency::reqZMargin( int inp, int ) const
{ return &gate; }
}//namespace
