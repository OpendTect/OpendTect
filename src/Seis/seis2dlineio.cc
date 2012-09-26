/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert
 * DATE     : Dec 2009
-*/

static const char* rcsID mUsedVar = "$Id$";

#include "seis2dlineio.h"
#include "seis2dline.h"
#include "seis2dlinemerge.h"
#include "seisselection.h"
#include "seisioobjinfo.h"
#include "seispacketinfo.h"
#include "seistrc.h"
#include "seistrcprop.h"
#include "surv2dgeom.h"
#include "bufstringset.h"
#include "cubesampling.h"
#include "file.h"
#include "ioman.h"
#include "ioobj.h"
#include "posinfo2d.h"
#include "ptrman.h"
#include "seisbuf.h"
#include "sorting.h"


ObjectSet<Seis2DLineIOProvider>& S2DLIOPs()
{
    static ObjectSet<Seis2DLineIOProvider>* theinst = 0;
    if ( !theinst ) theinst = new ObjectSet<Seis2DLineIOProvider>;
    return *theinst;
}


bool TwoDSeisTrcTranslator::implRemove( const IOObj* ioobj ) const
{
    if ( !ioobj ) return true;
    BufferString fnm( ioobj->fullUserExpr(true) );
    Seis2DLineSet lg( fnm );
    const int nrlines = lg.nrLines();
    BufferStringSet nms;
    for ( int iln=0; iln<nrlines; iln++ )
	nms.add( lg.lineKey(iln) );
    for ( int iln=0; iln<nrlines; iln++ )
	lg.remove( nms.get(iln) );
    nms.erase();

    BufferString bakfnm( fnm ); bakfnm += ".bak";
    if ( File::exists(bakfnm) )
	File::remove( bakfnm );

    return File::remove( fnm );
}


bool TwoDSeisTrcTranslator::implRename( const IOObj* ioobj, const char* newnm,
					const CallBack* cb ) const
{
    if ( !ioobj )
	return false;

    PtrMan<IOObj> oldioobj = IOM().get( ioobj->key() );
    if ( !oldioobj ) return false;

    BufferString oldname( oldioobj->name() );
    Seis2DLineSet ls( *ioobj );
    if ( ls.rename(ioobj->name()) && !ls.renameFiles(ioobj->name()) )
	return false;

    PosInfo::POS2DAdmin().renameLineSet( oldname, ioobj->name() );
    
    return Translator::implRename( ioobj, newnm, cb );
}


bool TwoDSeisTrcTranslator::initRead_()
{
    errmsg = 0;
    if ( !conn->ioobj )
	{ errmsg = "Cannot reconstruct 2D filename"; return false; }
    BufferString fnm( conn->ioobj->fullUserExpr(true) );
    if ( !File::exists(fnm) ) return false;

    Seis2DLineSet lset( fnm );
    if ( lset.nrLines() < 1 )
	{ errmsg = "Line set is empty"; return false; }
    lset.getTxtInfo( 0, pinfo.usrinfo, pinfo.stdinfo );
    addComp( DataCharacteristics(), pinfo.stdinfo, Seis::UnknowData );

    if ( seldata )
	curlinekey = seldata->lineKey();
    if ( !curlinekey.lineName().isEmpty() && lset.indexOf(curlinekey) < 0 )
	{ errmsg = "Cannot find line key in line set"; return false; }
    CubeSampling cs( true );
    errmsg = lset.getCubeSampling( cs, curlinekey );

    insd.start = cs.zrg.start; insd.step = cs.zrg.step;
    innrsamples = (int)((cs.zrg.stop-cs.zrg.start) / cs.zrg.step + 1.5);
    pinfo.inlrg.start = cs.hrg.start.inl; pinfo.inlrg.stop = cs.hrg.stop.inl;
    pinfo.inlrg.step = cs.hrg.step.inl; pinfo.crlrg.step = cs.hrg.step.crl;
    pinfo.crlrg.start = cs.hrg.start.crl; pinfo.crlrg.stop = cs.hrg.stop.crl;
    return true;
}


