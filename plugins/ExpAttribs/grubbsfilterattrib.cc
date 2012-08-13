/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Satyaki Maitra
 Date:          March 2011
________________________________________________________________________

-*/
static const char* rcsID mUnusedVar = "$Id: grubbsfilterattrib.cc,v 1.12 2012-08-13 09:36:56 cvsaneesh Exp $";

#include "grubbsfilterattrib.h"

#include "arrayndimpl.h"
#include "array1dinterpol.h"
#include "attribdataholder.h"
#include "attribdesc.h"
#include "attribdescset.h"
#include "attribfactory.h"
#include "attribparam.h"
#include "attribsteering.h"
#include <math.h>

namespace Attrib
{

mAttrDefCreateInstance(GrubbsFilter)    
    
void GrubbsFilter::initClass()
{
    mAttrStartInitClassWithUpdate

    FloatParam* grubbsval = new FloatParam( grubbsvalStr() );
    grubbsval->setLimits( Interval<float>(0,10) );
    grubbsval->setDefaultValue( 1);
    desc->addParam( grubbsval );

    ZGateParam* gate = new ZGateParam( gateStr() );
    gate->setLimits( Interval<float>(-1000,1000) );
    gate->setDefaultValue( Interval<float>(-28, 28) );
    desc->addParam( gate );

    BinIDParam* stepout = new BinIDParam( stepoutStr() );
    stepout->setDefaultValue( BinID(1,1) );
    desc->addParam( stepout );

    EnumParam* type = new EnumParam( replaceValStr() );
    type->addEnum( "Average" );
    type->addEnum( "Median" );
    type->addEnum( "Threshold" );
    type->addEnum( "GrubbsVal" );
    type->addEnum( "Interpolate" );
    type->setDefaultValue( GrubbsFilter::Threshold );
    desc->addParam( type );

    desc->addInput( InputSpec("Input data",true) );
    desc->addOutputDataType( Seis::UnknowData );

    mAttrEndInitClass
}


void GrubbsFilter::updateDesc( Desc& desc )
{
    desc.setParamEnabled( stepoutStr(), true );
}


GrubbsFilter::GrubbsFilter( Desc& desc )
    : Provider( desc )
{
    if ( !isOK() ) return;

    inputdata_.allowNull(true);

    mGetFloat( cogrubbsval_, grubbsvalStr() );

    mGetFloatInterval( gate_, gateStr() );
    gate_.scale( 1.f/zFactor() );

    mGetBinID( stepout_, stepoutStr() )
    getTrcPos();

    mGetEnum( type_, replaceValStr() );
}


bool GrubbsFilter::getTrcPos()
{
    trcpos_.erase();
    BinID bid;
    int trcidx = 0;
    centertrcidx_ = 0;
    for ( bid.inl=-stepout_.inl; bid.inl<=stepout_.inl; bid.inl++ )
    {
	for ( bid.crl=-stepout_.crl; bid.crl<=stepout_.crl; bid.crl++ )
	{
	    if ( !bid.inl && !bid.crl )
		centertrcidx_ = trcidx;
	    trcpos_ += bid;
	    trcidx++;
	}
    }

    return true;
}


bool GrubbsFilter::getInputData( const BinID& relpos, int zintv )
{
    while ( inputdata_.size() < trcpos_.size() )
	inputdata_ += 0;

    const BinID bidstep = inputs_[0]->getStepoutStep();
    for ( int idx=0; idx<trcpos_.size(); idx++ )
    {
	const DataHolder* data = 
		    inputs_[0]->getData( relpos+trcpos_[idx]*bidstep, zintv );
	if ( !data ) return false;
	inputdata_.replace( idx, data );
    }
    
    dataidx_ = getDataIndex( 0 );

    return true;
}

static void checkTopBotUndefs( Array1D<float>& inpvals )
{
    int firstvalidsample=0, lastvalidsample=0;
    const int size = inpvals.info().getSize(0);
    for ( int idx=0; idx<size; idx++ )
    {
	if ( !mIsUdf(inpvals.get(idx))  )
	{
	    firstvalidsample = idx;
	    break;
	}
    }

    for ( int isamp=0; isamp<firstvalidsample; isamp++ )
	inpvals.set( isamp, inpvals.get(firstvalidsample) );

    for ( int idx=size-1; idx>0; idx-- )
    {
	if ( !mIsUdf(inpvals.get(idx)) )
	{
	    lastvalidsample = idx;
	    break;
	}
    }

    for ( int isamp=size-1; isamp>lastvalidsample; isamp-- )
	inpvals.set( isamp, inpvals.get(lastvalidsample) );
}


bool GrubbsFilter::computeData( const DataHolder& output, const BinID& relpos, 
				int z0, int nrsamples, int threadid ) const
{
    if ( inputdata_.isEmpty() ) return false;

    const Interval<int> samplegate( mNINT32(gate_.start/refstep_),
				    mNINT32(gate_.stop/refstep_) );

    const int nrtraces = inputdata_.size();

    Stats::CalcSetup setup( true );
    setup.require( Stats::Average );
    setup.require( Stats::StdDev );
    setup.require( Stats::Median );
    Stats::WindowedCalc<float> rc( setup, (samplegate.width()+1)*nrtraces );

    for ( int idx=0; idx<nrsamples; idx++ )
    {
    
	Array1DImpl<float> vals( samplegate.width() );

	for ( int trcidx=0; trcidx<nrtraces; trcidx++ )
	{
	    const DataHolder* data = inputdata_[trcidx];
	    for ( int zidx=samplegate.start; zidx<=samplegate.stop ; zidx++ )
	    {
		float sampleidx = idx + zidx;

		float traceval = mUdf(float);
		if ( data )
		{
		    traceval = getInputValue(*data,dataidx_,(int)sampleidx,z0);
		    if ( trcidx == centertrcidx_ )
		    {
			const int arridx = samplegate.getIndex( zidx, 1 );
			if ( vals.info().validPos(arridx) )
			    vals.set( arridx, traceval );
		    }
		}

		if ( zidx == samplegate.stop || idx == 0 ) 
		    rc += traceval;
	    }
	}

	LinearArray1DInterpol interpolator;
	interpolator.setArray( vals );

	const DataHolder* data = inputdata_[nrtraces/2];
	const float traceval = getInputValue(*data,dataidx_,(int)idx,z0);
	float grubbsval = (float) ( (traceval - rc.average())/rc.stdDev() );
	const bool positive = grubbsval > 0;
	grubbsval = fabs( grubbsval );
	float newval = traceval;
	if ( type_ == GrubbsFilter::GrubbsValue )
	    newval = grubbsval;
	else if ( grubbsval > cogrubbsval_ ) 
	{
	    switch ( type_ ) 
	    { 
		case GrubbsFilter::Average:	newval = (float) rc.average(); break;
		case GrubbsFilter::Median:	newval = rc.median(); break;
		case GrubbsFilter::Threshold: 
		    newval = (float) ((cogrubbsval_ * rc.stdDev())+rc.average()); 
		    newval = positive ? newval * 1 : newval * -1;
		    break;
		case GrubbsFilter::Interpolate:
		    for ( int arridx=0; arridx<vals.info().getSize(0); arridx++)
		    {
			float arrval = vals.get( arridx );
			grubbsval = (float) fabs((arrval - rc.average())/rc.stdDev());
			if ( grubbsval > cogrubbsval_ )
			    vals.set( arridx, mUdf(float) );
		    }

		    checkTopBotUndefs( vals );

		    interpolator.execute();
		    newval = vals.get( samplegate.getIndex(0,1) );
		    break;
	    }
	}

	setOutputValue( output, 0, idx, z0, newval );
    }

    return true;
}



const BinID* GrubbsFilter::desStepout( int inp, int out ) const
{ return inp ? 0 : &stepout_; }


#define mAdjustGate( cond, gatebound, plus )\
{\
    if ( cond )\
    {\
	int minbound = (int)(gatebound / refstep_);\
	int incvar = plus ? 1 : -1;\
	gatebound = (minbound+incvar) * refstep_;\
    }\
}

void GrubbsFilter::prepPriorToBoundsCalc()
{
     const int truestep = mNINT32( refstep_*zFactor() );
     if ( truestep == 0 )
       	 return Provider::prepPriorToBoundsCalc();

    bool chgstartr = mNINT32(gate_.start*zFactor()) % truestep ; 
    bool chgstopr = mNINT32(gate_.stop*zFactor()) % truestep;

    mAdjustGate( chgstartr, gate_.start, false )
    mAdjustGate( chgstopr, gate_.stop, true )

    Provider::prepPriorToBoundsCalc();
}


const Interval<float>* GrubbsFilter::desZMargin( int inp, int ) const
{
    return inp ? 0 : &gate_;
}


}; //namespace
