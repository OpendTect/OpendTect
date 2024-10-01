/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

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
const char* SeisBayesClass::sKeyAPProbID()	{ return "PDF.Norm.APProb.ID"; }
const char* SeisBayesClass::sKeyPreNorm()	{ return "PDF.Norm.PreNorm"; }
const char* SeisBayesClass::sKeyPostNorm()	{ return "PDF.Norm.PostNorm"; }
const char* SeisBayesClass::sKeyPreScale()	{ return "PDF.Norm.PreScale"; }
const char* SeisBayesClass::sKeySeisInpID()	{ return "Seismics.Input.ID"; }
const char* SeisBayesClass::sKeySeisOutID()	{ return "Seismics.Output.ID"; }


#define mDefNrCompsSamps(trc) \
    const int nrcomps = (trc).nrComponents(); \
    const int nrsamps = (trc).size()


SeisBayesClass::SeisBayesClass( const IOPar& iop )
	: Executor( "Bayesian classification" )
	, inptrcs_(*new SeisTrcBuf(true))
	, aptrcs_(*new SeisTrcBuf(true))
	, outtrcs_(*new SeisTrcBuf(true))
	, pars_(*new IOPar(iop))
	, doprenorm_(!iop.isFalse( sKeyPreNorm() ))
	, dopostnorm_(!iop.isFalse( sKeyPostNorm() ))
	, nrdims_(0)
	, nrpdfs_(0)
	, needclass_(false)
	, nrdone_(0)
	, totalnr_(-2)
	, initstep_(1)
{
    aprdrs_.allowNull( true );

    const BufferString res = pars_.find( sKey::Type() );
    if ( res.isEmpty() )
	is2d_ = false;
    else
    {
	const BufferString firstchar( res[0] );
	is2d_ = firstchar.isEqual( "2" );
    }

    if ( is2d_ )
    { msg_ = tr("2D not implemented"); return; }

    msg_ = tr("Initializing");
}

#define mAddIdxRank(idx) arg( ("%1%2") \
			  .arg(toString(idx+1)) \
			  .arg( getRankPostFix(idx+1)) )


SeisBayesClass::~SeisBayesClass()
{
    cleanUp();

    delete &pars_;
    delete &inptrcs_;
    delete &aptrcs_;
    delete &outtrcs_;
}


void SeisBayesClass::cleanUp()
{
    deepErase(inppdfs_);
    deepErase(pdfs_);
    deepErase(rdrs_);
    deepErase(aprdrs_);
    deepErase(wrrs_);
}


bool SeisBayesClass::getPDFs()
{
    for ( int ipdf=0; ; ipdf++ )
    {
	MultiID mid;
	pars_.get( mGetSeisBayesPDFIDKey(ipdf), mid );
	if ( mid.isUdf() )
	    break;

	PtrMan<IOObj> ioobj = IOM().get( mid );
	if ( !ioobj )
	{
	    msg_ = tr("Cannot find object for %1 PDF in data store")
		 .arg(ipdf);
	    return false;
	}

	uiString errmsg;
	ProbDenFunc* pdf = ProbDenFuncTranslator::read( *ioobj, &errmsg );
	if ( !pdf )
	{
	    msg_ = errmsg;
	    return false;
	}

	inppdfs_ += pdf;
	pdfnames_.add( ioobj->name() );

	const ProbDenFunc& pdf0 = *inppdfs_[0];
	if ( ipdf == 0 )
	    const_cast<int&>(nrdims_) = pdf->nrDims();
	else if ( pdf->nrDims() != nrdims_ )
	{ msg_ = tr("PDF's have different dimensions"); return false; }
	else if ( !pdf->isCompatibleWith(pdf0) )
	{ msg_ = tr("PDF's are not compatible"); return false; }

	TypeSet<int>* idxs = new TypeSet<int>;
	pdf->getIndexTableFor( pdf0, *idxs );
	pdfxtbls_ += idxs;

	BufferString res = pars_.find( mGetSeisBayesPreScaleKey(ipdf) );
	float scl = 1;
	if ( !res.isEmpty() )
	    scl = res.toFloat();

	if ( scl < 0 )
	    scl = -scl;

	if ( scl == 0 )
	    scl = 1;

	prescales_ += scl;

	aptrcs_.add( new SeisTrc );

	MultiID key;
	pars_.get( mGetSeisBayesAPProbIDKey(ipdf), key );
	SeisTrcReader* rdr = nullptr;
	if ( !key.isUdf() )
	{
	    rdr = getReader( key, false, ipdf );
	    if ( !rdr )
		return false;
	}
	aprdrs_ += rdr;
    }

    const_cast<int&>(nrpdfs_) = inppdfs_.size();
    if ( nrpdfs_ < 1 )
    { msg_ = tr("No PDF's in parameters"); return false; }
    preScalePDFs();

    pdfinpvals_.setSize( nrdims_, 0 );
    pdfnames_.add( "Classification" );
    pdfnames_.add( "Confidence" );
    pdfnames_.add( "Determination" );
    initstep_ = 2;
    return true;
}


