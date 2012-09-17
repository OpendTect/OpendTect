/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        N. Hemstra
 Date:          May 2005
 RCS:           $Id: hilbertattrib.cc,v 1.29 2011/01/06 15:25:01 cvsbert Exp $
________________________________________________________________________

-*/

#include "hilbertattrib.h"

#include "arrayndimpl.h"
#include "attribdataholder.h"
#include "attribdesc.h"
#include "attribfactory.h"
#include "attribparam.h"
#include "hilberttransform.h"

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

    desc->setLocality( Desc::SingleTrace );
    mAttrEndInitClass
}


Hilbert::Hilbert( Desc& ds )
    : Provider( ds )
{
    if ( !isOK() ) return;

    mGetInt( halflen_, halflenStr() );
    zmargin_ = Interval<int>( -halflen_, halflen_ );
}


bool Hilbert::getInputOutput( int input, TypeSet<int>& res ) const
{
    return Provider::getInputOutput( input, res );
}


bool Hilbert::getInputData( const BinID& relpos, int intv )
{
    inputdata_ = inputs_[0]->getData( relpos, intv );
    dataidx_ = getDataIndex( 0 );
    return inputdata_;
}


bool Hilbert::computeData( const DataHolder& output, const BinID& relpos,
			   int z0, int nrsamples, int threadid ) const
{
    if ( !inputdata_ ) return false;

    const int hilbfilterlen = halflen_*2 + 1;
    const bool enoughsamps = nrsamples >= hilbfilterlen;
    const int arrminnrsamp = inputdata_->nrsamples_>hilbfilterlen
                                ? inputdata_->nrsamples_ : hilbfilterlen;
    const int nrsamptooutput = enoughsamps ? nrsamples : arrminnrsamp;
    const int shift = z0 - inputdata_->z0_;

    int inpstartidx = 0;
    int startidx = enoughsamps ? shift : 0;

    Array1DImpl<float> createarr( arrminnrsamp );
    ValueSeries<float>* padtrace = 0;
    if ( !enoughsamps )
    {
	for ( int idx=0; idx<arrminnrsamp; idx++ )
	{
	    const float val = idx < inputdata_->nrsamples_ ?
		inputdata_->series(dataidx_)->value(idx) : 0;
	    createarr.set( idx, val );
	}

	padtrace = createarr.getStorage();
	if ( !padtrace )
	    return false;

	startidx = shift;
    }

    HilbertTransform ht;
    if ( !ht.init() )
	return false;

    ht.setHalfLen( halflen_ );
    ht.setCalcRange( startidx, arrminnrsamp, inpstartidx );
    Array1DImpl<float> outarr( arrminnrsamp );
    const bool transformok = enoughsamps
	      ? ht.transform(*inputdata_->series(dataidx_),
		      	     inputdata_->nrsamples_,outarr,nrsamptooutput )
	      : ht.transform(*padtrace,arrminnrsamp,outarr,nrsamptooutput );
    if ( !transformok )
	return false;

    for ( int idx=0; idx<nrsamples; idx++ )
	setOutputValue( output, 0, idx, z0, outarr.get(shift+idx) );

    return true;
}

}; // namespace Attrib
