/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : A.H. Bril
 * DATE     : Mar 2001
 * FUNCTION : CBVS I/O
-*/

static const char* rcsID = "$Id: cbvswriter.cc,v 1.36 2003-11-07 12:21:57 bert Exp $";

#include "cbvswriter.h"
#include "datainterp.h"
#include "strmoper.h"
#include "errh.h"
#include "binidselimpl.h"
#include "survinfo.h"

const int CBVSIO::integersize = 4;
const int CBVSIO::version = 1;
const int CBVSIO::headstartbytes = 8 + 2 * CBVSIO::integersize;


CBVSWriter::CBVSWriter( ostream* s, const CBVSInfo& i, const PosAuxInfo* e )
	: strm_(*s)
	, auxinfo_(e)
	, thrbytes_(0)
	, file_lastinl_(false)
	, prevbinid_(-999,-999)
	, trcswritten_(0)
	, nrtrcsperposn_(i.nrtrcsperposn)
	, nrtrcsperposn_status_(2)
	, checknrtrcsperposn_(0)
	, auxinfosel_(i.auxinfosel)
	, survgeom_(i.geom)
	, auxnrbytes_(0)
	, input_rectnreg_(false)
    	, forcedlinestep_(0,0)
{
    init( i );
}


CBVSWriter::CBVSWriter( ostream* s, const CBVSWriter& cw, const CBVSInfo& ci )
	: strm_(*s)
	, auxinfo_(cw.auxinfo_)
	, thrbytes_(cw.thrbytes_)
	, file_lastinl_(false)
	, prevbinid_(-999,-999)
	, trcswritten_(0)
	, nrtrcsperposn_(cw.nrtrcsperposn_)
	, nrtrcsperposn_status_(cw.nrtrcsperposn_status_)
	, auxinfosel_(cw.auxinfosel_)
	, survgeom_(ci.geom)
	, auxnrbytes_(0)
    	, forcedlinestep_(cw.forcedlinestep_)
{
    init( ci );
}


void CBVSWriter::init( const CBVSInfo& i )
{
    nrbytespersample_ = 0;

    if ( !strm_.good() )
	{ errmsg_ = "Cannot open file for write"; return; }
    if ( !survgeom_.fullyrectandreg && !auxinfo_ )
	{ pErrMsg("Survey not rectangular but no explicit inl/crl info");
	  errmsg_ = "Internal error"; return; }

    if ( auxinfo_ && survgeom_.fullyrectandreg
      && !auxinfosel_.startpos && !auxinfosel_.coord
      && !auxinfosel_.offset && !auxinfosel_.azimuth
      && !auxinfosel_.pick && !auxinfosel_.refpos )
	auxinfo_ = 0;

    writeHdr( i ); if ( errmsg_ ) return;

    input_rectnreg_ = survgeom_.fullyrectandreg;

    streampos cursp = strm_.tellp();
    strm_.seekp( 8 );
    int nrbytes = (int)cursp;
    strm_.write( (const char*)&nrbytes, integersize );
    strm_.seekp( cursp );
}


CBVSWriter::~CBVSWriter()
{
    close();
    if ( &strm_ != &cout ) delete &strm_;
    delete [] nrbytespersample_;
}


#define mErrRet(s) { errmsg_ = s; return; }

void CBVSWriter::writeHdr( const CBVSInfo& info )
{
    unsigned char ucbuf[headstartbytes]; memset( ucbuf, 0, headstartbytes );
    ucbuf[0] = 'd'; ucbuf[1] = 'G'; ucbuf[2] = 'B';
    put_platform( ucbuf + 3 );
    ucbuf[4] = version;
    putAuxInfoSel( ucbuf + 5 );
    if ( !strm_.write((const char*)ucbuf,headstartbytes) )
	mErrRet("Cannot start writing to file")

    int sz = info.stdtext.size();
    strm_.write( (const char*)&sz, integersize );
    strm_.write( (const char*)info.stdtext, sz );

    writeComps( info );
    geomsp_ = strm_.tellp();
    writeGeom();
    strm_.write( (const char*)&info.seqnr, integersize );
    BufferString bs( info.usertext );

    streampos fo = strm_.tellp();
    int len = bs.size();
    int endpos = (int)fo + len + 4;
    while ( endpos % 4 )
	{ bs += " "; len++; endpos++; }
    strm_.write( (const char*)&len, integersize );
    strm_.write( (const char*)bs, len );

    if ( !strm_.good() ) mErrRet("Could not write complete header");
}


