/*+
 * COPYRIGHT: (C) de Groot-Bril Earth Sciences B.V.
 * AUTHOR   : A.H. Bril
 * DATE     : Mar 2001
 * FUNCTION : CBVS I/O
-*/

static const char* rcsID = "$Id: cbvsreader.cc,v 1.1 2001-03-19 10:20:01 bert Exp $";

#include "cbvsreader.h"
#include "datainterp.h"

#define mIntSz 4


CBVSReader::CBVSReader( istream* s )
	: strm_(*s)
	, datastartfo(readInfo())
{
    lastposfo = datastartfo;
}


#define mErrRet(s) { errmsg_ = s; return 0; }

streampos CBVSReader::readInfo()
{
    info_.clean();
    errmsg_ = check( strm_ );
    if ( errmsg_ ) return 0;

    strm_.seekg( 3, ios::beg );
    unsigned char buf[5 + mIntSz];
    strm_.read( buf, 5 + mIntSz );

    DataCharacteristics dc;
    dc.littleendian = buf[0] != 0;
    DataInterpreter<float> finterp( dc );
    dc.setNrBytes( BinDataDesc::N8 );
    DataInterpreter<double> dinterp( dc );
    dc.BinDataDesc::set( true, true, BinDataDesc::N4 );
    DataInterpreter<int> iinterp( dc );

    // const int version = (int)buf[1];
    getExplicits( buf+2 );

    int nrcomp = iinterp.get( buf+5, 0 );
    if ( nrcomp < 1 )	mErrRet("Corrupt CBVS format")
    if ( !readComps( nrcomp, iinterp, finterp ) ) return 0;

    strm_.read( buf, mIntSz );
    info_.nrtrcsperposn = iinterp.get( buf, 0 );
    if ( !readGeom( iinterp, dinterp ) ) return 0;

    strm_.read( buf, 8 );
    info_.seqnr = iinterp.get( buf, 0 );
    int nrchar = iinterp.get( buf, 1 );
    if ( nrchar > 0 )
    {
	char usrtxt[nrchar+1];
	strm_.read( usrtxt, nrchar );
	usrtxt[nrchar] = '\0';
	info_.usertext = usrtxt;
    }

    const streampos dfo = strm_.tellg();
    return info_.geom.fullyrectangular || readTrailer(iinterp) ? dfo : 0;
}


#undef mErrRet
#define mErrRet(s) { strm.seekg( 0, ios::beg ); return s; }

const char* CBVSReader::check( istream& strm )
{
    if ( strm.bad() ) return "Input stream cannot be used";
    if ( strm.fail() ) strm.clear();

    strm.seekg( 0, ios::beg );
    char buf[4]; memset( buf, 0, 4 );
    strm.read( buf, 3 );
    if ( !strm.good() ) mErrRet("Input stream cannot be used")
    if ( strcmp(buf,"dGB") ) mErrRet("File is not in CBVS format")

    unsigned char plf; strm.read( &plf, 1 );
    if ( plf > 2 ) mErrRet("File is not in CBVS format")

    strm.seekg( 0, ios::beg );
    return 0;
}


void CBVSReader::getExplicits( const unsigned char* ptr )
{
    info_.explinfo.startpos =	*ptr & (unsigned char)1;
    info_.explinfo.coord =	*ptr & (unsigned char)2;
    info_.explinfo.offset =	*ptr & (unsigned char)4;
    info_.explinfo.pick =	*ptr & (unsigned char)8;
    info_.explinfo.refpos =	*ptr & (unsigned char)16;
}


bool CBVSReader::readComps( int nrcomp, const DataInterpreter<int>& iinterp,
			    const DataInterpreter<float>& finterp )
{
    return true;
}


bool CBVSReader::readGeom( const DataInterpreter<int>& iinterp,
			   const DataInterpreter<double>& dinterp )
{
    return true;
}


#undef mErrRet
#define mErrRet(s) { strm_.seekg( 0, ios::beg ); return s; }

bool CBVSReader::readTrailer( const DataInterpreter<int>& iinterp )
{
    strm_.seekg( -3, ios::end );
    char buf[3*mIntSz];
    strm_.read( buf, 3 ); buf[3] = '\0';
    if ( strcmp(buf,"dGB") ) mErrRet("Missing required file trailer")
    
    strm_.seekg( -4, ios::end );
    info_.geom.fullyrectangular = (bool)strm_.peek();
    if ( info_.geom.fullyrectangular )
    {
	strm_.seekg( -4-6*mIntSz, ios::end );
	return readBinIDBounds( iinterp );
    }
    strm_.seekg( -4-mIntSz, ios::end ); strm_.read( buf, mIntSz );
    const int nrbytesrest = iinterp.get( buf, 0 );

    strm_.seekg( -4-nrbytesrest, ios::end ); strm_.read( buf, mIntSz );
    const int nrinl = iinterp.get( buf, 0 );

    for ( int iinl=0; iinl<nrinl; iinl++ )
    {
	strm_.read( buf, 2 * mIntSz );
	CBVSInfo::SurvGeom::InlineInfo* iinf
		= new CBVSInfo::SurvGeom::InlineInfo(
						iinterp.get( buf, 0 ) );
	const int nrseg = iinterp.get( buf, 1 );
	for ( int iseg=0; iseg<nrseg; iseg++ )
	{
	    strm_.read( buf, 3 * mIntSz );
	    iinf->segments += CBVSInfo::SurvGeom::InlineInfo::Segment(
		iinterp.get(buf,0), iinterp.get(buf,1), iinterp.get(buf,2) );
	}
    }

    return strm_.good();
}


bool CBVSReader::readBinIDBounds( const DataInterpreter<int>& iinterp )
{
    return true;
}