Seis2DLineMerger::Seis2DLineMerger( const MultiID& lsky )
    : Executor("Merging linens")
    , oinf_(*new SeisIOObjInfo(lsky))
    , ls_(0)
    , fetcher_(0)
    , putter_(0)
    , outbuf_(*new SeisTrcBuf(false))
    , tbuf1_(*new SeisTrcBuf(false))
    , tbuf2_(*new SeisTrcBuf(false))
    , l2dd1_(*new PosInfo::Line2DData)
    , l2dd2_(*new PosInfo::Line2DData)
    , outl2dd_(*new PosInfo::Line2DData)
    , attrnms_(*new BufferStringSet)
    , opt_(MatchTrcNr)
    , numbering_(1,1)
    , renumber_(false)
    , stckdupl_(false)
    , snapdist_(0.01)
    , nrdone_(0)
    , totnr_(-1)
    , curattridx_(0)
    , currentlyreading_(0)
    , msg_("Opening files")
    , nrdonemsg_("Files opened")
{
}


Seis2DLineMerger::~Seis2DLineMerger()
{
    delete fetcher_;
    delete putter_;
    delete ls_;
    tbuf1_.deepErase();
    tbuf2_.deepErase();
    outbuf_.deepErase();

    delete &tbuf1_;
    delete &tbuf2_;
    delete &outbuf_;
    delete &attrnms_;
    delete &oinf_;
}


MultiID Seis2DLineMerger::lsID() const
{
    return oinf_.isOK() ? oinf_.ioObj()->key() : MultiID();
}


bool Seis2DLineMerger::getLineID( const char* lnm, int& lid ) const
{
    lid = ls_->indexOf( LineKey(lnm,attrnms_.get(curattridx_)) );
    if ( lid >= 0 )
	return true;

    lid = ls_->indexOf( LineKey(lnm) );
    if ( lid < 0 )
	lid = ls_->indexOfFirstOccurrence( lnm );
    return false;
}


bool Seis2DLineMerger::nextAttr()
{
    have1_ = have2_ = false;
    while ( !have1_ && !have2_ )
    {
	curattridx_++;
	if ( curattridx_ >= attrnms_.size() )
	    return false;

	have1_ = getLineID( lnm1_, lid1_ );
	have2_ = getLineID( lnm2_, lid2_ );
    }

    msg_ = "Merging '"; msg_ += attrnms_.get(curattridx_); msg_ += "'";
    currentlyreading_ = 0;
    return nextFetcher();
}



#define mErrRet(s) \
    { \
	msg_ = s; msg_ += " "; msg_ += lnm; msg_ += " ("; \
	msg_ += attrnms_.get(curattridx_); msg_ += ")"; \
	return false; \
    }

bool Seis2DLineMerger::nextFetcher()
{
    delete fetcher_; fetcher_ = 0;
    currentlyreading_++;
    if ( currentlyreading_ > 2 )
	{ currentlyreading_ = 0; return true; }

    const int lid = currentlyreading_ == 1 ? lid1_ : lid2_;
    PosInfo::Line2DData& l2dd( currentlyreading_==1 ? l2dd1_ : l2dd2_ );
    l2dd.setLineName( currentlyreading_ == 1 ? lnm1_ : lnm2_ );
    const char* lnm = l2dd.lineName().buf();
    SeisTrcBuf& tbuf = currentlyreading_==1 ? tbuf1_ : tbuf2_;
    tbuf.deepErase();

    S2DPOS().setCurLineSet( ls_->name() );
    if ( !S2DPOS().getGeometry(l2dd) )
	mErrRet("Cannot open")
    nrdone_ = 0;
    totnr_ = l2dd.positions().size();
    if ( totnr_ < 0 )
	mErrRet("No data in")
    fetcher_ = ls_->lineFetcher( lid, tbuf, 1 );
    if ( !fetcher_ )
	mErrRet("Cannot create a reader for")

    nrdonemsg_ = "Traces read";
    return true;
}


#undef mErrRet
#define mErrRet(s) { if ( s ) msg_ = s; return Executor::ErrorOccurred(); }

int Seis2DLineMerger::nextStep()
{
    if ( !oinf_.isOK() )
	mErrRet("Cannot find the Line Set")
    else if ( ls_ )
	return doWork();

    if ( attrnms_.isEmpty() )
    {
	BufferStringSet attrnms2;
	oinf_.getAttribNamesForLine( lnm1_, attrnms_ );
	oinf_.getAttribNamesForLine( lnm2_, attrnms2 );
	attrnms_.add( attrnms2, false );
	if ( attrnms_.isEmpty() )
	    mErrRet("Cannot find any attributes for these lines");
    }
    ls_ = new Seis2DLineSet( *oinf_.ioObj() );
    if ( ls_->nrLines() < 2 )
	mErrRet("Cannot find 2 lines in Line Set");

    curattridx_ = -1;
    msg_.setEmpty();
    if ( !nextAttr() )
    {
	if ( msg_.isEmpty() )
	    msg_ = "Cannot find any common attribute";
	mErrRet(0)
    }

    return Executor::MoreToDo();
}


