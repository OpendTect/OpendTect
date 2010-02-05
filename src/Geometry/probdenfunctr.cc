/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	A.H. Bril
 Date:		Jul 2005
________________________________________________________________________

-*/
static const char* rcsID = "$Id: probdenfunctr.cc,v 1.4 2010-02-05 12:08:49 cvsnanne Exp $";

#include "probdenfunctr.h"

#include "ascstream.h"
#include "ioobj.h"
#include "iopar.h"
#include "keystrs.h"
#include "ptrman.h"
#include "sampledprobdenfunc.h"
#include "streamconn.h"

defineTranslatorGroup(ProbDenFunc,ProbDenFuncTranslator::key());
defineTranslator(dgb,ProbDenFunc,mDGBKey);

mDefSimpleTranslatorSelector(ProbDenFunc,ProbDenFuncTranslator::key())
mDefSimpleTranslatorioContext(ProbDenFunc,Feat)

const char* ProbDenFuncTranslator::key()
{ return "Probability Density Function"; }

ProbDenFunc* dgbProbDenFuncTranslator::read( const IOObj& ioobj )
{
    PtrMan<StreamConn> conn =
	dynamic_cast<StreamConn*>(ioobj.getConn(Conn::Read));
    if ( !conn )
	return false;

    ascistream astrm( conn->iStream() );
    std::istream& strm = astrm.stream();
    IOPar par( astrm );
    FixedString type = par.find( sKey::Type );
    if ( type.isEmpty() )
	return 0;

    ProbDenFunc* pdf;
    if ( type == SampledProbDenFunc1D::typeStr() )
	pdf = new SampledProbDenFunc1D( Array1DImpl<float>(-1) );
    else if ( type == SampledProbDenFunc2D::typeStr() )
	pdf = new SampledProbDenFunc2D( Array2DImpl<float>(-1,-1) );

    pdf->usePar( par );
    pdf->obtain( strm );
    return pdf;
}


bool dgbProbDenFuncTranslator::write( const ProbDenFunc& pdf,
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
    pdf.fillPar( par );
    par.putTo( astrm );
    pdf.dump( strm );
    return true;
}
