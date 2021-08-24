/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : A.H. Bril
 * DATE     : Oct 2003
-*/


#include "seisimpbpsif.h"
#include "seisimpps.h"
#include "seistrc.h"
#include "strmoper.h"
#include "dirlist.h"
#include "filepath.h"
#include "ioman.h"
#include "ioobj.h"
#include "oddirs.h"
#include "survinfo.h"
#include "trckeyzsampling.h"
#include "od_istream.h"
#include <iostream>


SeisImpBPSIF::SeisImpBPSIF( const char* filenm, const MultiID& id )
    : Executor("Importing BPSIF data")
    , datamgr_(*new SeisPSImpDataMgr(id))
    , curfileidx_(-1)
    , nrshots_(0)
    , nrrejected_(0)
    , curstrm_(0)
    , nrrcvpershot_(-1)
    , binary_(false)
    , irregular_(false)
    , endofinput_(false)
{
    if ( !filenm || !*filenm )
	return;

    FilePath fp( filenm );
    if ( !fp.isAbsolute() )
	fp.setPath( GetBaseDataDir() );

    const char* ptr = firstOcc( filenm, '*' );
    if ( !ptr )
	fnames_.add( fp.fullPath() );
    else
    {
	DirList dl( fp.pathOnly(), File::FilesInDir, fp.fileName() );
	for ( int idx=0; idx<dl.size(); idx++ )
	    fnames_.add( dl.fullPath(idx) );
    }

    if ( fnames_.isEmpty() )
    { errmsg_ = tr("No valid input file specified"); return; }
}


SeisImpBPSIF::~SeisImpBPSIF()
{
    delete curstrm_;
    delete &datamgr_;
}


void SeisImpBPSIF::setMaxInlOffset( int mo )
{
    datamgr_.setMaxInlOffset( mo );
}


bool SeisImpBPSIF::open( const char* fnm )
{
    delete curstrm_;
    curstrm_ = new od_istream( fnm );
    return curstrm_->isOK();
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
	{ \
	    errmsg_ = s; \
	    return false; \
	} \


bool SeisImpBPSIF::readFileHeader()
{
    BufferString buf;
    curstrm_->getLine( buf ); removeTrailingBlanks(buf.getCStr());
    if ( buf != "#BPSIF#" )
	mErrRet( toUiString(" is not a BPSIF file") )
    if ( curstrm_->peek() == '\n' )
	curstrm_->ignore( 1 );

    while ( true )
    {
	BufferString* lineread = new BufferString;
	if ( !curstrm_->getLine(*lineread) )
	    return false;
	else if ( lineread->startsWith("#BINARY") )
	    { binary_ = true; nrrcvpershot_ = toInt(lineread->buf()+9); }
	else if ( lineread->startsWith("#----") )
	    { delete lineread; break; }

	hdrlines_ += lineread;
    }

    if ( !(shotattrs_.isEmpty() && rcvattrs_.isEmpty()) )
	return true;

    static const char* valstr = "#Value.";
    for ( int idx=0; idx<hdrlines_.size(); idx++ )
    {
	BufferString ln = hdrlines_.get( idx );
	if ( ln.startsWith(valstr) )
	{
	    const char* nrstr = ln.buf() + 7;
	    char* attrstr = ln.find( ':' );
	    if ( !attrstr ) continue;
	    *attrstr++ = '\0';
	    char* subnrstr = ln.getCStr() + 9;
	    removeTrailingBlanks( subnrstr );
	    const FixedString fssubnrstr = FixedString(subnrstr);
	    if ( fssubnrstr == "0" || fssubnrstr == "1" || fssubnrstr == "2" )
		continue; // coordinates

	    addAttr( *nrstr == '1' ? shotattrs_ : rcvattrs_, attrstr );
	}
    }

    if ( shotattrs_.isEmpty() && rcvattrs_.isEmpty() )
	mErrRet(tr("%1 does not contain any attribute data")
	      .arg(fnames_.get(curfileidx_)))

    BufferStringSet sampnms( shotattrs_ );
    sampnms.add( rcvattrs_, true );
    datamgr_.setSampleNames( sampnms );
    return true;
}


void SeisImpBPSIF::addAttr( BufferStringSet& attrs, char* attrstr )
{
    mSkipBlanks(attrstr);
    char* ptr = firstOcc( attrstr, '=' );
    if ( !ptr ) ptr = attrstr;
    else	ptr++;
    mTrimBlanks(ptr);

    if ( *ptr )
	attrs.add( ptr );
}


uiString SeisImpBPSIF::uiMessage() const
{
    if (!errmsg_.isEmpty()) return errmsg_;
    return datamgr_.needWrite() ? tr("Writing to data store")
				: tr("Reading traces");
}


int SeisImpBPSIF::nextStep()
{
    if ( fnames_.isEmpty() )
	return Executor::ErrorOccurred();
    else if ( curfileidx_ < 0 )
	return openNext() ? Executor::MoreToDo() : Executor::ErrorOccurred();
    else if ( datamgr_.needWrite() )
	return writeData();
    else if ( endofinput_ )
	return Executor::Finished();

    return binary_ ? readBinary() : readAscii();
}


int SeisImpBPSIF::readAscii()
{
    std::istream& strm = curstrm_->stdStream();
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
	    tmpltrc.info().nr = mNINT32(val);
    }

    BufferString rcvdata;
    if ( !StrmOper::readLine(strm,&rcvdata) )
	return fileEnded();
    else
    {
	int nrpos = addTrcsAscii( tmpltrc, rcvdata.getCStr() );
	if ( nrrcvpershot_ < 0 )
	    nrrcvpershot_ = nrpos;
	else if ( nrpos == 0 )
	    return fileEnded();
	else if ( nrpos != nrrcvpershot_ )
	    irregular_ = true;
	nrshots_++;
    }

    return Executor::MoreToDo();
}