int Seis2DLineMerger::doWork()
{
    if ( fetcher_ )
    {
	const int res = fetcher_->doStep();
	if ( res < 0 )
	    { msg_ = fetcher_->message(); return res; }
	else if ( res == 1 )
	    { nrdone_++; return Executor::MoreToDo(); }

	return nextFetcher() ? Executor::MoreToDo() : Executor::ErrorOccurred();
    }
    else if ( putter_ )
    {
	if ( nrdone_ >= outbuf_.size() )
	{
	    outbuf_.deepErase();
	    if ( !putter_->close() )
		mErrRet(putter_->errMsg())
	    delete putter_; putter_ = 0;
#	    define mRetNextAttr \
	    { \
		if ( nextAttr() ) \
		    return Executor::MoreToDo(); \
		else \
		{ \
		    outl2dd_.setLineName( outlnm_ ); \
		    PosInfo::POS2DAdmin().setGeometry( outl2dd_ ); \
		    return Executor::Finished(); \
		} \
	    }	
	    mRetNextAttr;
	}

	const SeisTrc& trc = *outbuf_.get( nrdone_ );
	if ( !putter_->put(trc) )
	    mErrRet(putter_->errMsg())

	PosInfo::Line2DPos pos( trc.info().nr );
	pos.coord_ = trc.info().coord;
	outl2dd_.add( pos );
	nrdone_++;
	return Executor::MoreToDo();
    }

    if ( tbuf1_.isEmpty() && tbuf2_.isEmpty() )
	mRetNextAttr;

    mergeBufs();
    if ( outbuf_.isEmpty() )
	mRetNextAttr;

    nrdone_ = 0;
    totnr_ = outbuf_.size();
    IOPar* lineiopar = new IOPar;
    LineKey lk( outlnm_, attrnms_.get(curattridx_) );
    lk.fillPar( *lineiopar, true );
    putter_ = ls_->linePutter( lineiopar );
    if ( !putter_ )
	mErrRet("Cannot create writer for output line");

    nrdonemsg_ = "Traces written";
    return Executor::MoreToDo();
}


void Seis2DLineMerger::mergeBufs()
{
    makeBufsCompat();

    if ( opt_ == MatchCoords && !tbuf1_.isEmpty() && !tbuf2_.isEmpty() )
	mergeOnCoords();
    else
    {
	outbuf_.stealTracesFrom( tbuf1_ );
	outbuf_.stealTracesFrom( tbuf2_ );
	if ( opt_ == MatchTrcNr )
	{
	    outbuf_.sort( true, SeisTrcInfo::TrcNr );
	    outbuf_.enforceNrTrcs( 1, SeisTrcInfo::TrcNr, stckdupl_ );
	}
    }

    if ( renumber_ )
    {
	for ( int idx=0; idx<outbuf_.size(); idx++ )
	    outbuf_.get( idx )->info().nr = numbering_.atIndex( idx );
    }
}


void Seis2DLineMerger::makeBufsCompat()
{
    if ( tbuf1_.isEmpty() || tbuf2_.isEmpty() )
	return;

    const SeisTrc& trc10 = *tbuf1_.get( 0 );
    const int trcsz = trc10.size();
    const int trcnc = trc10.nrComponents();
    const SamplingData<float> trcsd = trc10.info().sampling;

    const SeisTrc& trc20 = *tbuf2_.get( 0 );
    if ( trc20.size() == trcsz && trc20.info().sampling == trcsd
      && trc20.nrComponents() == trcnc )
	return;

    for ( int itrc=0; itrc<tbuf2_.size(); itrc++ )
    {
	SeisTrc& trc = *tbuf2_.get( itrc );
	SeisTrc cptrc( trc );		// Copy old data for values
	trc = trc10;			// Get entire structure as trc10
	trc.info() = cptrc.info();	// Yet, keep old info
	trc.info().sampling = trcsd;	// ... but do take the samplingdata

	for ( int icomp=0; icomp<trcnc; icomp++ )
	{
	    const bool havethiscomp = icomp < cptrc.nrComponents();
	    for ( int isamp=0; isamp<trcsz; isamp++ )
	    {
		if ( !havethiscomp )
		    trc.set( isamp, 0, icomp );
		else
		{
		    const float z = trc.samplePos( isamp );
		    trc.set( isamp, cptrc.getValue(z,icomp), icomp );
		}
	    }
	}
    }
}