void CBVSWriter::putAuxInfoSel( unsigned char* ptr ) const
{
    *ptr = 0;

#define mDoMemb(memb,n) \
    if ( auxinfosel_.memb ) \
    { \
	*ptr |= (unsigned char)n; \
	const_cast<CBVSWriter*>(this)->auxnrbytes_ += sizeof(auxinfo_->memb); \
    }

    mDoMemb(startpos,1)
    mDoMemb(coord,2)
    mDoMemb(offset,4)
    mDoMemb(pick,8)
    mDoMemb(refpos,16)
    mDoMemb(azimuth,32)
}


void CBVSWriter::writeComps( const CBVSInfo& info )
{
    nrcomps_ = info.compinfo.size();
    strm_.write( (const char*)&nrcomps_, integersize );

    cnrbytes_ = new int [nrcomps_];
    nrbytespersample_ = new int [nrcomps_];

    unsigned char dcdump[4];
    dcdump[2] = dcdump[3] = 0; // added space for compression type

    for ( int icomp=0; icomp<nrcomps_; icomp++ )
    {
	BasicComponentInfo& cinf = *info.compinfo[icomp];
	int sz = cinf.name().size();
	strm_.write( (const char*)&sz, integersize );
	strm_.write( (const char*)cinf.name(), sz );
	strm_.write( (const char*)&cinf.datatype, integersize );
       	cinf.datachar.dump( dcdump[0], dcdump[1] );
	strm_.write( (const char*)dcdump, 4 );
	strm_.write( (const char*)&cinf.sd.start, sizeof(float) );
	strm_.write( (const char*)&cinf.sd.step, sizeof(float) );
	strm_.write( (const char*)&cinf.nrsamples, integersize );
	float a = 0, b = 1;
	if ( cinf.scaler )
	    { a = cinf.scaler->constant; b = cinf.scaler->factor; }
	strm_.write( (const char*)&a, sizeof(float) );
	strm_.write( (const char*)&b, sizeof(float) );

	nrbytespersample_[icomp] = cinf.datachar.nrBytes();
	cnrbytes_[icomp] = cinf.nrsamples * nrbytespersample_[icomp];
    }
}


void CBVSWriter::writeGeom()
{
    int irect = 0;
    if ( survgeom_.fullyrectandreg )
    {
	irect = 1;
	nrxlines_ = (survgeom_.stop.crl - survgeom_.start.crl)
		  / survgeom_.step.crl + 1;
    }
    strm_.write( (const char*)&irect, integersize );
    strm_.write( (const char*)&nrtrcsperposn_, integersize );
    strm_.write( (const char*)&survgeom_.start.inl, 2 * integersize );
    strm_.write( (const char*)&survgeom_.stop.inl, 2 * integersize );
    strm_.write( (const char*)&survgeom_.step.inl, 2 * integersize );
    strm_.write( (const char*)&survgeom_.b2c.getTransform(true).a, 
		    3*sizeof(double) );
    strm_.write( (const char*)&survgeom_.b2c.getTransform(false).a, 
		    3*sizeof(double) );
}


void CBVSWriter::newSeg( bool newinl )
{
    const bool goodgeom = nrtrcsperposn_status_ == 0 && nrtrcsperposn_ > 0;
    if ( !goodgeom && !newinl )
    {
	inldata_[inldata_.size()-1]->segments[0].stop = curbinid_.crl;
	return;
    }

    if ( !trcswritten_ ) prevbinid_ = curbinid_;

    int newstep = forcedlinestep_.crl ? forcedlinestep_.crl : SI().crlStep();
    if ( newinl )
    {
	if ( goodgeom && inldata_.size() )
	    newstep = inldata_[inldata_.size()-1]->segments[0].step;
	inldata_ += new CBVSInfo::SurvGeom::InlineInfo( curbinid_.inl );
    }
    else if ( goodgeom )
	newstep = inldata_[inldata_.size()-1]->segments[0].step;

    inldata_[inldata_.size()-1]->segments +=
	CBVSInfo::SurvGeom::InlineInfo::Segment(curbinid_.crl,curbinid_.crl,
						newstep);
}


