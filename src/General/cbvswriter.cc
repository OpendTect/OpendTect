/*+
 * COPYRIGHT: (C) de Groot-Bril Earth Sciences B.V.
 * AUTHOR   : A.H. Bril
 * DATE     : Mar 2001
 * FUNCTION : CBVS I/O
-*/

static const char* rcsID = "$Id: cbvswriter.cc,v 1.5 2001-04-04 11:12:12 bert Exp $";

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
	, errmsg_(0)
	, strmclosed(false)
	, finishing_inline(false)
	, previnl(-999)
	, trcswritten(0)
	, nrcomps(0)
	, nrxlines(1)
	, nrtrcsperposn(i.nrtrcsperposn)
	, explinfo(i.explinfo)
	, survgeom(i.geom)
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

    writeHdr( i ); if ( *(const char*)errmsg_ ) return;

    streampos datastart = strm_.tellp();
    strm_.seekp( 8 );
    int nrbytes = (int)datastart;
    strm_.write( &nrbytes, integersize );
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
    put_platform( ucbuf+3 );
    ucbuf[4] = version;
    putExplicits( ucbuf + 5 );
    if ( !strm_.write(ucbuf,headstartbytes) )
	mErrRet("Cannot start writing to file")

    writeComps( info );
    geomfo = strm_.tellp();
    writeGeom();
    strm_.write( &info.seqnr, integersize );
    int len = info.usertext.size();
    strm_.write( &len, integersize );
    strm_.write( (const char*)info.usertext, len );

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
    int sz = info.stdtext.size();
    strm_.write( &sz, integersize );
    strm_.write( (const char*)info.stdtext, sz );

    nrcomps = info.compinfo.size();
    strm_.write( &nrcomps, integersize );

    cnrbytes_ = new int [nrcomps];

    for ( int icomp=0; icomp<nrcomps; icomp++ )
    {
	CBVSComponentInfo& cinf = *info.compinfo[icomp];
	sz = cinf.name().size();
	strm_.write( &sz, integersize );
	strm_.write( (const char*)cinf.name(), sz );
	unsigned short dcdump = cinf.datachar.dump();
	strm_.write( &cinf.datatype, integersize );
	strm_.write( &dcdump, sizeof(unsigned short) );
	strm_.write( &cinf.sd.start, sizeof(float) );
	strm_.write( &cinf.sd.step, sizeof(float) );
	strm_.write( &cinf.nrsamples, integersize );
	float a = 0, b = 1;
	if ( cinf.scaler )
	    { a = cinf.scaler->constant; b = cinf.scaler->factor; }
	strm_.write( &a, sizeof(float) );
	strm_.write( &b, sizeof(float) );

	cnrbytes_[icomp] = cinf.nrsamples * cinf.datachar.nrBytes();
    }
}


void CBVSWriter::writeGeom()
{
    int irect = 0;
    if ( survgeom.fullyrectandreg )
    {
	irect = 1;
	nrxlines = (survgeom.stop.crl - survgeom.start.crl)
		 / survgeom.step.crl + 1;
    }
    strm_.write( &irect, integersize );
    strm_.write( &nrtrcsperposn, integersize );
    strm_.write( &survgeom.start.inl, 2 * integersize );
    strm_.write( &survgeom.stop.inl, 2 * integersize );
    strm_.write( &survgeom.step.inl, 2 * integersize );
    strm_.write( &survgeom.b2c.getTransform(true).a, 3*sizeof(double) );
    strm_.write( &survgeom.b2c.getTransform(false).a, 3*sizeof(double) );
}


void CBVSWriter::newSeg()
{
    if ( !trcswritten ) previnl = curbid.inl;

    inldata += new CBVSInfo::SurvGeom::InlineInfo( curbid.inl );
    inldata[inldata.size()-1]->segments +=
	    CBVSInfo::SurvGeom::InlineInfo::Segment(curbid.crl,curbid.crl,1);
}


