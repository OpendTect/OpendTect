/*+
 * COPYRIGHT: (C) de Groot-Bril Earth Sciences B.V.
 * AUTHOR   : A.H. Bril
 * DATE     : Mar 2001
 * FUNCTION : CBVS I/O
-*/

static const char* rcsID = "$Id: cbvswriter.cc,v 1.13 2001-05-31 13:25:41 windev Exp $";

#include "cbvswriter.h"
#include "datainterp.h"
#include "strmoper.h"
#include "errh.h"
#include "binidselimpl.h"

const int CBVSIO::integersize = 4;
const int CBVSIO::version = 1;
const int CBVSIO::headstartbytes = 8 + 2 * CBVSIO::integersize;


CBVSWriter::CBVSWriter( ostream* s, const CBVSInfo& i,
			const CBVSInfo::ExplicitData* e )
	: strm_(*s)
	, expldat(e)
	, thrbytes_(0)
	, finishing_inline(false)
	, previnl(-999)
	, trcswritten(0)
	, nrtrcsperposn(i.nrtrcsperposn)
	, explinfo(i.explinfo)
	, survgeom(i.geom)
{
    init( i );
}


CBVSWriter::CBVSWriter( ostream* s, const CBVSWriter& cw, const CBVSInfo& ci )
	: strm_(*s)
	, expldat(cw.expldat)
	, thrbytes_(cw.thrbytes_)
	, finishing_inline(false)
	, previnl(-999)
	, trcswritten(0)
	, nrtrcsperposn(cw.nrtrcsperposn)
	, explinfo(cw.explinfo)
	, survgeom(ci.geom)
{
    init( ci );
}


void CBVSWriter::init( const CBVSInfo& i )
{
    if ( !strm_.good() )
	{ errmsg_ = "Cannot open file for write"; return; }
    if ( !survgeom.fullyrectandreg && !expldat )
	{ pErrMsg("Survey not rectangular but no explicit inl/crl info");
	  errmsg_ = "Internal error"; return; }

    if ( expldat && survgeom.fullyrectandreg
      && !explinfo.startpos && !explinfo.coord && !explinfo.offset
      && !explinfo.pick && !explinfo.refpos )
	expldat = 0;

    writeHdr( i ); if ( errmsg_ ) return;

    streampos datastart = strm_.tellp();
    strm_.seekp( 8 );
    int nrbytes = (int)datastart;
    strm_.write( (const char*)&nrbytes, integersize );
    strm_.seekp( datastart );
}


CBVSWriter::~CBVSWriter()
{
    close();
    delete &strm_;
}


#define mErrRet(s) { errmsg_ = s; return; }

void CBVSWriter::writeHdr( const CBVSInfo& info )
{
    unsigned char ucbuf[headstartbytes]; memset( ucbuf, 0, headstartbytes );
    ucbuf[0] = 'd'; ucbuf[1] = 'G'; ucbuf[2] = 'B';
    put_platform( ucbuf + 3 );
    ucbuf[4] = version;
    putExplicits( ucbuf + 5 );
    if ( !strm_.write((const char*)ucbuf,headstartbytes) )
	mErrRet("Cannot start writing to file")

    int sz = info.stdtext.size();
    strm_.write( (const char*)&sz, integersize );
    strm_.write( (const char*)info.stdtext, sz );

    writeComps( info );
    geomfo = strm_.tellp();
    writeGeom();
    strm_.write( (const char*)&info.seqnr, integersize );
    BufferString bs( info.usertext );
    int len = bs.size();
    while ( len % 4 )
	{ bs += " "; len = bs.size(); }
    strm_.write( (const char*)&len, integersize );
    strm_.write( (const char*)bs, len );

    if ( !strm_.good() ) mErrRet("Could not write complete header");
}


void CBVSWriter::putExplicits( unsigned char* ptr ) const
{
    *ptr = 0;
    if ( explinfo.startpos )	*ptr |= (unsigned char)1;
    if ( explinfo.coord )	*ptr |= (unsigned char)2;
    if ( explinfo.offset )	*ptr |= (unsigned char)4;
    if ( explinfo.pick )	*ptr |= (unsigned char)8;
    if ( explinfo.refpos )	*ptr |= (unsigned char)16;
}