void CBVSWriter::getBinID()
{
    const int nrtrcpp = nrtrcsperposn_ < 2 ? 1 : nrtrcsperposn_;
    if ( input_rectnreg_ || !auxinfo_ )
    {
	int posidx = trcswritten_ / nrtrcpp;
	curbinid_.inl = survgeom_.start.inl
		   + survgeom_.step.inl * (posidx / nrxlines_);
	curbinid_.crl = survgeom_.start.crl
		   + survgeom_.step.crl * (posidx % nrxlines_);
    }
    else if ( !(trcswritten_ % nrtrcpp) )
    {
	curbinid_ = auxinfo_->binid;
	if ( !trcswritten_ || prevbinid_.inl != curbinid_.inl )
	    newSeg( true );
	else
	{
	    CBVSInfo::SurvGeom::InlineInfo& inlinf
			= *inldata_[inldata_.size()-1];
	    CBVSInfo::SurvGeom::InlineInfo::Segment& seg =
				inlinf.segments[inlinf.segments.size()-1];
	    if ( !forcedlinestep_.crl && seg.stop == seg.start )
	    {
		if ( seg.stop != curbinid_.crl )
		{
		    seg.stop = curbinid_.crl;
		    seg.step = seg.stop - seg.start;
		}
	    }
	    else
	    {
		if ( curbinid_.crl != seg.stop + seg.step )
		    newSeg( false );
		else
		    seg.stop = curbinid_.crl;
	    }
	}
    }
}


int CBVSWriter::put( void** cdat, int offs )
{
    getBinID();
    if ( prevbinid_.inl != curbinid_.inl )
    {
	// getBinID() has added a new segment, so remove it from list ...
	CBVSInfo::SurvGeom::InlineInfo* newinldat = inldata_[inldata_.size()-1];
	inldata_.remove( inldata_.size()-1 );
	if ( file_lastinl_ )
	{
	    delete newinldat;
	    close();
	    return errmsg_ ? -1 : 1;
	}

	doClose( false );
	inldata_ += newinldat;

	if ( errmsg_ ) return -1;
    }

    if ( !writeAuxInfo() )
	{ errmsg_ = "Cannot write Trace header data"; return -1; }

    for ( int icomp=0; icomp<nrcomps_; icomp++ )
    {
	const char* ptr = ((const char*)cdat[icomp])
	    		+ offs * nrbytespersample_[icomp];
	if ( !writeWithRetry(strm_,ptr,cnrbytes_[icomp],2,100) )
	    { errmsg_ = "Cannot write CBVS data"; return -1; }
    }

    if ( thrbytes_ && strm_.tellp() >= thrbytes_ )
	file_lastinl_ = true;

    if ( nrtrcsperposn_status_ && trcswritten_ )
    {
	if ( trcswritten_ == 1 )
	    nrtrcsperposn_ = 1;

	if ( nrtrcsperposn_status_ == 2 )
	{
	    if ( prevbinid_ == curbinid_ )
		nrtrcsperposn_++;
	    else
	    {
		nrtrcsperposn_status_ = 1;
		checknrtrcsperposn_ = 1;
	    }
	}
	else
	{
	    if ( prevbinid_ == curbinid_ )
		checknrtrcsperposn_++;
	    else
	    {
		nrtrcsperposn_status_ = 0;
		if ( checknrtrcsperposn_ != nrtrcsperposn_ )
		{
		    input_rectnreg_ = true;
		    nrtrcsperposn_ = -1;
		}
	    }
	}
    }
    prevbinid_ = curbinid_;
    trcswritten_++;
    return 0;
}


bool CBVSWriter::writeAuxInfo()
{
    if ( auxinfo_ && auxnrbytes_ )
    {
#define mDoWrAI(memb)  \
    if ( auxinfosel_.memb ) \
	strm_.write( (const char*)&auxinfo_->memb, sizeof(auxinfo_->memb) );

	mDoWrAI(startpos)
	mDoWrAI(coord)
	mDoWrAI(offset)
	mDoWrAI(pick)
	mDoWrAI(refpos)
	mDoWrAI(azimuth)
    }

    return strm_.good();
}


