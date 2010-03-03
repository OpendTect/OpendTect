/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	A.H. Bril
 Date:		Jul 2005
________________________________________________________________________

-*/
static const char* rcsID = "$Id: probdenfunctr.cc,v 1.9 2010-03-03 02:34:09 cvsnanne Exp $";

#include "probdenfunctr.h"

#include "ascstream.h"
#include "ioobj.h"
#include "iopar.h"
#include "keystrs.h"
#include "sampledprobdenfunc.h"
#include "strmprov.h"

defineTranslatorGroup(ProbDenFunc,ProbDenFuncTranslator::key());
defineTranslator(od,ProbDenFunc,mdTectKey);

mDefSimpleTranslatorSelector(ProbDenFunc,ProbDenFuncTranslator::key())
mDefSimpleTranslatorioContext(ProbDenFunc,Feat)

ProbDenFuncTranslator::ProbDenFuncTranslator( const char* nm, const char* unm )
    : Translator(nm,unm)
    , binary_(false)
{}


const char* ProbDenFuncTranslator::key()
{ return "Probability Density Function"; }

static const char* sKeyBinary = "Binary";


ProbDenFunc* ProbDenFuncTranslator::read( const IOObj& ioobj,
					  BufferString* emsg )
{
    Translator* trl = ioobj.getTranslator();
    mDynamicCastGet(ProbDenFuncTranslator*,pdftr,trl)
    if ( !pdftr )
	{ if ( emsg ) *emsg = "Cannot create Translator"; return 0; }

    const BufferString fnm( ioobj.fullUserExpr(true) );
    StreamData sd( StreamProvider(fnm).makeIStream() );
    if ( !sd.usable() )
    {
	if ( emsg )
	    { *emsg = "Cannot open '"; *emsg += fnm; *emsg += "'"; }
	return 0;
    }

    ProbDenFunc* ret = pdftr->read( *sd.istrm );
    ret->setName( ioobj.name() );
    sd.close();
    if ( !ret && emsg )
	{ *emsg = "Cannot read PDF from '"; *emsg += fnm; *emsg += "'"; }
    return ret;
}


bool ProbDenFuncTranslator::write( const ProbDenFunc& pdf, const IOObj& ioobj,
				   BufferString* emsg )
{
    Translator* trl = ioobj.getTranslator();
    mDynamicCastGet(ProbDenFuncTranslator*,pdftr,trl)
    if ( !pdftr )
	{ if ( emsg ) *emsg = "Cannot create Translator"; return false; }

    const BufferString fnm( ioobj.fullUserExpr(false) );
    StreamData sd( StreamProvider(fnm).makeOStream() );
    if ( !sd.usable() )
    {
	if ( emsg )
	    { *emsg = "Cannot write to '"; *emsg += fnm; *emsg += "'"; }
	return false;
    }

    const bool ret = pdftr->write( pdf, *sd.ostrm );
    sd.close();
    if ( !ret && emsg )
	{ *emsg = "Cannot write PDF to '"; *emsg += fnm; *emsg += "'"; }
    return ret;
}


ProbDenFunc* odProbDenFuncTranslator::read( std::istream& strm )
{
    ascistream astrm( strm );
    IOPar par( astrm );
    FixedString type = par.find( sKey::Type );
    if ( type.isEmpty() )
	return 0;

    ProbDenFunc* pdf;
    if ( type == SampledProbDenFunc1D::typeStr() )
	pdf = new SampledProbDenFunc1D( Array1DImpl<float>(-1) );
    else if ( type == SampledProbDenFunc2D::typeStr() )
	pdf = new SampledProbDenFunc2D( Array2DImpl<float>(-1,-1) );
    else if ( type == SampledProbDenFuncND::typeStr() )
	pdf = new SampledProbDenFuncND();

    pdf->usePar( par );
    binary_ = false;
    par.getYN( sKeyBinary, binary_ );

    pdf->obtain( strm, binary_ );
    return pdf;
}


bool odProbDenFuncTranslator::write( const ProbDenFunc& pdf,
				     std::ostream& strm )
{
    if ( !strm.good() )
	return false;

    ascostream astrm( strm );
    astrm.putHeader( mTranslGroupName(ProbDenFunc) );

    IOPar par;
    pdf.fillPar( par );
    par.setYN( sKeyBinary, binary_ );
    par.putTo( astrm );

    pdf.dump( strm, binary_ );
    return strm.good();
}
