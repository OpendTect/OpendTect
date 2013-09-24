/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	A.H. Bril
 Date:		Jul 2005
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "probdenfunctr.h"

#include "ascstream.h"
#include "ioobj.h"
#include "iopar.h"
#include "keystrs.h"
#include "sampledprobdenfunc.h"
#include "strmprov.h"
#include <iostream>

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


ProbDenFunc* ProbDenFuncTranslator::read( const IOObj& ioobj,
					  BufferString* emsg )
{
    mDynamicCast(ProbDenFuncTranslator*,
	    	 PtrMan<ProbDenFuncTranslator> pdftr, ioobj.createTranslator());
    if ( !pdftr )
	{ if ( emsg ) *emsg = "Cannot create Translator"; return 0; }

    const BufferString fnm( ioobj.fullUserExpr(true) );
    od_istream strm( fnm );
    if ( !strm.isOK() )
    {
	if ( emsg )
	    { emsg->set( "Cannot open '" ).add( fnm ).add( "'" );
		strm.addErrMsgTo(*emsg); }
	return 0;
    }

    ProbDenFunc* ret = pdftr->read( strm );
    ret->setName( ioobj.name() );
    if ( !ret && emsg )
	{ *emsg = "Cannot read PDF from '"; *emsg += fnm; *emsg += "'"; }
    return ret;
}


bool ProbDenFuncTranslator::write( const ProbDenFunc& pdf, const IOObj& ioobj,
				   BufferString* emsg )
{
    mDynamicCast(ProbDenFuncTranslator*,
		 PtrMan<ProbDenFuncTranslator> pdftr, ioobj.createTranslator());
    if ( !pdftr )
	{ if ( emsg ) *emsg = "Cannot create Translator"; return false; }

    const BufferString fnm( ioobj.fullUserExpr(false) );
    od_ostream strm( fnm );
    if ( !strm.isOK() )
    {
	if ( emsg )
	    { emsg->set( "Cannot write to '" ).add( fnm ).add( "'" );
		strm.addErrMsgTo(*emsg); }
	return false;
    }

    const bool ret = pdftr->write( pdf, strm );
    if ( !ret && emsg )
	{ *emsg = "Cannot write PDF to '"; *emsg += fnm; *emsg += "'"; }
    return ret;
}


ProbDenFunc* odProbDenFuncTranslator::read( od_istream& strm )
{
    ascistream astrm( strm );
    IOPar par( astrm );
    FixedString type = par.find( sKey::Type() );
    if ( type.isEmpty() )
	return 0;

    ProbDenFunc* pdf = 0;
    if ( type == Sampled1DProbDenFunc::typeStr() )
	pdf = new Sampled1DProbDenFunc();
    else if ( type == Sampled2DProbDenFunc::typeStr() )
	pdf = new Sampled2DProbDenFunc();
    else if ( type == SampledNDProbDenFunc::typeStr() )
	pdf = new SampledNDProbDenFunc();

    if ( !pdf->usePar(par) )
	{ delete pdf; return 0; }

    binary_ = false;
    par.getYN( sKey::Binary(), binary_ );
    if ( !pdf->obtain(strm,binary_) )
	{ delete pdf; pdf = 0; }

    return pdf;
}


bool odProbDenFuncTranslator::write( const ProbDenFunc& pdf, od_ostream& strm )
{
    if ( !strm.isOK() )
	return false;

    ascostream astrm( strm );
    astrm.putHeader( mTranslGroupName(ProbDenFunc) );

    IOPar par;
    pdf.fillPar( par );
    par.setYN( sKey::Binary(), binary_ );
    par.putTo( astrm );

    pdf.dump( strm, binary_ );
    return strm.isOK();
}
