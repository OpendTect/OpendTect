/*+
 * COPYRIGHT: (C) de Groot-Bril Earth Sciences B.V.
 * AUTHOR   : A.H. Bril
 * DATE     : Mar 2001
 * FUNCTION : CBVS I/O
-*/

static const char* rcsID = "$Id: cbvsreader.cc,v 1.3 2001-04-04 11:12:11 bert Exp $";

#include "cbvsreader.h"
#include "datainterp.h"


CBVSReader::CBVSReader( istream* s )
	: strm_(*s)
	, iinterp(DataCharacteristics())
	, finterp(DataCharacteristics())
	, dinterp(DataCharacteristics())
	, nrxlines(0)
	, bytespertrace(0)
	, hinfofetched(false)
	, isclosed(false)
	, posidx(0)
	, datastartfo(readInfo())
{
    lastposfo = datastartfo;
}


void CBVSReader::close()
{
    if ( !isclosed )
	delete &strm_;
    isclosed = true;
}


#define mErrRet(s) { errmsg_ = s; return 0; }

streampos CBVSReader::readInfo()
{
    info_.clean();
    errmsg_ = check( strm_ );
    if ( errmsg_ ) return 0;

    unsigned char buf[headstartbytes];
    strm_.read( buf, headstartbytes );

    DataCharacteristics dc;
    dc.littleendian = buf[3] != 0;
    finterp.set( dc );
    dc.setNrBytes( BinDataDesc::N8 );
    dinterp.set( dc );
    dc.BinDataDesc::set( true, true, BinDataDesc::N4 );
    iinterp.set( dc );

    // const int version = (int)buf[4];
    getExplicits( buf+5 );
    // const int nrbytesinheader = iinterp.get( buf+8 );

    if ( !readComps() || !readGeom() )
	return 0;

    strm_.read( buf, 2 * integersize );
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
    return info_.geom.fullyrectandreg || readTrailer() ? dfo : 0;
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

    explicitnrbytes = 0;
    if ( info_.explinfo.startpos )
	explicitnrbytes += sizeof(CBVSInfo::ExplicitData::startpos);
    if ( info_.explinfo.coord )
	explicitnrbytes += sizeof(CBVSInfo::ExplicitData::coord);
    if ( info_.explinfo.offset )
	explicitnrbytes += sizeof(CBVSInfo::ExplicitData::offset);
    if ( info_.explinfo.pick )
	explicitnrbytes += sizeof(CBVSInfo::ExplicitData::pick);
    if ( info_.explinfo.refpos )
	explicitnrbytes += sizeof(CBVSInfo::ExplicitData::refpos);
}


#undef mErrRet
#define mErrRet(s) { strm_.seekg( 0, ios::beg ); errmsg_ = s; return false; }


bool CBVSReader::readComps()
{
    unsigned char ucbuf[4*integersize];
    strm_.read( ucbuf, integersize );
    const int nrcomp = iinterp.get( ucbuf, 0 );
    if ( nrcomp < 1 ) mErrRet("Corrupt CBVS format: No components defined")

    cnrbytes_ = new int [nrcomp];
    bytespertrace = 0;

    for ( int icomp=0; icomp<nrcomp; icomp++ )
    {
	strm_.read( ucbuf, integersize );
	    int nrchar = iinterp.get( ucbuf, 0 ); char buf[nrchar+1];
	strm_.read( buf, nrchar ); buf[nrchar] = '\0';
	    CBVSComponentInfo* newinf = new CBVSComponentInfo( buf );
	
	strm_.read( ucbuf, integersize );
	    newinf->datatype = iinterp.get( ucbuf, 0 );
	strm_.read( ucbuf, sizeof(unsigned short) );
	    newinf->datachar.set( *((unsigned short*)ucbuf) );
	strm_.read( ucbuf, sizeof(float) );
	    newinf->sd.start = finterp.get( ucbuf, 0 );
	strm_.read( ucbuf, sizeof(float) );
	    newinf->sd.step = finterp.get( ucbuf, 0 );
	strm_.read( ucbuf, integersize );
	    newinf->nrsamples = iinterp.get( ucbuf, 0 );
	strm_.read( ucbuf, 2*sizeof(float) );
	{
	    float a = finterp.get( ucbuf, 0 );
	    float b = finterp.get( ucbuf, 1 );
	    if ( !mIS_ZERO(a) || !mIS_ZERO(1-b) )
		newinf->scaler = new LinScaler( a, b );
	}

	if ( newinf->nrsamples < 0 || newinf->datatype < 0 )
	{
	    delete newinf;
	    mErrRet("Corrupt CBVS format: Component desciption error")
	}

	cnrbytes_[icomp] = newinf->nrsamples * newinf->datachar.nrBytes();
	bytespertrace += cnrbytes_[icomp];
	info_.compinfo += newinf;
    }

    return true;
}


bool CBVSReader::readGeom()
{
    unsigned char buf[8*sizeof(double)];
    strm_.read( buf, 8*integersize );
    info_.geom.fullyrectandreg = (bool)iinterp.get( buf, 0 );
    info_.nrtrcsperposn = iinterp.get( buf, 1 );
    info_.geom.start.inl = iinterp.get( buf, 2 );
    info_.geom.start.crl = iinterp.get( buf, 3 );
    info_.geom.stop.inl = iinterp.get( buf, 4 );
    info_.geom.stop.crl = iinterp.get( buf, 5 );
    info_.geom.step.inl = iinterp.get( buf, 6 );
    info_.geom.step.crl = iinterp.get( buf, 7 );
    strm_.read( buf, 6*sizeof(double) );
    BinID2Coord::BCTransform xtr, ytr;
    xtr.a = dinterp.get( buf, 0 ); xtr.b = dinterp.get( buf, 1 );
    xtr.c = dinterp.get( buf, 2 ); ytr.a = dinterp.get( buf, 3 );
    ytr.b = dinterp.get( buf, 4 ); ytr.c = dinterp.get( buf, 5 );
    info_.geom.b2c.setTransforms( xtr, ytr );

    if ( info_.geom.fullyrectandreg )
	nrxlines = (info_.geom.stop.crl - info_.geom.start.crl)
                 / info_.geom.step.crl + 1;

    return strm_.good();
}