void CBVSWriter::getBinID()
{
    if ( survgeom.fullyrectandreg || !expldat )
    {
	int posidx = trcswritten / nrtrcsperposn;
	curbid.inl = survgeom.start.inl
		   + survgeom.step.inl * (posidx / nrxlines);
	curbid.crl = survgeom.start.crl
		   + survgeom.step.crl * (posidx % nrxlines);
    }
    else if ( !(trcswritten % nrtrcsperposn) )
    {
	curbid = expldat->binid;
	if ( !trcswritten || previnl != curbid.inl )
	    newSeg();
	else
	{
	    CBVSInfo::SurvGeom::InlineInfo& inlinf = *inldata[inldata.size()-1];
	    CBVSInfo::SurvGeom::InlineInfo::Segment& seg =
				inlinf.segments[inlinf.segments.size()-1];
	    if ( seg.stop == seg.start )
	    {
		if ( seg.stop != curbid.crl )
		{
		    seg.stop = curbid.crl;
		    seg.step = seg.stop - seg.start;
		}
	    }
	    else
	    {
		if ( curbid.crl != seg.stop + seg.step )
		    newSeg();
		else
		    seg.stop = curbid.crl;
	    }
	}
    }
}


int CBVSWriter::put( void** cdat )
{
    getBinID();
    if ( finishing_inline && previnl != curbid.inl )
    {
	close();
	return 1;
    }

    if ( !writeExplicits() )
	{ errmsg_ = "Cannot write Trace header data"; return -1; }

    for ( int icomp=0; icomp<nrcomps; icomp++ )
    {
	if ( !writeWithRetry(strm_,cdat[icomp],cnrbytes_[icomp],2,100) )
	    { errmsg_ = "Cannot write CBVS data"; return -1; }
    }

    if ( strm_.tellp() >= thrbytes_ )
	finishing_inline = true;

    previnl = curbid.inl;
    trcswritten++;
    return 0;
}


bool CBVSWriter::writeExplicits()
{
    if ( !expldat ) return true;

#define mDoWrExpl(memb) \
    if ( explinfo.memb ) strm_.write( &expldat->memb, sizeof(expldat->memb) )

    mDoWrExpl(startpos);
    mDoWrExpl(coord);
    mDoWrExpl(offset);
    mDoWrExpl(pick);
    mDoWrExpl(refpos);

    return strm_.good();
}


void CBVSWriter::close()
{
    if ( strmclosed ) return;

    getRealGeometry();
    if ( survgeom.fullyrectandreg )
    {
	streampos kp = strm_.tellp();
	strm_.seekp( geomfo );
	writeGeom();
	strm_.seekp( kp );
    }
    else if ( !writeTrailer() )
    {
	// damn! we were almost there!
	errmsg_ = "Could not write CBVS trailer";
	ErrMsg( errmsg_ );
    }
    
    strm_.flush();
    strmclosed = true;
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
	    bids.start.crl = bids.stop.crl = inlinf.segments[0].start;
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
	    bids.include( BinID(seg.start,seg.stop) );
	}
    }

    survgeom.start = bids.start;
    survgeom.stop = bids.stop;
    survgeom.step = bids.step;
}


bool CBVSWriter::writeTrailer()
{
    const int nrinl = inldata.size();
    streampos trailerstart = strm_.tellp();
    strm_.write( &nrinl, integersize );
    for ( int iinl=0; iinl<nrinl; iinl++ )
    {
	CBVSInfo::SurvGeom::InlineInfo& inlinf = *inldata[iinl];
	strm_.write( &inlinf.inl, integersize );
	const int nrcrl = inlinf.segments.size();
	strm_.write( &nrcrl, integersize );

	for ( int icrl=0; icrl<nrcrl; icrl++ )
	{
	    CBVSInfo::SurvGeom::InlineInfo::Segment& seg =inlinf.segments[icrl];
	    strm_.write( &seg.start, integersize );
	    strm_.write( &seg.stop, integersize );
	    strm_.write( &seg.step, integersize );
	}
	if ( !strm_.good() ) return false;
    }

    int bytediff = (int)(strm_.tellp() - trailerstart);
    strm_.write( &bytediff, integersize );
    unsigned char buf[4];
    put_platform( buf );
    buf[1] = 'B';
    buf[2] = 'G';
    buf[3] = 'd';
    strm_.write( buf, integersize );

    return true;
}
