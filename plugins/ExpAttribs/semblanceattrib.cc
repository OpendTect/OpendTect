/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nanne Hemstra
 Date:		January 2008
________________________________________________________________________

-*/
static const char* rcsID = "$Id: semblanceattrib.cc,v 1.4 2009-07-22 16:01:26 cvsbert Exp $";


#include "semblanceattrib.h"

#include "attribdataholder.h"
#include "attribdesc.h"
#include "attribfactory.h"
#include "attribparam.h"


namespace Attrib
{

mAttrDefCreateInstance(Semblance)

void Semblance::initClass()
{
    mAttrStartInitClass
    IntParam* inlsz = new IntParam( inlszStr() );
    inlsz->setLimits( Interval<int>(1,mUdf(int)) );
    inlsz->setDefaultValue( 3 );
    desc->addParam( inlsz );

    IntParam* crlsz = new IntParam( crlszStr() );
    crlsz->setLimits( Interval<int>(1,mUdf(int)) );
    crlsz->setDefaultValue( 3 );
    desc->addParam( crlsz );

    IntParam* zsz = new IntParam( zszStr() );
    zsz->setLimits( Interval<int>(1,mUdf(int)) );
    zsz->setDefaultValue( 3 );
    desc->addParam( zsz );
    desc->addOutputDataType( Seis::UnknowData );
    desc->addInput( InputSpec("Input data",true) );
    mAttrEndInitClass
}


Semblance::Semblance( Desc& desc_ )
    : Provider( desc_ )
{
    mGetInt( inlsz_, inlszStr() );
    mGetInt( crlsz_, crlszStr() );
    mGetInt( zsz_, zszStr() );

    stepout_ = BinID( inlsz_/2, crlsz_/2 );
    zintv_ = Interval<int>(-zsz_/2 ,zsz_/2 );
}


const BinID* Semblance::desStepout( int inp, int out ) const
{
    return &stepout_;
}


const Interval<int>* Semblance::desZSampMargin(int,int) const
{
    return &zintv_;
}

		 
bool Semblance::getInputData( const BinID& relpos, int zintv )
{
    inpdata_.erase();
    BinID bid;
    for ( bid.inl=-inlsz_/2; bid.inl<=inlsz_/2; bid.inl++ )
    {
	for ( bid.crl=-crlsz_/2; bid.crl<=crlsz_/2; bid.crl++ )
	    inpdata_ += inputs[0]->getData( relpos+bid, zintv );
    }

     dataidx_ = getDataIndex( 0 );
     return true;
}

       
bool Semblance::computeData( const DataHolder& output, const BinID& relpos,
			     int z0, int nrsamples, int threadid ) const	
{
    for ( int idx=0; idx<nrsamples; idx++ )
    {
	float numerator = 0;
	float denominator = 0;
        for ( int zidx=-zsz_/2; zidx<=zsz_/2 ; zidx++) 
	{
	    float sum = 0;
	    for ( int trcidx=0; trcidx<inpdata_.size(); trcidx++ )
            {
		const DataHolder* data = inpdata_[trcidx];
		const int sampidx = idx + zidx;
		const float traceval = 
		   		getInputValue( *data, dataidx_, sampidx, z0 );
		if ( mIsUdf(traceval) ) continue;
		sum += traceval;
 		denominator += traceval*traceval;
            }

	    numerator += sum*sum;
	}

	float semblance = denominator ? numerator/(inpdata_.size()*denominator)
	    			      : mUdf(float);
	setOutputValue( output, 0, idx, z0, semblance );
    }

    return true;
}

} // namespace Attrib
