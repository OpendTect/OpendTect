/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert
 * DATE     : Feb 2010
-*/

static const char* rcsID = "$Id: seisbayesclass.cc,v 1.2 2010-02-12 14:50:03 cvsbert Exp $";

#include "seisbayesclass.h"
#include "seisread.h"
#include "seiswrite.h"
#include "seistrc.h"
#include "seistrctr.h"
#include "seisbuf.h"
#include "seisbounds.h"
#include "probdenfunc.h"
#include "probdenfunctr.h"
#include "keystrs.h"
#include "iopar.h"
#include "ioman.h"
#include "ioobj.h"

const char* SeisBayesClass::sKeyPDFID()		{ return "PDF.ID"; }
const char* SeisBayesClass::sKeySeisInpID()	{ return "Seismics.Input.ID"; }
const char* SeisBayesClass::sKeySeisOutID()	{ return "Seismics.Output.ID"; }


SeisBayesClass::SeisBayesClass( const IOPar& iop )
    	: Executor( "Bayesian inversion" )
	, needclass_(false)
	, nrdone_(0)
	, totalnr_(-2)
	, inptrcs_(*new SeisTrcBuf(true))
	, outtrcs_(*new SeisTrcBuf(true))
{
    const char* res = iop.find( sKey::Type );
    is2d_ = res && *res == '2';
    if ( is2d_ )
	{ msg_ = "2D not implemented"; return; }

    if ( !getPDFs(iop) || !getReaders(iop) || !getWriters(iop) )
	return;

    msg_ = "Processing";
}

#define mAddIdxRank(idx) msg_ += idx+1; msg_ += getRankPostFix(idx+1)


bool SeisBayesClass::getPDFs( const IOPar& iop )
{
    for ( int ipdf=0; ; ipdf++ )
    {
	const char* id = iop.find( mGetSeisBayesPDFIDKey(ipdf) );
	if ( !id || !*id )
	    break;

	PtrMan<IOObj> ioobj = IOM().get( MultiID(id) );
	if ( !ioobj )
	{
	    msg_ = "Cannot find object for "; mAddIdxRank(ipdf);
	    msg_ += " PDF in data store"; return false;
	}

	ProbDenFunc* pdf = ProbDenFuncTranslator::read( *ioobj, &msg_ );
	if ( !pdf )
	    return false;

	pdfs_ += pdf;
	pdfnames_.add( ioobj->name() );
    }

    if ( pdfs_.isEmpty() )
	{ msg_ = "No PDF's in parameters"; return false; }

    pdfnames_.add( "Classification" );
    pdfnames_.add( "Confidence" );
    return true;
}


bool SeisBayesClass::getReaders( const IOPar& iop )
{
    if ( pdfs_.isEmpty() ) return false;
    const ProbDenFunc& pdf0 = *pdfs_[0];
    const int nrvars = pdf0.nrDims();
    for ( int ivar=0; ivar<nrvars; ivar++ )
    {
	const char* id = iop.find( mGetSeisBayesSeisInpIDKey(ivar) );
	if ( !id || !*id )
	{
	    msg_ = "Cannot find "; mAddIdxRank(ivar);
	    msg_ += " input cube (for "; msg_ += pdf0.dimName(ivar);
	    msg_ += ") in parameters";
	    return false;
	}

	PtrMan<IOObj> ioobj = IOM().get( MultiID(id) );
	if ( !ioobj )
	{
	    msg_ = "Cannot find input cube for "; msg_ += pdf0.dimName(ivar);
	    msg_ += "\nID found is "; msg_ += id; return false;
	}

	SeisTrcReader* rdr = new SeisTrcReader( ioobj );
	rdr->usePar( iop );
	if ( !rdr->prepareWork() )
	    { msg_ = rdr->errMsg(); delete rdr; return false; }

	rdrs_ += rdr;
	inptrcs_.add( new SeisTrc );
    }

    return true;
}


bool SeisBayesClass::getWriters( const IOPar& iop )
{
    const int nrpdfs = pdfs_.size();
    if ( nrpdfs < 1 ) return false;

    wrrs_.allowNull( true ); bool haveoutput = false;
    for ( int ipdf=0; ipdf<nrpdfs+2; ipdf++ )
    {
	const char* id = iop.find( mGetSeisBayesSeisOutIDKey(ipdf) );
	if ( !id || !*id )
	    { wrrs_ += 0; continue; }
	else
	    haveoutput = true;

	PtrMan<IOObj> ioobj = IOM().get( MultiID(id) );
	if ( !ioobj )
	{
	    msg_ = "Cannot find output cube for ";
	    msg_ += pdfnames_.get( ipdf );
	    msg_ += "\nID found is "; msg_ += id; return false;
	}

	wrrs_ += new SeisTrcWriter( ioobj );
	outtrcs_.add( new SeisTrc );
    }

    if ( !haveoutput )
	{ msg_ = "No output specified in parameters"; return false; }

    needclass_ = wrrs_[nrpdfs] || wrrs_[nrpdfs+1];
    return true;
}


SeisBayesClass::~SeisBayesClass()
{
    deepErase(pdfs_);
    deepErase(rdrs_);
    deepErase(wrrs_);
}


const char* SeisBayesClass::message() const
{
    return msg_;
}


const char* SeisBayesClass::nrDoneText() const
{
    return "Positions handled";
}


od_int64 SeisBayesClass::nrDone() const
{
    return nrdone_;
}


od_int64 SeisBayesClass::totalNr() const
{
    if ( totalnr_ == -2 && !rdrs_.isEmpty() )
    {
	Seis::Bounds* sb = rdrs_[0]->getBounds();
	SeisBayesClass& self = *(const_cast<SeisBayesClass*>(this));
	self.totalnr_ = sb->expectedNrTraces();
	delete sb;
    }
    return totalnr_ < -1 ? -1 : totalnr_;
}


#include "timefun.h"

int SeisBayesClass::nextStep()
{
    if ( wrrs_.isEmpty() )
	return ErrorOccurred();

    int ret = readInpTrcs();
    if ( ret != 1 )
	return ret > 1	? MoreToDo()
	    : (ret == 0	? closeDown() : ErrorOccurred());;

    return createOutput() ? MoreToDo() : ErrorOccurred();
}


int SeisBayesClass::closeDown()
{
    return Finished();
}


int SeisBayesClass::readInpTrcs()
{
    SeisTrcInfo& ti0 = inptrcs_.get(0)->info();
    int ret = rdrs_[0]->get( ti0 );
    if ( ret != 1 )
    {
	if ( ret < 0 )
	    msg_ = rdrs_[0]->errMsg();
	return ret;
    }

    for ( int idx=0; idx<rdrs_.size(); idx++ )
    {
	if ( idx && !rdrs_[idx]->seisTranslator()->goTo( ti0.binid ) )
	    return 2;
	if ( !rdrs_[idx]->get(*inptrcs_.get(idx)) )
	{
	    msg_ = rdrs_[idx]->errMsg();
	    return Executor::ErrorOccurred();
	}
    }

    return 1;
}


int SeisBayesClass::createOutput()
{
    return MoreToDo();
}
