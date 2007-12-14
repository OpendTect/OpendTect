/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : Oct 1999
-*/

static const char* rcsID = "$Id: coherencyattrib.cc,v 1.20 2007-12-14 23:00:44 cvskris Exp $";


#include "coherencyattrib.h"
#include "attribdataholder.h"
#include "attribdesc.h"
#include "attribfactory.h"
#include "attribparam.h"
#include "ptrman.h"
#include "simpnumer.h"
#include "math2.h"

namespace Attrib
{

mAttrDefCreateInstance(Coherency)
    
void Coherency::initClass()
{
    Desc* desc = new Desc( attribName(), updateDesc );
    desc->ref();

    IntParam* type = new IntParam( sKeyType() );
    type->setLimits( Interval<int>(1,2) );
    type->setDefaultValue( 2 );
    desc->addParam( type );

    ZGateParam* gate = new ZGateParam( sKeyGate() );
    gate->setLimits( Interval<float>(-mUdf(float),mUdf(float)) );
    gate->setDefaultValue( Interval<float>(-28,28) );
    desc->addParam( gate );

    FloatParam* maxdip = new FloatParam( sKeyMaxDip() );
    maxdip->setLimits( Interval<float>(0,mUdf(float)) );
    maxdip->setDefaultValue( 250 );
    desc->addParam( maxdip );

    FloatParam* ddip = new FloatParam( sKeyDDip() );
    ddip->setLimits( Interval<float>(0,mUdf(float)) );
    ddip->setDefaultValue( 10 );
    desc->addParam( ddip );

    BinIDParam* stepout = new BinIDParam( sKeyStepout() );
    stepout->setDefaultValue( BinID(1,1) );
    stepout->setRequired( false );
    desc->addParam( stepout );

    desc->addInput( InputSpec("Real data for Coherency",true) );
    desc->addInput( InputSpec("Imag data for Coherency",true) );
    desc->setNrOutputs( Seis::UnknowData, 3 );

    mAttrEndInitClass
}


void Coherency::updateDesc( Desc& desc )
{
    const ValParam* type = desc.getValParam( sKeyType() );
    if ( type->getIntValue() == 2 )
	desc.inputSpec(1).enabled = true;
}


Coherency::Coherency( Desc& desc_ )
    : Provider( desc_ )
    , realdataholder_ ( 0 )
    , imagdataholder_ ( 0 )
{ 
    if ( !isOK() ) return;

    inputdata_.allowNull(true);
    
    mGetInt( type_, sKeyType() );
    
    mGetFloatInterval( gate_, sKeyGate() );
    gate_.start = gate_.start / zFactor(); gate_.stop = gate_.stop / zFactor();

    mGetFloat( maxdip_, sKeyMaxDip() );
    maxdip_ = maxdip_/dipFactor();

    mGetFloat( ddip_, sKeyDDip() );
    ddip_ = ddip_/dipFactor();

    mGetBinID( stepout_, sKeyStepout() );
    stepout_.inl = abs( stepout_.inl ); stepout_.crl = abs( stepout_.crl );

    const float extraz = 
		    (stepout_.inl*inldist()+stepout_.crl*crldist()) * maxdip_;
    desgate_ = Interval<float>( gate_.start-extraz, gate_.stop+extraz );
}


Coherency::~Coherency()
{
    if ( realdataholder_ ) delete realdataholder_;
    if ( imagdataholder_ ) delete imagdataholder_;
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
	float val1 = interp1.value( *(dh1.series(realidx_)), s1-dh1.z0_ + s );
	float val2 = interp2.value( *(dh2.series(realidx_)), s2-dh2.z0_ + s );
	xsum += val1 * val2;
	sum1 += val1 * val1;
	sum2 += val2 * val2;
    }

    return xsum / Sqrt( sum1 * sum2 );
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
	    float inlpos = (idy - (inlsz/2)) * distinl_;
	    for ( int idz=0; idz<crlsz; idz++ )
	    {
		ValueSeriesInterpolator<float> 
		    	interp( re.get(idy,idz)->nrsamples_-1 );
		float crlpos = (idz - (crlsz/2)) * distcrl_;
		float place = s - re.get(idy,idz)->z0_ + idx + 
		    	     (inlpos*inldip)/refstep + (crlpos*crldip)/refstep;
		    
		float real = 
		    interp.value( *(re.get(idy,idz)->series(realidx_)), place );

		float imag =  
		   -interp.value( *(im.get(idy,idz)->series(imagidx_)), place );
		
		realsum += real;
		imagsum += imag;

		denominator += real * real + imag*imag;
	    }
	}

	numerator += realsum * realsum + imagsum * imagsum;
    }	

    return denominator? numerator / ( inlsz * crlsz * denominator ) : 0 ;	
}


void Coherency::prepPriorToBoundsCalc()
{
    bool chgstart = mNINT(desgate_.start*zFactor()) % mNINT(refstep*zFactor());
    bool chgstop = mNINT(desgate_.stop*zFactor()) % mNINT(refstep*zFactor());

    if ( chgstart )
    {
	int minstart = (int)(desgate_.start / refstep);
	desgate_.start = (minstart-1) * refstep;
    }
    if ( chgstop )
    {
	int minstop = (int)(desgate_.stop / refstep);
	desgate_.stop = (minstop+1) * refstep;
    }
}


void Coherency::prepareForComputeData()
{
    BinID step = inputs[0]->getStepoutStep();
	
    distinl_ = fabs(inldist()*step.inl);
    distcrl_ = fabs(crldist()*step.crl);
}


