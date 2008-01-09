/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : A.H. Bril
 * DATE     : Oct 2003
-*/

static const char* rcsID = "$Id: seisimpbpsif.cc,v 1.4 2008-01-09 14:06:32 cvsbert Exp $";

#include "seisimpbpsif.h"
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
    				SeisPSImpDataMgr()	{}
				~SeisPSImpDataMgr()	{ deepErase(lines_); }

    void			add(SeisTrc*);

    ObjectSet<SeisPSImpLineBuf>	lines_;
};


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



SeisImpBPSIF::SeisImpBPSIF( const char* filenm, const MultiID& )
    	: Executor("Importing BPSIF data")
    	, pswrr_(0)
    	, datamgr_(*new SeisPSImpDataMgr)
    	, curfileidx_(-1)
    	, nrshots_(0)
    	, nrrcvpershot_(0)
    	, irregular_(false)
{
    if ( !filenm || !*filenm )
	return;

    FilePath fp( filenm );
    if ( !fp.isAbsolute() )
	fp.setPath( GetBaseDataDir() );

    const char* ptr = strchr( filenm, '*' );
    if ( !ptr )
    {
	fnames_.add( fp.fullPath() );
	return;
    }

    DirList dl( fp.pathOnly(), DirList::FilesOnly, fp.fileName() );
    for ( int idx=0; idx<dl.size(); idx++ )
	fnames_.add( dl.fullPath(idx) );
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
    if ( curfileidx_ > fnames_.size() )
	return false;

    if ( !readFileHeader() || !open( fnames_.get(curfileidx_) ) )
	return openNext();

    return true;
}


#define mErrRet(s) \
	{ errmsg_ = fnames_.get(curfileidx_); errmsg_ += s; return false; }


bool SeisImpBPSIF::readFileHeader()
{
    char buf[10];
    cursd_.istrm->getline( buf, 10 );
    if ( strcmp(buf,"#BPSIF#") )
	mErrRet(" is not a BPSIF file")
    if ( cursd_.istrm->peek() == '\n' ) cursd_.istrm->ignore( 1 );

    while ( true )
    {
	BufferString* lineread = new BufferString;
	if ( !StrmOper::readLine(*cursd_.istrm,lineread) )
	    return false;
	if ( lineread->isEmpty() )
	    { delete lineread; continue; }

	if ( matchString("#----",lineread->buf()) )
	    { delete lineread; break; }

	hdrlines_ += lineread;
    }

    if ( !(shotattrs_.isEmpty() && rcvattrs_.isEmpty()) )
	return true;

    static const char* valstr = "#Value.";
    for ( int idx=0; idx<hdrlines_.size(); idx++ )
    {
	const BufferString& ln = hdrlines_.get( idx );
	if ( matchString(valstr,ln.buf()) )
	{
	    const char* nrstr = ln.buf() + 7;
	    const char* attrstr = strchr( ln.buf(), ':' );
	    if ( !attrstr ) continue;
	    addAttr( *nrstr == '1' ? shotattrs_ : rcvattrs_, attrstr+1 );
	}
    }

    if ( shotattrs_.isEmpty() && rcvattrs_.isEmpty() )
	mErrRet(" does not contain any attribute data")
    return true;
}


void SeisImpBPSIF::addAttr( BufferStringSet& attrs, const char* attrstr )
{
    mSkipBlanks(attrstr);
    const char* ptr = strchr( attrstr, '=' );
    if ( !ptr ) ptr = attrstr;
    else
	{ ptr++; mSkipBlanks(ptr); }

    if ( *ptr )
	attrs.add( ptr );
}


const char* SeisImpBPSIF::message() const
{
    return errmsg_.isEmpty() ? "Importing traces" : errmsg_.buf();
}


int SeisImpBPSIF::nextStep()
{
    if ( curfileidx_ < 0 )
	return openNext() ? Executor::MoreToDo : Executor::ErrorOccurred;
    else if ( pswrr_ )
	return writeData();

    const int nrshotattrs = shotattrs_.size();
    float shotnr, x, y;
    std::istream& strm = *cursd_.istrm;
    SeisTrc tmpltrc( nrshotattrs + rcvattrs_.size() );
    strm >> shotnr >> x >> y;
    tmpltrc.info().coord.x = x; tmpltrc.info().coord.y = y;
    for ( int idx=0; idx<nrshotattrs; idx++ )
    {
	float val; strm >> val;
	tmpltrc.set( idx, val, 0 );
    }

    BufferString rcvdata;
    if ( !StrmOper::readLine(strm,&rcvdata) )
	return writeData();
    else
    {
	int nrpos = addTrcs( tmpltrc, rcvdata.buf() );
	if ( nrrcvpershot_ < 0 )
	    nrrcvpershot_ = nrpos;
	else if ( nrpos == 0 )
	    return writeData();
	else if ( nrpos != nrrcvpershot_ )
	    irregular_ = true;
	nrshots_++;
    }

    return Executor::MoreToDo;
}


static float getVal( char* data, char*& ptr )
{
    mSkipBlanks( ptr );
    if ( !*ptr ) return mUdf(float);

    const char* startptr = ptr;
    mSkipNonBlanks(ptr);
    if ( *ptr ) *ptr++ = '\0';

    return atof( startptr );
}


int SeisImpBPSIF::addTrcs( const SeisTrc& tmpltrc, char* data )
{
    const int nrrcvattrs = rcvattrs_.size();
    const int nrshotattrs = shotattrs_.size();

    SeisTrc* newtrc = new SeisTrc( tmpltrc );
    char* ptr = data;
    int nradded = 0;
    while ( true )
    {
	Coord rcvcoord;
	rcvcoord.x = getVal( data, ptr );
	rcvcoord.y = getVal( data, ptr );
	for ( int idx=0; idx<nrrcvattrs; idx++ )
	{
	    float val = getVal( data, ptr );
	    if ( mIsUdf(val) )
		{ delete newtrc; return nradded; }
	    newtrc->set( nrshotattrs+idx, val, 0 );
	}

	newtrc->info().coord += rcvcoord; newtrc->info().coord *= 0.5;
	newtrc->info().binid = SI().transform( newtrc->info().coord );
	datamgr_.add( newtrc );
	nradded++;
    }

    return nradded;
}


int SeisImpBPSIF::writeData()
{
    // Create pswrr_ if null
    // Write one inline
    return Executor::Finished;
}