void SeisBayesClass::preScalePDFs()
{
    for ( int ipdf=0; ipdf<nrpdfs_; ipdf++ )
    {
	ProbDenFunc* pdf = inppdfs_[ipdf]->clone();
	pdfs_ += pdf;
	float prescl = prescales_[ipdf];
	if ( doprenorm_ )
	    prescl *= pdf->normFac();
	pdf->scale( prescl );
    }
}


SeisTrcReader* SeisBayesClass::getReader( const char* id, bool isdim, int idx )
{
    MultiID key;
    if ( !key.fromString(id) )
	return nullptr;

    return getReader( key, isdim, idx );
}


SeisTrcReader* SeisBayesClass::getReader( const MultiID& id, bool isdim,
					  int idx )
{
    PtrMan<IOObj> ioobj = IOM().get( id );
    if ( !ioobj )
    {
	const ProbDenFunc& pdf0 = *inppdfs_[0];
	if ( isdim )
	    msg_ = tr("Cannot find input cube for %1" ).arg(pdf0.dimName(idx));
	else
	    msg_ = tr( "Cannot find a priori scaling cube for %1"
		      "\nID found is %2").arg( pdf0.name() ).arg( id );

	return nullptr;
    }

    const Seis::GeomType gt = Seis::geomTypeOf( is2d_, false );
    auto* rdr = new SeisTrcReader( *ioobj, &gt );
    rdr->usePar( pars_ );
    if ( !rdr->prepareWork() )
    {
	msg_ = tr( "For %1:\n%2" ).arg( ioobj->name() ).arg( rdr->errMsg() );
	delete rdr; return nullptr;
    }

    return rdr;
}


bool SeisBayesClass::getReaders()
{
    if ( nrpdfs_ < 1 ) return false;
    const ProbDenFunc& pdf0 = *inppdfs_[0];

    for ( int idim=0; idim<nrdims_; idim++ )
    {
	inptrcs_.add( new SeisTrc );

	MultiID key;
	pars_.get( mGetSeisBayesSeisInpIDKey(idim), key );
	if ( key.isUdf() )
	{
	    msg_ = tr("Cannot find %1  input cube (for %2) in parameters")
		 .arg(idim).arg( pdf0.dimName(idim) );
	    return false;
	}

	SeisTrcReader* rdr = getReader( key, true, idim );
	if ( !rdr ) return false;
	rdrs_ += rdr;
    }

    initstep_ = 3;
    return true;
}


bool SeisBayesClass::getWriters()
{
    if ( nrpdfs_ < 1 )
	return false;

    const Seis::GeomType gt = Seis::geomTypeOf( is2d_, false );
    wrrs_.allowNull( true ); bool haveoutput = false;
    for ( int ipdf=0; ipdf<nrpdfs_+3; ipdf++ )
    {
	outtrcs_.add( new SeisTrc );
	MultiID mid;
	pars_.get( mGetSeisBayesSeisOutIDKey(ipdf), mid );
	if ( mid.isUdf() )
	{
	    wrrs_ += 0;
	    continue;
	}
	else
	    haveoutput = true;

	PtrMan<IOObj> ioobj = IOM().get( mid );
	if ( !ioobj )
	{
	    msg_ = tr("Cannot find output cube for %1"
		      "\nID found is %2)")
		 .arg( pdfnames_.get( ipdf ) )
		 .arg( mid );
	    return false;
	}

	wrrs_ += new SeisTrcWriter( *ioobj, &gt );
    }

    if ( !haveoutput )
    { msg_ = tr("No output specified in parameters"); return false; }

    const_cast<bool&>(needclass_) = wrrs_[nrpdfs_] || wrrs_[nrpdfs_+1];
    initstep_ = 0;
    msg_ = tr("Processing");
    return true;
}