int SeisImpBPSIF::readBinary()
{
    std::istream& strm = curstrm_->stdStream();
    const int nrshotattrs = shotattrs_.size();
    const int nrrcvattrs = rcvattrs_.size();
    mAllocVarLenArr( float, vbuf, 2+nrshotattrs );
    SeisTrc tmpltrc( nrshotattrs + nrrcvattrs );
    tmpltrc.info().sampling.start = SI().zRange(true).start;
    tmpltrc.info().sampling.step = SI().zStep();
    StrmOper::readBlock( strm, vbuf, (2+nrshotattrs)*sizeof(float) );
    tmpltrc.info().coord.x = vbuf[0]; tmpltrc.info().coord.y = vbuf[1];
    for ( int idx=0; idx<nrshotattrs; idx++ )
	tmpltrc.set( idx, vbuf[2+idx], 0 );
    tmpltrc.info().nr = mNINT32(vbuf[0]);

    if ( !addTrcsBinary(tmpltrc) )
	return fileEnded();

    nrshots_++;
    return Executor::MoreToDo();
}


static double getVal( char* data, char*& ptr )
{
    mSkipBlanks( ptr );
    if ( !*ptr ) return mUdf(float);

    const char* startptr = ptr;
    mSkipNonBlanks(ptr);
    if ( *ptr ) *ptr++ = '\0';

    return toDouble( startptr );
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
	if ( SI().sampling(false).hsamp_.includes(newtrc->info().binid) )
	    datamgr_.add( newtrc );
	else
	{
	    delete newtrc;
	    nrrejected_++;
	}

	nrfound++;
    }

    return nrfound;
}


bool SeisImpBPSIF::addTrcsBinary( const SeisTrc& tmpltrc )
{
    const int nrrcvattrs = rcvattrs_.size();
    const int nrshotattrs = shotattrs_.size();

    const int nrrcvvals = 2+nrrcvattrs;
    mAllocVarLenArr( float, vbuf, nrrcvvals );
    for ( int idx=0; idx<nrrcvpershot_; idx++ )
    {
	if ( !StrmOper::readBlock( curstrm_->stdStream(), vbuf,
				   nrrcvvals*sizeof(float) ) )
	    return false;

	SeisTrc* newtrc = new SeisTrc( tmpltrc );
	Coord rcvcoord; rcvcoord.x = vbuf[0]; rcvcoord.y = vbuf[1];
	for ( int iattr=0; iattr<nrrcvattrs; iattr++ )
	    newtrc->set( nrshotattrs+iattr, vbuf[2+iattr], 0 );

	newtrc->info().setPSFlds( rcvcoord, tmpltrc.info().coord, true );
	if ( SI().sampling(false).hsamp_.includes(newtrc->info().binid) )
	    datamgr_.add( newtrc );
	else
	{
	    delete newtrc;
	    nrrejected_++;
	}
    }

    return true;
}


int SeisImpBPSIF::fileEnded()
{
    if ( openNext() )
	return Executor::MoreToDo();

    datamgr_.endReached();
    endofinput_ = true;
    return writeData();
}


int SeisImpBPSIF::writeData()
{
    if ( !datamgr_.needWrite() )
	return datamgr_.isEmpty() ? Executor::Finished() : Executor::MoreToDo();

    if ( !datamgr_.writeGather() )
    {
	errmsg_ = datamgr_.errMsg();
	return Executor::ErrorOccurred();
    }

    return Executor::MoreToDo();
}
