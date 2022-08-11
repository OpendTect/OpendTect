/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	A.H. Bril
 Date:		Jul 2005
________________________________________________________________________

-*/

#include "probdenfunctr.h"

#include "ascstream.h"
#include "ioobj.h"
#include "iopar.h"
#include "keystrs.h"
#include "sampledprobdenfunc.h"
#include "gaussianprobdenfunc.h"
#include "uistrings.h"

defineTranslatorGroup(ProbDenFunc, "Probability Density Function");
defineTranslator(od,ProbDenFunc,mdTectKey);

mDefSimpleTranslatorSelector(ProbDenFunc)
mDefSimpleTranslatorioContext(ProbDenFunc,Feat)

ProbDenFuncTranslator::ProbDenFuncTranslator( const char* nm, const char* unm )
    : Translator(nm,unm)
    , binary_(false)
{}


uiString ProbDenFuncTranslatorGroup::sTypeName( int num )
{ return uiStrings::sProbDensFunc( false, num ); }


ProbDenFunc* ProbDenFuncTranslator::read( const IOObj& ioobj,
					  uiString* emsg )
{
    mDynamicCast(ProbDenFuncTranslator*,
		 PtrMan<ProbDenFuncTranslator> pdftr, ioobj.createTranslator());
    if ( !pdftr )
    {
	if (emsg) *emsg = uiStrings::phrCannotCreate(tr("Translator"));
	return 0;
    }

    const BufferString fnm( ioobj.fullUserExpr(true) );
    od_istream strm( fnm );
    if ( !strm.isOK() )
    {
	if ( emsg )
	{
	    *emsg = uiStrings::phrCannotOpen(toUiString(fnm));
	    strm.addErrMsgTo(*emsg); 
	}
	return 0;
    }

    ProbDenFunc* ret = pdftr->read( strm );
    if ( !ret ) return 0;
    ret->setName( ioobj.name() );
    if ( !ret && emsg )
    { *emsg = tr("Cannot read PDF from '%1'").arg(fnm); }
    return ret;
}


ProbDenFunc* ProbDenFuncTranslator::readInfo( const IOObj& ioobj,
					      uiString* emsg )
{
    mDynamicCast(ProbDenFuncTranslator*,
	PtrMan<ProbDenFuncTranslator> pdftr, ioobj.createTranslator());
    if ( !pdftr )
    {
	if ( emsg )
	    *emsg = uiStrings::phrCannotCreate(tr("Translator"));

	return nullptr;
    }

    const BufferString fnm( ioobj.fullUserExpr(true) );
    od_istream strm( fnm );
    if ( !strm.isOK() )
    {
	if ( emsg )
	{
	    *emsg = uiStrings::phrCannotOpen(toUiString(fnm));
	    strm.addErrMsgTo(*emsg);
	}

	return nullptr;
    }

    ProbDenFunc* ret = pdftr->readInfo( strm );
    if ( !ret )
	return nullptr;

    ret->setName( ioobj.name() );
    if ( !ret && emsg )
	*emsg = tr("Cannot read PDF from '%1'").arg(fnm);

    return ret;
}


bool ProbDenFuncTranslator::write( const ProbDenFunc& pdf, const IOObj& ioobj,
				   uiString* emsg )
{
    mDynamicCast(ProbDenFuncTranslator*,
		 PtrMan<ProbDenFuncTranslator> pdftr, ioobj.createTranslator());
    if ( !pdftr )
    {
	if (emsg)
	    *emsg = uiStrings::phrCannotCreate(tr("Translator"));
	return false;
    }

    const BufferString fnm( ioobj.fullUserExpr(false) );
    od_ostream strm( fnm );
    if ( !strm.isOK() )
    {
	if ( emsg )
	{
	    *emsg = uiStrings::phrCannotOpen( toUiString(fnm) );
	    strm.addErrMsgTo(*emsg); 
	}
	return false;
    }

    const bool ret = pdftr->write( pdf, strm );
    if ( !ret && emsg )
    { *emsg = uiStrings::phrCannotWrite(toUiString(fnm)); }
    return ret;
}


ProbDenFunc* odProbDenFuncTranslator::read( od_istream& strm )
{
    ProbDenFunc* pdf = readInfo( strm );
    if ( !pdf )
	return nullptr;

    if ( !pdf->readBulk(strm,binary_) )
	deleteAndZeroPtr( pdf );

    return pdf;
}


ProbDenFunc* odProbDenFuncTranslator::readInfo( od_istream& strm )
{
    ascistream astrm( strm );
    IOPar par( astrm );
    StringView type = par.find( sKey::Type() );
    if ( type.isEmpty() )
	return nullptr;

    ProbDenFunc* pdf = nullptr;
    if ( type == Sampled1DProbDenFunc::typeStr() )
	pdf = new Sampled1DProbDenFunc();
    else if ( type == Sampled2DProbDenFunc::typeStr() )
	pdf = new Sampled2DProbDenFunc();
    else if ( type == SampledNDProbDenFunc::typeStr() )
	pdf = new SampledNDProbDenFunc();
    else if ( type == Gaussian1DProbDenFunc::typeStr() )
	pdf = new Gaussian1DProbDenFunc();
    else if ( type == Gaussian2DProbDenFunc::typeStr() )
	pdf = new Gaussian2DProbDenFunc();
    else if ( type == GaussianNDProbDenFunc::typeStr() )
	pdf = new GaussianNDProbDenFunc();

    if ( !pdf )
	return nullptr;

    if ( !pdf->usePar(par) )
	deleteAndZeroPtr( pdf );

    binary_ = false;
    par.getYN( sKey::Binary(), binary_ );
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

    pdf.writeBulk( strm, binary_ );
    return strm.isOK();
}