uiString SeisBayesClass::uiMessage() const
{
    return msg_;
}


uiString SeisBayesClass::uiNrDoneText() const
{
    return initstep_ ? tr("Step") : tr("Positions handled");
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
	return (initstep_ == 1 ? getPDFs()
	     : (initstep_ == 2 ? getReaders()
			       : getWriters()))
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
	if ( idx && !rdrs_[idx]->seisTranslator()->goTo( ti0.binID() ) )
	    return 2;
	if ( !rdrs_[idx]->get(*inptrcs_.get(idx)) )
	{
	    msg_ = rdrs_[idx]->errMsg();
	    return Executor::ErrorOccurred();
	}
    }

    for ( int idx=0; idx<aprdrs_.size(); idx++ )
    {
	SeisTrcReader* rdr = aprdrs_[idx];
	if ( !rdr ) continue;

	if ( !rdr->seisTranslator()->goTo( ti0.binID() ) )
	    return 2;
	if ( !rdr->get(*aptrcs_.get(idx)) )
	{
	    msg_ = rdr->errMsg();
	    return Executor::ErrorOccurred();
	}
    }

    return 1;
}


#define mWrTrc(nr) { \
	wrr = wrrs_[nr]; \
	if ( wrr && !wrr->put(*outtrcs_.get(nr)) ) \
	    { msg_ = wrr->errMsg(); return ErrorOccurred(); } }

int SeisBayesClass::createOutput()
{
    for ( int ipdf=0; ipdf<nrpdfs_; ipdf++ )
	calcProbs( ipdf );
    if ( nrpdfs_>1 && dopostnorm_ )
	postScaleProbs();

    SeisTrcWriter* wrr;

    for ( int ipdf=0; ipdf<nrpdfs_; ipdf++ )
	mWrTrc(ipdf)

    if ( needclass_ )
    {
	calcClass();
	mWrTrc(nrpdfs_) mWrTrc(nrpdfs_+1)
    }

    if ( wrrs_[nrpdfs_+2] )
    {
	calcDet();
	mWrTrc(nrpdfs_+2)
    }

    nrdone_++;
    return MoreToDo();
}


void SeisBayesClass::prepOutTrc( SeisTrc& trc, bool isch ) const
{
    const SeisTrc& inptrc = *inptrcs_.get( 0 );
    if ( trc.isEmpty() )
    {
	const DataCharacteristics dc( isch ? DataCharacteristics::SI8
					   : DataCharacteristics::F32 );
	trc.data().setComponent( dc, 0 );
	for ( int icomp=0; icomp<inptrc.nrComponents(); icomp++ )
	{
	    if ( icomp < trc.nrComponents() )
		trc.data().setComponent( dc, icomp );
	    else
		trc.data().addComponent( inptrc.size(), dc );
	    trc.reSize( inptrc.size(), false );
	}
    }

    trc.info() = inptrc.info();
}


float SeisBayesClass::getPDFValue( int ipdf, int isamp, int icomp,
				   bool inp ) const
{
    const SeisTrc& inptrc0 = *inptrcs_.get( 0 );
    const SeisTrc& outtrc = *outtrcs_.get( ipdf );
    const float eps = inptrc0.info().sampling.step_ * 0.0001f;

    for ( int idim0=0; idim0<nrdims_; idim0++ )
    {
	const int idim = (*pdfxtbls_[ipdf])[idim0];
	const SeisTrc& inptrc = *inptrcs_.get( idim );
	const float z = outtrc.samplePos( isamp );
	if ( z < inptrc.startPos() - eps )
	    pdfinpvals_[idim] = inptrc.getFirst( icomp );
	else if ( z > inptrc.endPos() + eps )
	    pdfinpvals_[idim] = inptrc.getLast( icomp );
	else
	    pdfinpvals_[idim] = inptrc.getValue( z, icomp );
    }
    return ((inp ? inppdfs_ : pdfs_)[ipdf])->value( pdfinpvals_ );
}