/* Algo:

   1) Determine points furthest away from other line
   2) Project all points on that line, lpar is then a sorting key
   3) sort, make outbuf accoring to that, and snap if needed
*/

void Seis2DLineMerger::mergeOnCoords()
{
    const int nrtrcs1 = tbuf1_.size() - 1;
    const int nrtrcs2 = tbuf2_.size() - 1;
    const Coord c10( tbuf1_.get(0)->info().coord );
    const Coord c11( tbuf1_.get(nrtrcs1)->info().coord );
    const Coord c20( tbuf2_.get(0)->info().coord );
    const Coord c21( tbuf2_.get(nrtrcs2)->info().coord );
    const double sqd10 = c10.sqDistTo( c20 ) + c10.sqDistTo( c21 );
    const double sqd11 = c11.sqDistTo( c20 ) + c11.sqDistTo( c21 );
    const double sqd20 = c20.sqDistTo( c10 ) + c20.sqDistTo( c11 );
    const double sqd21 = c21.sqDistTo( c10 ) + c21.sqDistTo( c11 );
    const Coord lnstart( sqd11 > sqd10 ? c11 : c10 );
    const Coord lnend( sqd21 > sqd20 ? c21 : c20 );
    const Coord lndelta( lnend.x - lnstart.x, lnend.y - lnstart.y );
    const Coord sqlndelta( lndelta.x * lndelta.x, lndelta.y * lndelta.y );
    const double sqabs = sqlndelta.x + sqlndelta.y;
    if ( sqabs < 0.001 )
	return;

    TypeSet<double> lpars; TypeSet<int> idxs;
    for ( int ibuf=0; ibuf<2; ibuf++ )
    {
	const int ntr = ibuf ? nrtrcs2 : nrtrcs1;
	for ( int idx=0; idx<ntr; idx++ )
	{
	    const SeisTrcBuf& tb( ibuf ? tbuf2_ : tbuf1_ );
	    const Coord& ctrc( tb.get(idx)->info().coord );
	    const Coord crel( ctrc.x - lnstart.x, ctrc.y - lnstart.y );
	    const double lpar = (lndelta.x * crel.x + lndelta.y * crel.y)
			      / sqabs;
	    // const Coord projrelpt( lpar * lndelta.x, lpar.lndelta.y );

	    lpars += lpar;
	    idxs += ibuf ? nrtrcs1 + idx : idx;
	}
    }

    sort_coupled( lpars.arr(), idxs.arr(), lpars.size() );
    doMerge( idxs, true );
}


void Seis2DLineMerger::doMerge( const TypeSet<int>& idxs, bool snap )
{
    const int nrtrcs1 = tbuf1_.size() - 1;
    for ( int idx=0; idx<idxs.size(); idx++ )
    {
	const int globidx = idxs[idx];
	const bool is1 = globidx < nrtrcs1;
	const int bufidx = globidx - (is1 ? 0 : nrtrcs1);
	outbuf_.add( (is1 ? tbuf1_ : tbuf2_).get(bufidx) );
    }
    tbuf1_.erase(); tbuf2_.erase();
    if ( !snap ) return;

    const double sqsnapdist = snapdist_ * snapdist_;
    int nrsnapped = 0;
    for ( int itrc=1; itrc<outbuf_.size(); itrc++ )
    {
	SeisTrc* prvtrc = outbuf_.get( itrc-1 );
	SeisTrc* curtrc = outbuf_.get( itrc );
	const double sqdist = curtrc->info().coord.sqDistTo(
			      prvtrc->info().coord );
	if ( sqdist > sqsnapdist )
	    nrsnapped = 0;
	else
	{
	    nrsnapped++;
	    if ( stckdupl_ )
		SeisTrcPropChg( *prvtrc )
		    .stack( *curtrc, false, 1.f / ((float)nrsnapped) );

	    delete outbuf_.remove( itrc );
	    itrc--;
	}
    }
}
