/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
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
	deleteAndNullPtr( pdf );

    return pdf;
}


ProbDenFunc* odProbDenFuncTranslator::readInfo( od_istream& strm )
{
    ascistream astrm( strm );
    IOPar par( astrm );
    const BufferString type = par.find( sKey::Type() );
    if ( type.isEmpty() )
	return nullptr;

    ProbDenFunc* pdf = nullptr;
    if ( type.isEqual(Sampled1DProbDenFunc::typeStr()) )
	pdf = new Sampled1DProbDenFunc();
    else if ( type.isEqual(Sampled2DProbDenFunc::typeStr()) )
	pdf = new Sampled2DProbDenFunc();
    else if ( type.isEqual(SampledNDProbDenFunc::typeStr()) )
    {
	int nrdim = 0;
	par.get( ProbDenFunc::sKeyNrDim(), nrdim );
	pdf = new SampledNDProbDenFunc(nrdim>0 ? nrdim : 3);
    }
    else if ( type.isEqual(Gaussian1DProbDenFunc::typeStr()) )
	pdf = new Gaussian1DProbDenFunc();
    else if ( type.isEqual(Gaussian2DProbDenFunc::typeStr()) )
	pdf = new Gaussian2DProbDenFunc();
    else if ( type.isEqual(GaussianNDProbDenFunc::typeStr()) )
	pdf = new GaussianNDProbDenFunc();

    if ( !pdf )
	return nullptr;

    if ( !pdf->usePar(par) )
	deleteAndNullPtr( pdf );

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