float SeisBayesClass::getAPTrcVal( int ipdf, int isamp, int icomp )
{
    const SeisTrc& trc = *aptrcs_.get( ipdf );
    if ( icomp >= trc.nrComponents() ) icomp = 0;
    return trc.get( isamp, icomp );
}


void SeisBayesClass::postScaleProbs()
{
    mDefNrCompsSamps(*outtrcs_.get(0));
    for ( int icomp=0; icomp<nrcomps; icomp++ )
    {
	for ( int isamp=0; isamp<nrsamps; isamp++ )
	{
	    TypeSet<float> vals; float sumval = 0;
	    for ( int ipdf=0; ipdf<nrpdfs_; ipdf++ )
	    {
		const float val = outtrcs_.get(ipdf)->get(isamp,icomp);
		vals += val; sumval += val;
	    }
	    const bool haveout = sumval != 0;
	    for ( int ipdf=0; ipdf<nrpdfs_; ipdf++ )
	    {
		const float val = haveout ? vals[ipdf] / sumval : 0;
		outtrcs_.get(ipdf)->set( isamp, val, icomp );
	    }
	}
    }
}


void SeisBayesClass::calcProbs( int ipdf )
{
    SeisTrc& trc = *outtrcs_.get( ipdf ); prepOutTrc( trc, false );

    mDefNrCompsSamps(trc);
    for ( int icomp=0; icomp<nrcomps; icomp++ )
    {
	for ( int isamp=0; isamp<nrsamps; isamp++ )
	{
	    float val = getPDFValue( ipdf, isamp, icomp );
	    if ( aprdrs_[ipdf] )
		val *= getAPTrcVal( ipdf, isamp, icomp );
	    trc.set( isamp, val, icomp );
	}
    }
}


void SeisBayesClass::calcDet()
{
    SeisTrc& dettrc = *outtrcs_.get( nrpdfs_+2 ); prepOutTrc( dettrc, false );

    mDefNrCompsSamps(dettrc);
    for ( int icomp=0; icomp<nrcomps; icomp++ )
    {
	for ( int isamp=0; isamp<nrsamps; isamp++ )
	{
	    float sum = 0;
	    for ( int ipdf=0; ipdf<nrpdfs_; ipdf++ )
		sum += getPDFValue(ipdf,isamp,icomp,true);
	    dettrc.set( isamp, sum, icomp );
	}
    }
}


void SeisBayesClass::calcClass()
{
    SeisTrc& clsstrc = *outtrcs_.get( nrpdfs_ ); prepOutTrc( clsstrc, true );
    SeisTrc& conftrc = *outtrcs_.get( nrpdfs_+1 ); prepOutTrc( conftrc, false );

    TypeSet<float> probs( nrpdfs_, 0 ); int winner; float conf;
    mDefNrCompsSamps(clsstrc);
    for ( int icomp=0; icomp<nrcomps; icomp++ )
    {
	for ( int isamp=0; isamp<nrsamps; isamp++ )
	{
	    for ( int ipdf=0; ipdf<nrpdfs_; ipdf++ )
		probs[ipdf] = outtrcs_.get(ipdf)->get( isamp, icomp );
	    getClass( probs, winner, conf );
	    clsstrc.set( isamp, mCast(float,winner), icomp );
	    conftrc.set( isamp, conf, icomp );
	}
    }
}


void SeisBayesClass::getClass( const TypeSet<float>& probs, int& winner,
				float& conf ) const
{
    // users don't like class '0', so we add '1' to winner
    if ( probs.size() < 2 )
	{ conf = (float) (winner = 1); return; }

    winner = 0; float winnerval = probs[0];
    for ( int idx=1; idx<probs.size(); idx++ )
    {
	if ( probs[idx] > winnerval )
	    { winner = idx; winnerval = probs[idx]; }
    }
    if ( winnerval < 1e-20 )
	{ conf = 0; winner = -1; return; }

    float runnerupval = probs[winner ? 0 : 1];
    for ( int idx=0; idx<probs.size(); idx++ )
    {
	if ( idx == winner ) continue;
	if ( probs[idx] > runnerupval )
	    runnerupval = probs[idx];
    }

    winner++;
    conf = (winnerval - runnerupval) / winnerval;
}