#undef mErrRet
#define mErrRet(s) { strm_.seekg( 0, ios::beg ); return s; }

bool CBVSReader::readTrailer()
{
    strm_.seekg( -3, ios::end );
    char buf[3*integersize];
    strm_.read( buf, 3 ); buf[3] = '\0';
    if ( strcmp(buf,"BGd") ) mErrRet("Missing required file trailer")
    
    strm_.seekg( -4-integersize, ios::end );
    strm_.read( buf, integersize );
    int nrbytes = iinterp.get( buf, 0 );

    strm_.seekg( -4-integersize-nrbytes, ios::end );
    strm_.read( buf, integersize );
    const int nrinl = iinterp.get( buf, 0 );

    for ( int iinl=0; iinl<nrinl; iinl++ )
    {
	strm_.read( buf, 2 * integersize );
	CBVSInfo::SurvGeom::InlineInfo* iinf
		= new CBVSInfo::SurvGeom::InlineInfo(
						iinterp.get( buf, 0 ) );
	const int nrseg = iinterp.get( buf, 1 );
	for ( int iseg=0; iseg<nrseg; iseg++ )
	{
	    strm_.read( buf, 3 * integersize );
	    iinf->segments += CBVSInfo::SurvGeom::InlineInfo::Segment(
		iinterp.get(buf,0), iinterp.get(buf,1), iinterp.get(buf,2) );
	}
    }

    return strm_.good();
}


bool CBVSReader::goTo( const BinID& bid )
{
    if ( isclosed ) return false;

    int nrposns = 0;
    if ( info_.geom.fullyrectandreg )
    {
	if ( bid.inl < info_.geom.start.inl || bid.inl > info_.geom.stop.inl
	  || bid.crl < info_.geom.start.crl || bid.crl > info_.geom.stop.crl )
	    return false;
	
	if ( info_.geom.step.inl == 1 )
	    curbinid.inl = bid.inl;
	else
	{
	    StepInterval<int> inls( info_.geom.start.inl, info_.geom.stop.inl,
				    info_.geom.step.inl );
	    curbinid.inl = inls.atIndex( inls.nearestIndex( bid.inl ) );
	}
	if ( info_.geom.step.crl == 1 )
	    curbinid.crl = bid.crl;
	else
	{
	    StepInterval<int> crls( info_.geom.start.crl, info_.geom.stop.crl,
				    info_.geom.step.crl );
	    curbinid.crl = crls.atIndex( crls.nearestIndex( bid.crl ) );
	}
	nrposns =
	    ((bid.inl-info_.geom.start.inl) / info_.geom.step.inl) * nrxlines
	  + ((bid.crl-info_.geom.start.crl) / info_.geom.step.crl);
    }
    else
    {
	const int sz = info_.geom.inldata.size();
	bool foundseg = false;
	for ( int iinl=0; iinl<sz; iinl++ )
	{
	    const CBVSInfo::SurvGeom::InlineInfo& inlinf
					= *info_.geom.inldata[iinl];
	    foundseg = false;
	    for ( int iseg=0; iseg<inlinf.segments.size(); iseg++ )
	    {
		if ( inlinf.inl != bid.inl
		  || !inlinf.segments[iseg].includes(bid.crl) )
		    nrposns += inlinf.segments[iseg].nrSteps();
		else
		{
		    lastinlinfnr = iinl; lastsegnr = iseg;
		    int posns = inlinf.segments[iseg].nearestIndex( bid.crl );
		    nrposns += posns;
		    curbinid.inl = inlinf.inl;
		    curbinid.crl = inlinf.segments[iseg].atIndex( posns );
		    foundseg = true;
		    break;
		}
	    }

	    if ( foundseg ) break;
	}
	if ( !foundseg ) return false;
    }

    lastposfo = ((streampos)nrposns) * info_.nrtrcsperposn
	      * (explicitnrbytes + bytespertrace);
    strm_.seekg( datastartfo + lastposfo, ios::beg );
    hinfofetched = false;
    return true;
}


bool CBVSReader::toNext( bool skiptonextpos )
{
    hinfofetched = false;
    if ( !skiptonextpos )
    {
	posidx++;
	if ( posidx >= info_.nrtrcsperposn )
	    posidx = 0;
    }

    if ( posidx )
    {
	lastposfo += (explicitnrbytes + bytespertrace);
	strm_.seekg( datastartfo + lastposfo, ios::beg );
	return true;
    }

    // Check whether we are at last position
    return true;
}


#define mGet(memb) \
    if ( info_.explinfo.memb ) \
    { \
	strm_.read( buf, sizeof(expldat.memb) ); \
	expldat.memb = finterp.get( buf, 0 ); \
    }
#define mGetCoord() \
    if ( info_.explinfo.coord ) \
    { \
	strm_.read( buf, 2*sizeof(expldat.coord.x) ); \
	expldat.coord.x = dinterp.get( buf, 0 ); \
	expldat.coord.y = dinterp.get( buf, 1 ); \
    }

bool CBVSReader::getHInfo( CBVSInfo::ExplicitData& expldat )
{
    if ( isclosed || hinfofetched ) return true;

    unsigned char buf[8];
    mGet(startpos)
    mGetCoord()
    mGet(offset)
    mGet(pick)
    mGet(refpos)

    return strm_.good();
}
