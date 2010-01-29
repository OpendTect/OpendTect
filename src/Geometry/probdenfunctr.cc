/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	A.H. Bril
 Date:		Jul 2005
________________________________________________________________________

-*/
static const char* rcsID = "$Id: probdenfunctr.cc,v 1.3 2010-01-29 11:46:34 cvsnanne Exp $";

#include "probdenfunctr.h"

#include "ascstream.h"
#include "ioobj.h"
#include "iopar.h"
#include "ptrman.h"
#include "sampledprobdenfunc.h"
#include "streamconn.h"

defineTranslatorGroup(ProbDenFunc,"Probability Density Function");
defineTranslator(dgb,ProbDenFunc,mDGBKey);

mDefSimpleTranslatorSelector(ProbDenFunc,ProbDenFuncTranslator::key())
mDefSimpleTranslatorioContext(ProbDenFunc,Feat)

const char* ProbDenFuncTranslator::sKeyNrDim()	  { return "Nr dimensions"; }
const char* ProbDenFuncTranslator::sKeyDimName()  { return "Name"; }
const char* ProbDenFuncTranslator::sKeySize()     { return "Size"; }
const char* ProbDenFuncTranslator::sKeySampling() { return "Sampling"; }

bool dgbProbDenFuncTranslator::read( SampledProbDenFuncND& pdf,
				     const IOObj& ioobj )
{
    return true;
}


void dgbProbDenFuncTranslator::fillPar(
	const SampledProbDenFuncND& pdf, IOPar& par )
{
    const int nrdim = pdf.nrDims();
    par.set( sKeyNrDim(), nrdim );
    for ( int idx=0; idx<nrdim; idx++ )
    {
	par.set( IOPar::compKey(sKeyDimName(),idx), pdf.dimName(idx) );
	par.set( IOPar::compKey(sKeySize(),idx), pdf.size(idx) );
	par.set( IOPar::compKey(sKeySampling(),idx), pdf.sampling(idx) );
    }
}


bool dgbProbDenFuncTranslator::write( const SampledProbDenFuncND& pdf,
				      const IOObj& ioobj )
{
    PtrMan<StreamConn> conn =
	dynamic_cast<StreamConn*>(ioobj.getConn(Conn::Write));
    if ( !conn )
	return false;

    ascostream astrm( conn->oStream() );
    astrm.putHeader( mTranslGroupName(ProbDenFunc) );
    std::ostream& strm = astrm.stream();
    if ( !strm.good() )
	return false;

    IOPar par;
    fillPar( pdf, par );
    par.putTo( astrm );

    const ArrayND<float>& array = pdf.getData();
    const od_int64 totalsz = array.info().getTotalSz();
    const float* values = array.getData();
    if ( values )
    {
	for ( od_int64 idx=0; idx<totalsz; idx++ )
	    strm << values[idx] << '\n';
	return true;
    }

    const ValueSeries<float>* stor = array.getStorage();
    if ( stor )
    {
	for ( od_int64 idx=0; idx<totalsz; idx++ )
	    strm << stor->value(idx) << '\n';
	return true;
    }

    return true;
}
