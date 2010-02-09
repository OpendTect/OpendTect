/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	A.H. Bril
 Date:		Jul 2005
________________________________________________________________________

-*/
static const char* rcsID = "$Id: probdenfunctr.cc,v 1.5 2010-02-09 07:48:25 cvsnanne Exp $";

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

ProbDenFuncTranslator::ProbDenFuncTranslator( const char* nm, const char* unm )
    : Translator(nm,unm)
    , binary_(false)
{}


const char* ProbDenFuncTranslator::key()
{ return "Probability Density Function"; }

static const char* sKeyBinary = "Binary";

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
    binary_ = false;
    par.getYN( sKeyBinary, binary_ );
    pdf->obtain( strm, binary_ );
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
    par.setYN( sKeyBinary, binary_ );
    par.putTo( astrm );
    pdf.dump( strm, binary_ );
    return true;
}
