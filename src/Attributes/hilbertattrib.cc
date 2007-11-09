/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        N. Hemstra
 Date:          May 2005
 RCS:           $Id: hilbertattrib.cc,v 1.21 2007-11-09 16:53:52 cvshelene Exp $
________________________________________________________________________

-*/

#include "hilbertattrib.h"

#include "attribdataholder.h"
#include "attribdesc.h"
#include "attribfactory.h"
#include "attribparam.h"
#include "genericnumer.h"

namespace Attrib
{

mAttrDefCreateInstance(Hilbert)
    
void Hilbert::initClass()
{
    mAttrStartInitClass

    IntParam* halflen = new IntParam( halflenStr() );
    halflen->setDefaultValue( 30 );
    halflen->setValue( 30 );
    halflen->setRequired( false );
    desc->addParam( halflen );

    desc->addInput( InputSpec("Input data",true) );
    desc->addOutputDataType( Seis::UnknowData );

    mAttrEndInitClass
}


Hilbert::Hilbert( Desc& ds )
    : Provider( ds )
{
    if ( !isOK() ) return;

    mGetInt( halflen_, halflenStr() );
    hilbfilterlen_ = halflen_ * 2 + 1;
    hilbfilter_ = makeHilbFilt( halflen_ );
    zmargin_ = Interval<int>( -halflen_, halflen_ );
}


bool Hilbert::getInputOutput( int input, TypeSet<int>& res ) const
{
    return Provider::getInputOutput( input, res );
}


bool Hilbert::getInputData( const BinID& relpos, int intv )
{
    inputdata_ = inputs[0]->getData( relpos, intv );
    dataidx_ = getDataIndex( 0 );
    return inputdata_;
}


float* Hilbert::makeHilbFilt( int hlen )
{
    float* h = new float[hlen*2+1];
    h[hlen] = 0;
    for ( int i=1; i<=hlen; i++ )
    {
	const float taper = 0.54 + 0.46 * cos( M_PI*(float)i / (float)(hlen) );
	h[hlen+i] = taper * ( -(float)(i%2)*2.0 / (M_PI*(float)(i)) );
	h[hlen-i] = -h[hlen+i];
    }

    return h;
}


class Masker
{
public:
Masker( const DataHolder* dh, int shift, float avg, int dataidx )
    : data_(dh )
    , avg_(avg)
    , shift_(shift)
    , dataidx_(dataidx) {}

float operator[]( int idx ) const
{
    const int pos = shift_ + idx;
    float val = mUdf(float);
    if ( pos < 0 )
	val = data_->series(dataidx_)->value(0) - avg_;
    else if ( pos >= data_->nrsamples_ )
	val = data_->series(dataidx_)->value(data_->nrsamples_-1) - avg_;
    else
	val = data_->series(dataidx_)->value( pos );
    
    bool goup = pos<data_->nrsamples_/2;
    int tmppos = goup ? ( pos<0 ? 1 : pos+1) 
		      : (pos>=data_->nrsamples_ ? data_->nrsamples_-2 : pos-1);
    while ( mIsUdf( val ) && tmppos>0 && tmppos<data_->nrsamples_ )
    {
	val = data_->series(dataidx_)->value( tmppos );
	goup ? tmppos++ : tmppos--;
    }
    
    return mIsUdf(val) ? val : val - avg_;
}

    const DataHolder*	data_;
    const int		shift_;
    float		avg_;
    int			dataidx_;
};


bool Hilbert::computeData( const DataHolder& output, const BinID& relpos, 
			   int z0, int nrsamples, int threadid ) const
{
    if ( !inputdata_ ) return false;

    const int shift = z0 - inputdata_->z0_;
    Masker masker( inputdata_, shift, 0, dataidx_ );
    float avg = 0;
    int nrsamptooutput = nrsamples<hilbfilterlen_ ? hilbfilterlen_ : nrsamples;
    int nrsampleused = nrsamptooutput;
    for ( int idx=0; idx<nrsamptooutput; idx++ )
    {
	float val = masker[idx];
	if ( mIsUdf(val) )
	{
	    avg += 0;
	    nrsampleused--;
	}
	else
	    avg += val;
    }

    masker.avg_ = avg / nrsampleused;
    float* newarr = new float[hilbfilterlen_];
    float* outp = nrsamples>=hilbfilterlen_ ? output.series(0)->arr() : newarr;
    GenericConvolve( hilbfilterlen_, -halflen_, hilbfilter_,
		     inputdata_->nrsamples_, 0, masker,
		     nrsamptooutput, 0, outp );

    if ( nrsamples<hilbfilterlen_ )
    {
	int startshift = (hilbfilterlen_-nrsamples)/2;
	for ( int idx=0; idx<nrsamples; idx++ )
	    setOutputValue( output, 0, idx, z0, outp[startshift+idx] );
    }
       
    delete [] newarr;
    return true;
}


}; // namespace Attrib
