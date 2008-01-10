/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : A.H. Bril
 * DATE     : Oct 2003
-*/

static const char* rcsID = "$Id: seisimpbpsif.cc,v 1.6 2008-01-10 13:12:35 cvsbert Exp $";

#include "seisimpbpsif.h"
#include "seispsioprov.h"
#include "seispswrite.h"
#include "seisbuf.h"
#include "seistrc.h"
#include "strmprov.h"
#include "strmoper.h"
#include "dirlist.h"
#include "filepath.h"
#include "ioman.h"
#include "ioobj.h"
#include "oddirs.h"
#include "survinfo.h"
#include "cubesampling.h"
#include <iostream>

class SeisPSImpLineBuf
{
public:

    				SeisPSImpLineBuf( int inl )
				    : inl_(inl)		{}
				~SeisPSImpLineBuf()	{ deepErase(gathers_); }

    void			add(SeisTrc*);

    const int			inl_;
    ObjectSet<SeisTrcBuf>	gathers_;

};


void SeisPSImpLineBuf::add( SeisTrc* trc )
{
    const int crl = trc->info().binid.crl;
    int bufidx = -1;
    for ( int idx=0; idx<gathers_.size(); idx++ )
    {
	SeisTrcBuf& tbuf = *gathers_[idx];
	const int bufcrl = tbuf.get(0)->info().binid.crl;
	if ( bufcrl == crl )
	    { tbuf.add( trc ); return; }
	else if ( bufcrl > crl )
	    { bufidx = idx; break; }
    }

    SeisTrcBuf* newbuf = new SeisTrcBuf( true );
    newbuf->add( trc );
    if ( bufidx == -1 )
	gathers_ += newbuf;
    else
	gathers_.insertAt( newbuf, bufidx );
}


class SeisPSImpDataMgr
{
public:
    				SeisPSImpDataMgr()
				: writeupto_(-1)	{}
				~SeisPSImpDataMgr()	{ deepErase(lines_); }

    void			add(SeisTrc*);
    void			endReached();
    void			updateStatus()		{}
    				//TODO figure out whether part can be written
    bool			writeGather(SeisPSWriter&);

    bool			needWrite() const
    				{ return writeupto_ >= 0; }

    ObjectSet<SeisPSImpLineBuf>	lines_;
    int				writeupto_;
    mutable BufferString	errmsg_;
};


void SeisPSImpDataMgr::endReached()
{
    writeupto_ = lines_.size() - 1;
}


void SeisPSImpDataMgr::add( SeisTrc* trc )
{
    const int inl = trc->info().binid.inl;
    int bufidx = -1;
    for ( int idx=0; idx<lines_.size(); idx++ )
    {
	SeisPSImpLineBuf& lbuf = *lines_[idx];
	if ( lbuf.inl_ == inl )
	    { lbuf.add( trc ); return; }
	else if ( lbuf.inl_ > inl )
	    { bufidx = idx; break; }
    }

    SeisPSImpLineBuf* newbuf = new SeisPSImpLineBuf( inl );
    newbuf->add( trc );
    if ( bufidx == -1 )
	lines_ += newbuf;
    else
	lines_.insertAt( newbuf, bufidx );
}


bool SeisPSImpDataMgr::writeGather( SeisPSWriter& pswr )
{
    if ( !needWrite() ) return true;

    SeisPSImpLineBuf& lbuf = *lines_[0];
    SeisTrcBuf* gath2write = lbuf.gathers_.remove( 0 );
    if ( lbuf.gathers_.isEmpty() )
    {
	delete lines_.remove( 0 );
	writeupto_--;
    }

    bool res = true;
    for ( int idx=0; idx<gath2write->size(); idx++ )
    {
	res = pswr.put( *gath2write->get(idx) );
	if ( !res ) break;
    }

    delete gath2write;
    return res;
}


SeisImpBPSIF::SeisImpBPSIF( const char* filenm, const MultiID& id )
    	: Executor("Importing BPSIF data")
    	, psid_(id)
    	, pswrr_(0)
    	, datamgr_(*new SeisPSImpDataMgr)
    	, curfileidx_(-1)
    	, nrshots_(0)
    	, nrrejected_(0)
    	, nrrcvpershot_(-1)
    	, binary_(false)
    	, irregular_(false)
{
    if ( !filenm || !*filenm )
	return;

    FilePath fp( filenm );
    if ( !fp.isAbsolute() )
	fp.setPath( GetBaseDataDir() );

    const char* ptr = strchr( filenm, '*' );
    if ( !ptr )
	fnames_.add( fp.fullPath() );
    else
    {
	DirList dl( fp.pathOnly(), DirList::FilesOnly, fp.fileName() );
	for ( int idx=0; idx<dl.size(); idx++ )
	    fnames_.add( dl.fullPath(idx) );
    }

    if ( fnames_.isEmpty() )
	{ errmsg_ = "No valid input file specified"; return; }

    IOObj* ioobj = IOM().get( psid_ );
    if ( !ioobj )
	{ errmsg_ = "Output data store not found in object manager"; return; }
    pswrr_ = SPSIOPF().getWriter( *ioobj );
    delete ioobj;
    if ( !pswrr_ )
	{ errmsg_ = "Cannot write to this data store type"; return; }
}


