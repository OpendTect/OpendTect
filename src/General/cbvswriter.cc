/*+
 * COPYRIGHT: (C) de Groot-Bril Earth Sciences B.V.
 * AUTHOR   : A.H. Bril
 * DATE     : Mar 2001
 * FUNCTION : CBVS I/O
-*/

static const char* rcsID = "$Id: cbvswriter.cc,v 1.2 2001-03-21 17:32:27 bert Exp $";

#include "cbvswriter.h"
#include "datainterp.h"

#define mIntSz 4
#define mVersion 1


CBVSWriter::CBVSWriter( ostream* s, const CBVSInfo& i,
			const CBVSInfo::ExplicitData* e )
	: strm_(*s)
	, expldat(e)
	, thrbytes_(0)
	, errmsg_(0)
	, strmclosed(false)
{
    if ( !strm_.good() ) { errmsg_ = "Cannot open file for write"; return; }

    writeHdr( i );
}


CBVSWriter::~CBVSWriter()
{
    close();
    delete &strm_;
}


void CBVSWriter::close()
{
    strmclosed = true;
    strm_.flush();
}


#define mErrRet(s) { errmsg_ = s; return; }

void CBVSWriter::writeHdr( const CBVSInfo& info )
{
    unsigned char ucbuf[8]; memset( ucbuf, 0, 8 );
    ucbuf[0] = 'd'; ucbuf[1] = 'G'; ucbuf[2] = 'B';
    put_platform( ucbuf+3 );
    ucbuf[4] = mVersion;
    putExplicits( info.explinfo, ucbuf + 5 );
    if ( !strm_.write(ucbuf,8) ) mErrRet("Cannot start writing to file")

    writeComps( info );
    writeGeom( info );
    strm_.write( &info.seqnr, mIntSz );
    int len = info.usertext.size();
    strm_.write( &len, mIntSz );
    strm_.write( (const char*)info.usertext, len );

    if ( !strm_.good() ) mErrRet("Could not write complete header");
}


void CBVSWriter::putExplicits( const CBVSInfo::ExplicitInfo& explinfo,
				unsigned char* ptr )
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

    const int nrcomps = info.compinfo.size();
    strm_.write( &nrcomps, mIntSz );

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
    }
}


void CBVSWriter::writeGeom( const CBVSInfo& info )
{
    geomfo = strm_.tellp();
    int irect = info.geom.fullyrectangular ? 1 : 0;
    strm_.write( &irect, mIntSz );
    strm_.write( &info.geom.start.inl, 2 * mIntSz );
    strm_.write( &info.geom.stop.inl, 2 * mIntSz );
    strm_.write( &info.geom.step.inl, 2 * mIntSz );
    strm_.write( &info.geom.b2c.getTransform(true).a, 3*sizeof(double) );
    strm_.write( &info.geom.b2c.getTransform(false).a, 3*sizeof(double) );
}


int CBVSWriter::put( void** cdat )
{
    //for ( int icomp=0; icomp<nrcomp; icomp++ )
    return 0;
}
