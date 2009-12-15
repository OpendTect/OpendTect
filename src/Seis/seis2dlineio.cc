/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert
 * DATE     : Dec 2009
-*/

static const char* rcsID = "$Id: seis2dlineio.cc,v 1.1 2009-12-15 12:20:18 cvsbert Exp $";

#include "seis2dlineio.h"
#include "seis2dline.h"
#include "seis2dlinemerge.h"
#include "seisselection.h"
#include "seisioobjinfo.h"
#include "seispacketinfo.h"
#include "bufstringset.h"
#include "cubesampling.h"
#include "posinfo.h"
#include "filegen.h"
#include "seisbuf.h"
#include "ioobj.h"


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
    if ( File_exists(bakfnm) )
	File_remove( bakfnm, mFile_NotRecursive );

    return File_remove( fnm, mFile_NotRecursive );
}


bool TwoDSeisTrcTranslator::implRename( const IOObj* ioobj, const char* newnm,
					const CallBack* cb ) const
{
    if ( !ioobj )
	return false;

    bool res = Translator::implRename( ioobj, newnm, cb );
    if ( !res ) 
	return false;

    Seis2DLineSet ls( *ioobj );
    return ls.rename( ioobj->name() );
}


bool TwoDSeisTrcTranslator::initRead_()
{
    errmsg = 0;
    if ( !conn->ioobj )
	{ errmsg = "Cannot reconstruct 2D filename"; return false; }
    BufferString fnm( conn->ioobj->fullUserExpr(true) );
    if ( !File_exists(fnm) ) return false;

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
    , tbuf_(*new SeisTrcBuf(false))
    , tbuf1_(*new SeisTrcBuf(false))
    , tbuf2_(*new SeisTrcBuf(false))
    , l2dd1_(*new PosInfo::Line2DData)
    , l2dd2_(*new PosInfo::Line2DData)
    , attrnms_(*new BufferStringSet)
    , opt_(MatchTrcNr)
    , numbering_(1,1)
    , renumber_(false)
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
    delete &tbuf1_;
    delete &tbuf2_;
    delete &tbuf_;
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
    currentlyreading_ = 1;
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
    currentlyreading_++;
    if ( currentlyreading_ > 2 )
	{ currentlyreading_ = 0; return false; }

    const int lid = currentlyreading_ == 1 ? lid1_ : lid2_;
    PosInfo::Line2DData& l2dd( currentlyreading_==1 ? l2dd1_ : l2dd2_ );
    const char* lnm = currentlyreading_ == 1 ? lnm1_.buf() : lnm2_.buf();
    SeisTrcBuf& tbuf = currentlyreading_==1 ? tbuf1_ : tbuf2_;
    tbuf.deepErase();

    if ( !ls_->getGeometry(lid,l2dd) )
	mErrRet("Cannot open")
    totnr_ = l2dd.posns_.size();
    if ( totnr_ < 0 )
	mErrRet("No data in")
    delete fetcher_;
    fetcher_ = ls_->lineFetcher( lid, tbuf );
    if ( !fetcher_ )
	mErrRet("Cannot create a reader for")


    nrdonemsg_ = "Traces read ("; nrdonemsg_ += lnm; nrdonemsg_ += ")";
    return true;
}


#undef mErrRet
#define mErrRet(s) { if ( s ) msg_ = s; return Executor::ErrorOccurred(); }

int Seis2DLineMerger::nextStep()
{
    if ( !oinf_.isOK() )
	mErrRet("Cannot find the Line Set")

    if ( !ls_ )
    {
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


    mErrRet("TODO: Not impl");
}