SeisImpBPSIF::~SeisImpBPSIF()
{
    cursd_.close();
    delete &datamgr_;
    delete pswrr_;
}


bool SeisImpBPSIF::open( const char* fnm )
{
    cursd_.close();
    cursd_ = StreamProvider(fnm).makeIStream();
    return cursd_.usable();
}


bool SeisImpBPSIF::openNext()
{
    curfileidx_++;
    if ( curfileidx_ >= fnames_.size() )
	return false;

    if ( !open( fnames_.get(curfileidx_) ) || !readFileHeader() )
	return openNext();

    return true;
}


#define mErrRet(s) \
	{ errmsg_ = fnames_.get(curfileidx_); errmsg_ += s; return false; }


bool SeisImpBPSIF::readFileHeader()
{
    char buf[10];
    cursd_.istrm->getline( buf, 10 ); removeTrailingBlanks(buf);
    if ( strcmp(buf,"#BPSIF#") )
	mErrRet(" is not a BPSIF file")
    if ( cursd_.istrm->peek() == '\n' ) cursd_.istrm->ignore( 1 );

    while ( true )
    {
	BufferString* lineread = new BufferString;
	if ( !StrmOper::readLine(*cursd_.istrm,lineread) )
	    return false;
	else if ( matchString("#BINARY",lineread->buf()) )
	    { binary_ = true; nrrcvpershot_ = atoi(lineread->buf()+9); }
	else if ( matchString("#----",lineread->buf()) )
	    { delete lineread; break; }

	hdrlines_ += lineread;
    }

    if ( !(shotattrs_.isEmpty() && rcvattrs_.isEmpty()) )
	return true;

    static const char* valstr = "#Value.";
    for ( int idx=0; idx<hdrlines_.size(); idx++ )
    {
	BufferString ln = hdrlines_.get( idx );
	if ( matchString(valstr,ln.buf()) )
	{
	    const char* nrstr = ln.buf() + 7;
	    char* attrstr = strchr( ln.buf(), ':' );
	    if ( !attrstr ) continue;
	    *attrstr++ = '\0';
	    char* subnrstr = ln.buf() + 9;
	    removeTrailingBlanks( subnrstr );
	    if ( !strcmp(subnrstr,"0") || !strcmp(subnrstr,"1") )
		continue; // coordinates

	    addAttr( *nrstr == '1' ? shotattrs_ : rcvattrs_, attrstr );
	}
    }

    if ( shotattrs_.isEmpty() && rcvattrs_.isEmpty() )
	mErrRet(" does not contain any attribute data")
    return true;
}


void SeisImpBPSIF::addAttr( BufferStringSet& attrs, char* attrstr )
{
    mSkipBlanks(attrstr);
    char* ptr = strchr( attrstr, '=' );
    if ( !ptr ) ptr = attrstr;
    else	ptr++;
    mTrimBlanks(ptr);

    if ( *ptr )
	attrs.add( ptr );
}


const char* SeisImpBPSIF::message() const
{
    if ( !errmsg_.isEmpty() ) return errmsg_.buf();
    return datamgr_.needWrite() ? "Writing to data store" : "Reading traces";
}


int SeisImpBPSIF::nextStep()
{
    if ( !pswrr_ )
	return Executor::ErrorOccurred;
    else if ( curfileidx_ < 0 )
	return openNext() ? Executor::MoreToDo : Executor::ErrorOccurred;
    else if ( datamgr_.needWrite() )
	return writeData();

    return binary_ ? readBinary() : readAscii();
}