void CBVSWriter::writeComps( const CBVSInfo& info )
{
    nrcomps_ = info.compinfo.size();
    strm_.write( (const char*)&nrcomps_, integersize );

    cnrbytes_ = new int [nrcomps_];

    for ( int icomp=0; icomp<nrcomps_; icomp++ )
    {
	BasicComponentInfo& cinf = *info.compinfo[icomp];
	int sz = cinf.name().size();
	strm_.write( (const char*)&sz, integersize );
	strm_.write( (const char*)cinf.name(), sz );
	unsigned short dcdump = cinf.datachar.dump();
	strm_.write( (const char*)&cinf.datatype, integersize );
	strm_.write( (const char*)&dcdump, sizeof(unsigned short) );
	dcdump = 0; // add space for compression type
	strm_.write( (const char*)&dcdump, sizeof(unsigned short) );
	strm_.write( (const char*)&cinf.sd.start, sizeof(float) );
	strm_.write( (const char*)&cinf.sd.step, sizeof(float) );
	strm_.write( (const char*)&cinf.nrsamples, integersize );
	float a = 0, b = 1;
	if ( cinf.scaler )
	    { a = cinf.scaler->constant; b = cinf.scaler->factor; }
	strm_.write( (const char*)&a, sizeof(float) );
	strm_.write( (const char*)&b, sizeof(float) );

	cnrbytes_[icomp] = cinf.nrsamples * cinf.datachar.nrBytes();
    }
}


void CBVSWriter::writeGeom()
{
    int irect = 0;
    if ( survgeom.fullyrectandreg )
    {
	irect = 1;
	nrxlines_ = (survgeom.stop.crl - survgeom.start.crl)
		  / survgeom.step.crl + 1;
    }
    strm_.write( (const char*)&irect, integersize );
    strm_.write( (const char*)&nrtrcsperposn, integersize );
    strm_.write( (const char*)&survgeom.start.inl, 2 * integersize );
    strm_.write( (const char*)&survgeom.stop.inl, 2 * integersize );
    strm_.write( (const char*)&survgeom.step.inl, 2 * integersize );
    strm_.write( (const char*)&survgeom.b2c.getTransform(true).a, 
		    3*sizeof(double) );
    strm_.write( (const char*)&survgeom.b2c.getTransform(false).a, 
		    3*sizeof(double) );
}


void CBVSWriter::newSeg()
{
    if ( !trcswritten ) previnl = curbinid_.inl;

    inldata += new CBVSInfo::SurvGeom::InlineInfo( curbinid_.inl );
    inldata[inldata.size()-1]->segments +=
	CBVSInfo::SurvGeom::InlineInfo::Segment(curbinid_.crl,curbinid_.crl,1);
}


void CBVSWriter::getBinID()
{
    if ( survgeom.fullyrectandreg || !expldat )
    {
	int posidx = trcswritten / nrtrcsperposn;
	curbinid_.inl = survgeom.start.inl
		   + survgeom.step.inl * (posidx / nrxlines_);
	curbinid_.crl = survgeom.start.crl
		   + survgeom.step.crl * (posidx % nrxlines_);
    }
    else if ( !(trcswritten % nrtrcsperposn) )
    {
	curbinid_ = expldat->binid;
	if ( !trcswritten || previnl != curbinid_.inl )
	    newSeg();
	else
	{
	    CBVSInfo::SurvGeom::InlineInfo& inlinf = *inldata[inldata.size()-1];
	    CBVSInfo::SurvGeom::InlineInfo::Segment& seg =
				inlinf.segments[inlinf.segments.size()-1];
	    if ( seg.stop == seg.start )
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
		    newSeg();
		else
		    seg.stop = curbinid_.crl;
	    }
	}
    }
}


