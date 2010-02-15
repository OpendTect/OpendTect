/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert
 * DATE     : Feb 2010
-*/

static const char* rcsID = "$Id: seisbayesclass.cc,v 1.3 2010-02-15 09:56:58 cvsbert Exp $";

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
	, pars_(*new IOPar(iop))
	, needclass_(false)
	, nrdone_(0)
	, totalnr_(-2)
	, inptrcs_(*new SeisTrcBuf(true))
	, outtrcs_(*new SeisTrcBuf(true))
	, initstep_(1)
{
    const char* res = iop.find( sKey::Type );
    is2d_ = res && *res == '2';
    if ( is2d_ )
	{ msg_ = "2D not implemented"; return; }

    msg_ = "Initializing";
}

#define mAddIdxRank(idx) msg_ += idx+1; msg_ += getRankPostFix(idx+1)


SeisBayesClass::~SeisBayesClass()
{
    cleanUp();

    delete &pars_;
    delete &inptrcs_;
    delete &outtrcs_;
}


void SeisBayesClass::cleanUp()
{
    deepErase(pdfs_);
    deepErase(rdrs_);
    deepErase(wrrs_);
}


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
    initstep_ = 2;
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

    initstep_ = 3;
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
    initstep_ = 0;
    msg_ = "Processing";
    return true;
}


const char* SeisBayesClass::message() const
{
    return msg_;
}


const char* SeisBayesClass::nrDoneText() const
{
    return initstep_ ? "Step" : "Positions handled";
}


od_int64 SeisBayesClass::nrDone() const
{
    return initstep_ ? initstep_ : nrdone_;
}


od_int64 SeisBayesClass::totalNr() const
{
    if ( initstep_ )
	return 4;

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
    if ( initstep_ )
	return (initstep_ == 1 ? getPDFs(pars_)
	     : (initstep_ == 2 ? getReaders(pars_)
		 	       : getWriters(pars_)))
	     ? MoreToDo() : ErrorOccurred();

    int ret = readInpTrcs();
    if ( ret != 1 )
	return ret > 1	? MoreToDo()
	    : (ret == 0	? closeDown() : ErrorOccurred());

    return createOutput() ? MoreToDo() : ErrorOccurred();
}


int SeisBayesClass::closeDown()
{
    cleanUp();
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


#define mWrTrc(nr) \
    	wrr = wrrs_[nr]; \
	if ( wrr && !wrr->put(*outtrcs_.get(nr)) ) \
	    { msg_ = wrr->errMsg(); return ErrorOccurred(); }

int SeisBayesClass::createOutput()
{
    const int nrpdfs = pdfs_.size();
    for ( int ipdf=0; ipdf<nrpdfs; ipdf++ )
    {
	if ( needclass_ || wrrs_[ipdf] )
	    calcProbs( ipdf );
	SeisTrcWriter* mWrTrc(ipdf)
    }

    if ( needclass_ )
    {
	calcClass();
	SeisTrcWriter* mWrTrc(nrpdfs)
	mWrTrc(nrpdfs+1)
    }

    return MoreToDo();
}


void SeisBayesClass::prepOutTrc( SeisTrc& trc ) const
{
    if ( trc.isEmpty() )
	trc = *inptrcs_.get(0);
    else
	trc.info() = inptrcs_.get(0)->info();
}


void SeisBayesClass::calcProbs( int ipdf )
{
    SeisTrc& trc = *outtrcs_.get( ipdf ); prepOutTrc( trc );
    const ProbDenFunc& pdf = *pdfs_[ipdf];
    const int nrdims = pdf.nrDims();

    TypeSet<float> inpvals( nrdims, 0 );
    for ( int icomp=0; icomp<trc.nrComponents(); icomp++ )
    {
	for ( int isamp=0; isamp<trc.size(); isamp++ )
	{
	    for ( int idim=0; idim<nrdims; idim++ )
		inpvals[idim] = inptrcs_.get(idim)->get( isamp, icomp );
	    trc.set( isamp, pdf.value(inpvals), icomp );
	}
    }
}


void SeisBayesClass::calcClass()
{
    const int nrpdfs = pdfs_.size();
    SeisTrc& clsstrc = *outtrcs_.get( nrpdfs ); prepOutTrc( clsstrc );
    SeisTrc& conftrc = *outtrcs_.get( nrpdfs+1 ); prepOutTrc( conftrc );

    TypeSet<float> probs( nrpdfs, 0 ); int winner; float conf;
    for ( int icomp=0; icomp<clsstrc.nrComponents(); icomp++ )
    {
	for ( int isamp=0; isamp<clsstrc.size(); isamp++ )
	{
	    for ( int iprob=0; iprob<nrpdfs; iprob++ )
		probs[iprob] = inptrcs_.get(iprob)->get( isamp, icomp );
	    getClass( probs, winner, conf );
	    clsstrc.set( isamp, winner, icomp );
	    clsstrc.set( isamp, conf, icomp );
	}
    }
}


void SeisBayesClass::getClass( const TypeSet<float>& probs, int& winner,
				float& conf ) const
{
    if ( probs.size() < 2 )
	{ winner = 0; conf = 1; return; }

    winner = 0; float winnerval = probs[0];
    for ( int idx=1; idx<probs.size(); idx++ )
    {
	if ( probs[idx] > winnerval )
	    { winner = idx; winnerval = probs[idx]; }
    }
    if ( winnerval < mDefEps )
	{ conf = 0; return; }

    int runnerup = winner ? 0 : 1; float runnerupval = probs[runnerup];
    for ( int idx=0; idx<probs.size(); idx++ )
    {
	if ( idx == winner ) continue;
	if ( probs[idx] > runnerupval )
	    { runnerup = idx; runnerupval = probs[idx]; }
    }

    conf = (winnerval - runnerupval) / winnerval;
}