void CBVSWriter::doClose( bool islast )
{
    if ( strmclosed_ ) return;

    getRealGeometry();
    streampos kp = strm_.tellp();
    strm_.seekp( geomsp_ );
    writeGeom();
    strm_.seekp( kp );

    if ( !survgeom_.fullyrectandreg && !writeTrailer() )
    {
	// damn! we were almost there!
	errmsg_ = "Could not write CBVS trailer";
	ErrMsg( errmsg_ );
    }
    
    strm_.flush();
    if ( islast )
	strmclosed_ = true;
    else
	strm_.seekp( kp );
}


void CBVSWriter::getRealGeometry()
{
    BinIDSampler bids;
    survgeom_.fullyrectandreg = true;

    const int nrinl = inldata_.size();
    bids.step.inl = 0;
    for ( int iinl=0; iinl<nrinl; iinl++ )
    {
	CBVSInfo::SurvGeom::InlineInfo& inlinf = *inldata_[iinl];
	int curinlstep = inlinf.inl - prevbinid_.inl;
	if ( iinl == 0 )
	{
	    prevbinid_.inl = bids.start.inl = bids.stop.inl = inlinf.inl;
	    bids.start.crl = inlinf.segments[0].start;
	    bids.stop.crl = inlinf.segments[0].stop;
	    bids.step.crl = inlinf.segments[0].step;
	}
	else if ( iinl == 1 || !bids.step.inl )
	    bids.step.inl = curinlstep;
	else if ( curinlstep != bids.step.inl )
	{
	    survgeom_.fullyrectandreg = false;
	    if ( curinlstep && curinlstep < bids.step.inl )
		bids.step.inl = curinlstep;
	}
	prevbinid_.inl = inlinf.inl;

	const int nrseg = inlinf.segments.size();
	if ( nrseg != 1 )
	    survgeom_.fullyrectandreg = false;

	for ( int iseg=0; iseg<nrseg; iseg++ )
	{
	    CBVSInfo::SurvGeom::InlineInfo::Segment& seg =inlinf.segments[iseg];
	    if ( !seg.step )
		seg.step = bids.step.crl ? bids.step.crl : SI().crlStep();
	    else if ( seg.step != bids.step.crl )
	    {
		survgeom_.fullyrectandreg = false;
		if ( seg.step < bids.step.crl )
		    bids.step.crl = seg.step;
	    }
	    else if ( iinl )
	    {
		Interval<int> intv( seg ); intv.sort();
		if ( intv.start != bids.start.crl || intv.stop != bids.stop.crl)
		    survgeom_.fullyrectandreg = false;
	    }
	    bids.include( BinID(inlinf.inl,seg.start) );
	    bids.include( BinID(inlinf.inl,seg.stop) );
	}
    }
    if ( !bids.step.inl )
	bids.step.inl = SI().inlStep();
    if ( !bids.step.crl )
	bids.step.crl = SI().crlStep();

    if ( survgeom_.fullyrectandreg )
	deepErase( survgeom_.inldata );
    else if ( forcedlinestep_.inl )
	bids.step.inl = forcedlinestep_.inl;

    survgeom_.start = bids.start;
    survgeom_.stop = bids.stop;
    survgeom_.step = bids.step;
}


bool CBVSWriter::writeTrailer()
{
    const int nrinl = inldata_.size();
    streampos trailerstart = strm_.tellp();
    strm_.write( (const char*)&nrinl, integersize );
    for ( int iinl=0; iinl<nrinl; iinl++ )
    {
	CBVSInfo::SurvGeom::InlineInfo& inlinf = *inldata_[iinl];
	strm_.write( (const char*)&inlinf.inl, integersize );
	const int nrcrl = inlinf.segments.size();
	strm_.write( (const char*)&nrcrl, integersize );

	for ( int icrl=0; icrl<nrcrl; icrl++ )
	{
	    CBVSInfo::SurvGeom::InlineInfo::Segment& seg =inlinf.segments[icrl];
	    strm_.write( (const char*)&seg.start, integersize );
	    strm_.write( (const char*)&seg.stop, integersize );
	    strm_.write( (const char*)&seg.step, integersize );
	}
	if ( !strm_.good() ) return false;
    }

    int bytediff = (int)(strm_.tellp() - trailerstart);
    strm_.write( (const char*)&bytediff, integersize );
    unsigned char buf[4];
    put_platform( buf );
    buf[1] = 'B';
    buf[2] = 'G';
    buf[3] = 'd';
    strm_.write( (const char*)buf, integersize );

    return true;
}