int SeisImpBPSIF::readAscii()
{
    std::istream& strm = *cursd_.istrm;
    const int nrshotattrs = shotattrs_.size();
    SeisTrc tmpltrc( nrshotattrs + rcvattrs_.size() );
    tmpltrc.info().sampling.start = SI().zRange(true).start;
    tmpltrc.info().sampling.step = SI().zStep();
    strm >> tmpltrc.info().coord.x >> tmpltrc.info().coord.y;
    for ( int idx=0; idx<nrshotattrs; idx++ )
    {
	float val; strm >> val;
	tmpltrc.set( idx, val, 0 );
	if ( idx == 0 )
	    tmpltrc.info().nr = mNINT(val);
    }

    BufferString rcvdata;
    if ( !StrmOper::readLine(strm,&rcvdata) )
	return fileEnded();
    else
    {
	int nrpos = addTrcsAscii( tmpltrc, rcvdata.buf() );
	if ( nrrcvpershot_ < 0 )
	    nrrcvpershot_ = nrpos;
	else if ( nrpos == 0 )
	    return fileEnded();
	else if ( nrpos != nrrcvpershot_ )
	    irregular_ = true;
	nrshots_++;
    }

    datamgr_.updateStatus();
    return Executor::MoreToDo;
}


int SeisImpBPSIF::readBinary()
{
    std::istream& strm = *cursd_.istrm;
    const int nrshotattrs = shotattrs_.size();
    const int nrrcvattrs = rcvattrs_.size();
    float vbuf[2+nrshotattrs];
    SeisTrc tmpltrc( nrshotattrs + nrrcvattrs );
    tmpltrc.info().sampling.start = SI().zRange(true).start;
    tmpltrc.info().sampling.step = SI().zStep();
    StrmOper::readBlock( strm, vbuf, (2+nrshotattrs)*sizeof(float) );
    tmpltrc.info().coord.x = vbuf[0]; tmpltrc.info().coord.y = vbuf[1];
    for ( int idx=0; idx<nrshotattrs; idx++ )
	tmpltrc.set( idx, vbuf[2+idx], 0 );
    tmpltrc.info().nr = mNINT(vbuf[0]);

    return addTrcsBinary( tmpltrc ) ? fileEnded() : Executor::MoreToDo;
}


static double getVal( char* data, char*& ptr )
{
    mSkipBlanks( ptr );
    if ( !*ptr ) return mUdf(float);

    const char* startptr = ptr;
    mSkipNonBlanks(ptr);
    if ( *ptr ) *ptr++ = '\0';

    return atof( startptr );
}


int SeisImpBPSIF::addTrcsAscii( const SeisTrc& tmpltrc, char* data )
{
    const int nrrcvattrs = rcvattrs_.size();
    const int nrshotattrs = shotattrs_.size();

    char* ptr = data;
    int nrfound = 0;
    while ( true )
    {
	SeisTrc* newtrc = new SeisTrc( tmpltrc );
	Coord rcvcoord;
	rcvcoord.x = getVal( data, ptr );
	rcvcoord.y = getVal( data, ptr );
	for ( int idx=0; idx<nrrcvattrs; idx++ )
	{
	    float val = (float)getVal( data, ptr );
	    if ( mIsUdf(val) )
		{ delete newtrc; return nrfound; }
	    newtrc->set( nrshotattrs+idx, val, 0 );
	}

	newtrc->info().setPSFlds( rcvcoord, tmpltrc.info().coord, true );
	if ( SI().sampling(false).hrg.includes(newtrc->info().binid) )
	    datamgr_.add( newtrc );
	else
	    nrrejected_++;

	nrfound++;
    }

    return nrfound;
}


bool SeisImpBPSIF::addTrcsBinary( const SeisTrc& tmpltrc )
{
    const int nrrcvattrs = rcvattrs_.size();
    const int nrshotattrs = shotattrs_.size();

    float vbuf[2+nrrcvattrs];
    for ( int idx=0; idx<nrrcvpershot_; idx++ )
    {
	if ( !StrmOper::readBlock( *cursd_.istrm, vbuf,
		    		   (2+nrshotattrs)*sizeof(float) ) )
	    return false;

	SeisTrc* newtrc = new SeisTrc( tmpltrc );
	Coord rcvcoord; rcvcoord.x = vbuf[0]; rcvcoord.y = vbuf[1];
	for ( int idx=0; idx<nrrcvattrs; idx++ )
	    newtrc->set( nrshotattrs+idx, vbuf[2+idx], 0 );

	newtrc->info().setPSFlds( rcvcoord, tmpltrc.info().coord, true );
	datamgr_.add( newtrc );
    }

    datamgr_.updateStatus();
    return true;
}


int SeisImpBPSIF::fileEnded()
{
    if ( openNext() )
	return Executor::MoreToDo;

    datamgr_.endReached();
    return writeData();
}


int SeisImpBPSIF::writeData()
{
    if ( !datamgr_.needWrite() )
	return datamgr_.lines_.isEmpty() ? Executor::Finished
	    				 : Executor::MoreToDo;

    if ( !datamgr_.writeGather(*pswrr_) )
    {
	errmsg_ = datamgr_.errmsg_;
	return Executor::ErrorOccurred;
    }

    return Executor::MoreToDo;
}
