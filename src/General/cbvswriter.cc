/*+
 * COPYRIGHT: (C) de Groot-Bril Earth Sciences B.V.
 * AUTHOR   : A.H. Bril
 * DATE     : Mar 2001
 * FUNCTION : CBVS I/O
-*/

static const char* rcsID = "$Id: cbvswriter.cc,v 1.3 2001-03-30 08:52:58 bert Exp $";

#include "cbvswriter.h"
#include "datainterp.h"
#include "strmoper.h"
#include "errh.h"

#define mIntSz 4
#define mVersion 1


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
	, cnrbytes(0)
	, nrxlines(1)
	, nrtrcsperposn(i.nrtrcsperposn)
	, explinfo(i.explinfo)
	, survgeom(i.geom)
{
    if ( !strm_.good() ) { errmsg_ = "Cannot open file for write"; return; }

    writeHdr( i );
    streampos datastart = strm_.tellp();
    strm_.seekp( 8 );
    int nrbytes = (int)datastart;
    strm_.write( &nrbytes, mIntSz );
    strm_.seekp( datastart );
}


CBVSWriter::~CBVSWriter()
{
    close();
    delete &strm_;
    delete [] cnrbytes;
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


#define mErrRet(s) { errmsg_ = s; return; }

void CBVSWriter::writeHdr( const CBVSInfo& info )
{
    const int headstartbytes = 8 + 2*mIntSz;
    unsigned char ucbuf[headstartbytes]; memset( ucbuf, 0, headstartbytes );
    ucbuf[0] = 'd'; ucbuf[1] = 'G'; ucbuf[2] = 'B';
    put_platform( ucbuf+3 );
    ucbuf[4] = mVersion;
    putExplicits( ucbuf + 5 );
    if ( !strm_.write(ucbuf,headstartbytes) )
	mErrRet("Cannot start writing to file")

    writeComps( info );
    geomfo = strm_.tellp();
    writeGeom();
    strm_.write( &info.seqnr, mIntSz );
    int len = info.usertext.size();
    strm_.write( &len, mIntSz );
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
    strm_.write( &sz, mIntSz );
    strm_.write( (const char*)info.stdtext, sz );

    nrcomps = info.compinfo.size();
    strm_.write( &nrcomps, mIntSz );

    cnrbytes = new int [nrcomps];

    for ( int icomp=0; icomp<nrcomps; icomp++ )
    {
	BasicComponentInfo& cinf = *info.compinfo[icomp];
	sz = cinf.name().size();
	strm_.write( &sz, mIntSz );
	strm_.write( (const char*)cinf.name(), sz );
	unsigned short dcdump = cinf.datachar.dump();
	strm_.write( &cinf.datatype, mIntSz );
	strm_.write( &dcdump, sizeof(unsigned short) );
	strm_.write( &cinf.sd.start, sizeof(float) );
	strm_.write( &cinf.sd.step, sizeof(float) );
	strm_.write( &cinf.nrsamples, mIntSz );

	cnrbytes[icomp] = cinf.nrsamples;
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
    strm_.write( &irect, mIntSz );
    strm_.write( &nrtrcsperposn, mIntSz );
    strm_.write( &survgeom.start.inl, 2 * mIntSz );
    strm_.write( &survgeom.stop.inl, 2 * mIntSz );
    strm_.write( &survgeom.step.inl, 2 * mIntSz );
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

    for ( int icomp=0; icomp<nrcomps; icomp++ )
    {
	if ( !writeWithRetry(strm_,cdat[icomp],cnrbytes[icomp],1,50) )
	    { errmsg_ = "Cannot write CBVS data"; return -1; }
    }

    if ( strm_.tellp() >= thrbytes_ )
	finishing_inline = true;

    previnl = curbid.inl;
    trcswritten++;
    return 0;
}