bool Coherency::computeData( const DataHolder& output, const BinID& relpos,
			     int z0, int nrsamples, int threadid ) const
{
    return type_ == 1 ? computeData1(output, z0, nrsamples) 
		     : computeData2(output, z0, nrsamples);
}


bool Coherency::computeData1( const DataHolder& output, int z0, 
			      int nrsamples ) const
{
    Interval<int> samplegate( mNINT(gate_.start/refstep),
				mNINT(gate_.stop/refstep) );
    for ( int idx=0; idx<nrsamples; idx++ )
    {
	float cursamp = z0 + idx;
	float maxcoh = -1;
	float dipatmax;

	float curdip = -maxdip_;

	while ( curdip <= maxdip_ )
	{
	    float coh = calc1( cursamp, cursamp + (curdip * distinl_)/refstep,
				samplegate, *inputdata_[0], *inputdata_[1] );

	    if ( coh > maxcoh ) { maxcoh = coh; dipatmax = curdip; }
	    curdip += ddip_;
	}
	
	float cohres = maxcoh;
	float inldip = dipatmax;

	maxcoh = -1;
	
	curdip = -maxdip_;

	while ( curdip <= maxdip_ )
	{
	    float coh = calc1( cursamp, cursamp + (curdip * distcrl_)/refstep,
				samplegate, *inputdata_[0], *inputdata_[2] );

	    if ( coh > maxcoh ) { maxcoh = coh; dipatmax = curdip; }
	    curdip += ddip_;
	}

	cohres += maxcoh;
	cohres /= 2;

	setOutputValue( output, 0, idx, z0, cohres );
	setOutputValue( output, 1, idx, z0, inldip * dipFactor() );
	setOutputValue( output, 2, idx, z0, dipatmax * dipFactor() );
    }

    return true;
}


bool Coherency::computeData2( const DataHolder& output, int z0, 
			      int nrsamples ) const
{
    Interval<int> samplegate( mNINT(gate_.start/refstep),
				mNINT(gate_.stop/refstep) );
    for ( int idx=0; idx<nrsamples; idx++ )
    {
	float cursample = z0 + idx;
	float maxcoh = -1;
	float inldipatmax;
	float crldipatmax;

	float inldip = -maxdip_;

	while ( inldip <= maxdip_ )
	{
	    float crldip = -maxdip_;

	    while ( crldip <= maxdip_ )
	    {
		float coh = calc2( cursample, samplegate, inldip, 
				crldip, *realdataholder_, *imagdataholder_ );

		if ( coh > maxcoh )
		    { maxcoh = coh; inldipatmax = inldip; crldipatmax = crldip;}

		crldip += ddip_;
	    }

	    inldip += ddip_;
	}
	
	setOutputValue( output, 0, idx, z0, maxcoh );
	setOutputValue( output, 1, idx, z0, inldipatmax * dipFactor() );
	setOutputValue( output, 2, idx, z0, crldipatmax * dipFactor() );
    }

    return true;
}


bool Coherency::getInputOutput( int input, TypeSet<int>& res ) const
{
    return Provider::getInputOutput( input, res );
}
	

bool Coherency::getInputData( const BinID& relpos, int idx )
{
    const BinID bidstep = inputs[0]->getStepoutStep();
    if ( type_==1 )
    {
	while ( inputdata_.size() < 3 )
	    inputdata_ += 0;

	const DataHolder* datac = inputs[0]->getData( relpos, idx );
	const DataHolder* datai = 
	   inputs[0]->getData( BinID(relpos.inl+bidstep.inl, relpos.crl), idx );
	const DataHolder* datax =
	   inputs[0]->getData( BinID(relpos.inl, relpos.crl+bidstep.crl), idx );
	if ( !datac || !datai || !datax )
	    return false;

	realidx_ = getDataIndex( 0 );
	inputdata_.replace( 0, datac );
	inputdata_.replace( 1, datai );
	inputdata_.replace( 2, datax );
    }
    else
    {
	if ( !realdataholder_ )
	    realdataholder_ =
		new Array2DImpl<DataHolder*>( stepout_.inl * 2 + 1,
					      stepout_.crl * 2 + 1 );
	if ( !imagdataholder_ )
	    imagdataholder_ =
		new Array2DImpl<DataHolder*>( stepout_.inl * 2 + 1,
	                                      stepout_.crl * 2 + 1 );

	for ( int idy=-stepout_.inl; idy<=stepout_.inl; idy++ )
	{ 
	    for ( int idz=-stepout_.crl; idz<=stepout_.crl; idz++ )
	    {
		BinID bid = BinID( relpos.inl + idy * bidstep.inl, 
				   relpos.crl + idz * bidstep.crl );
		const DataHolder* dh = inputs[0]->getData( bid, idx );
		if ( !dh )
		    return false;

		realdataholder_->set(idy+stepout_.inl,idz+stepout_.crl, 
			const_cast<DataHolder*>(dh) );

		const DataHolder* data = inputs[1]->getData( bid, idx );
		if ( !data )
		    return false;
		
		imagdataholder_->set(idy+stepout_.inl,idz+stepout_.crl, 
				     const_cast<DataHolder*>(data) );
	    }
	}
	realidx_ = getDataIndex( 0 );
	imagidx_ = getDataIndex( 1 );
    } 
    return true;
}


const BinID* Coherency::reqStepout( int inp, int out ) const
{ return &stepout_; }


const Interval<float>* Coherency::reqZMargin( int inp, int ) const
{
    return &desgate_;
}

} // namespace Attrib