int CBVSWriter::put( void** cdat )
{
    getBinID();
    if ( finishing_inline && previnl != curbinid_.inl )
    {
	// remove last segment added
	delete inldata[inldata.size()-1];
	inldata.remove( inldata.size()-1 );

	close();
	return 1;
    }

    if ( !writeExplicits() )
	{ errmsg_ = "Cannot write Trace header data"; return -1; }

    for ( int icomp=0; icomp<nrcomps_; icomp++ )
    {
	if ( !writeWithRetry(strm_,cdat[icomp],cnrbytes_[icomp],2,100) )
	    { errmsg_ = "Cannot write CBVS data"; return -1; }
    }

    if ( thrbytes_ && strm_.tellp() >= thrbytes_ )
	finishing_inline = true;

    previnl = curbinid_.inl;
    trcswritten++;
    return 0;
}


bool CBVSWriter::writeExplicits()
{
    if ( !expldat ) return true;

#define mDoWrExpl(memb)  \
    if ( explinfo.memb ) \
	strm_.write( (const char*)&expldat->memb, sizeof(expldat->memb) )

    mDoWrExpl(startpos);
    mDoWrExpl(coord);
    mDoWrExpl(offset);
    mDoWrExpl(pick);
    mDoWrExpl(refpos);

    return strm_.good();
}


void CBVSWriter::close()
{
    if ( strmclosed_ ) return;

    getRealGeometry();
    streampos kp = strm_.tellp();
    strm_.seekp( geomfo );
    writeGeom();
    strm_.seekp( kp );
    if ( !survgeom.fullyrectandreg && !writeTrailer() )
    {
	// damn! we were almost there!
	errmsg_ = "Could not write CBVS trailer";
	ErrMsg( errmsg_ );
    }
    
    strm_.flush();
    strmclosed_ = true;
}


void CBVSWriter::getRealGeometry()
{
    BinIDSampler bids;
    survgeom.fullyrectandreg = true;

    const int nrinl = inldata.size();
    for ( int iinl=0; iinl<nrinl; iinl++ )
    {
	CBVSInfo::SurvGeom::InlineInfo& inlinf = *inldata[iinl];
	if ( iinl == 0 )
	{
	    previnl = bids.start.inl = bids.stop.inl = inlinf.inl;
	    bids.start.crl = inlinf.segments[0].start;
	    bids.stop.crl = inlinf.segments[0].stop;
	    bids.step.crl = inlinf.segments[0].step;
	}
	else if ( iinl == 1 )
	    bids.step.inl = inlinf.inl - previnl;
	else if ( inlinf.inl - previnl != bids.step.inl )
	    survgeom.fullyrectandreg = false;
	previnl = inlinf.inl;

	const int nrcrl = inlinf.segments.size();
	if ( nrcrl != 1 )
	    survgeom.fullyrectandreg = false;

	for ( int icrl=0; icrl<nrcrl; icrl++ )
	{
	    CBVSInfo::SurvGeom::InlineInfo::Segment& seg =inlinf.segments[icrl];
	    if ( seg.step != bids.step.crl )
		survgeom.fullyrectandreg = false;
	    else if ( iinl )
	    {
		Interval<int> intv( seg ); intv.sort();
		if ( intv.start != bids.start.crl || intv.stop != bids.stop.crl)
		    survgeom.fullyrectandreg = false;
	    }
	    bids.include( BinID(inlinf.inl,seg.start) );
	    bids.include( BinID(inlinf.inl,seg.stop) );
	}
    }

    if ( survgeom.fullyrectandreg )
	deepErase( survgeom.inldata );

    survgeom.start = bids.start;
    survgeom.stop = bids.stop;
    survgeom.step = bids.step;
}


bool CBVSWriter::writeTrailer()
{
    const int nrinl = inldata.size();
    streampos trailerstart = strm_.tellp();
    strm_.write( (const char*)&nrinl, integersize );
    for ( int iinl=0; iinl<nrinl; iinl++ )
    {
	CBVSInfo::SurvGeom::InlineInfo& inlinf = *inldata[iinl];
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
