/*+
 * COPYRIGHT: (C) de Groot-Bril Earth Sciences B.V.
 * AUTHOR   : A.H. Bril
 * DATE     : Mar 2001
 * FUNCTION : CBVS I/O
-*/

static const char* rcsID = "$Id: cbvsreader.cc,v 1.17 2001-05-31 13:25:38 windev Exp $";

#include "cbvsreader.h"
#include "datainterp.h"
#include "binidselimpl.h"
#include "survinfo.h"


CBVSReader::CBVSReader( istream* s )
	: strm_(*s)
	, iinterp(DataCharacteristics())
	, finterp(DataCharacteristics())
	, dinterp(DataCharacteristics())
	, bytespertrace(0)
	, hinfofetched(false)
	, posidx(0)
	, datastartfo(0)
	, lastposfo(0)
	, bidrg(*new BinIDRange)
	, samprgs(0)
{
    if ( readInfo() )
	toOffs( datastartfo );
}


CBVSReader::~CBVSReader()
{
    close();
    delete [] samprgs;
    delete &bidrg;
}


void CBVSReader::close()
{
    if ( !strmclosed_ )
	delete &strm_;
    strmclosed_ = true;
}


#define mErrRet(s) { errmsg_ = s; return 0; }

bool CBVSReader::readInfo()
{
    info_.clean();
    errmsg_ = check( strm_ );
    if ( errmsg_ ) return false;

    BufferString buf(headstartbytes);
    strm_.read( buf.buf(), headstartbytes );

    DataCharacteristics dc;
    dc.littleendian = buf[3] != 0;
    finterp.set( dc );
    dc.setNrBytes( BinDataDesc::N8 );
    dinterp.set( dc );
    dc.BinDataDesc::set( true, true, BinDataDesc::N4 );
    iinterp.set( dc );

    // const int version = (int)buf[4];
    getExplicits( (const unsigned char *)buf.buf()+5 );
    // const int nrbytesinheader = iinterp.get( buf+8 );

    strm_.read( buf.buf(), integersize );
    getText( iinterp.get(buf,0), info_.stdtext );

    if ( !readComps() || !readGeom() )
	return false;

    strm_.read( buf.buf(), 2 * integersize );
    info_.seqnr = iinterp.get( buf, 0 );
    getText( iinterp.get(buf,1), info_.usertext );
    removeTrailingBlanks( info_.usertext.buf() );

    datastartfo = strm_.tellg();
    curbinid_ = info_.geom.start;
    if ( !info_.geom.fullyrectandreg && !readTrailer() )
	return false;

    lastbinid.inl = info_.geom.step.inl > 0
		  ? info_.geom.stop.inl : info_.geom.start.inl;
    lastbinid.crl = info_.geom.step.crl > 0
		  ? info_.geom.stop.crl : info_.geom.start.crl;
    if ( !info_.geom.fullyrectandreg )
    {
	CBVSInfo::SurvGeom::InlineInfo& iinf =
		*info_.geom.inldata[ info_.geom.inldata.size()-1 ];
	lastbinid.inl = iinf.inl;
	lastbinid.crl = iinf.segments[ iinf.segments.size()-1 ].stop;
    }

    return true;
}


void CBVSReader::getText( int nrchar, BufferString& txt )
{
    if ( nrchar > 0 )
    {
	txt.setBufSize(nrchar+1);
	strm_.read( txt.buf(), nrchar );
	txt[nrchar] = '\0';
    }
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

    char plf; strm.read( &plf, 1 );
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
	explicitnrbytes += sizeof(float);
    if ( info_.explinfo.coord )
	explicitnrbytes += sizeof(Coord);
    if ( info_.explinfo.offset )
	explicitnrbytes += sizeof(float);
    if ( info_.explinfo.pick )
	explicitnrbytes += sizeof(float);
    if ( info_.explinfo.refpos )
	explicitnrbytes += sizeof(float);
}


#undef mErrRet
#define mErrRet(s) { toOffs(0); errmsg_ = s; return false; }


bool CBVSReader::readComps()
{
#ifdef __msvc__
    BufferString mscbuf(4*integersize);
    #define ucbuf mscbuf.buf()
#else
    unsigned char ucbuf[4*integersize];
#endif

    strm_.read( ucbuf, integersize );
    nrcomps_ = iinterp.get( ucbuf, 0 );
    if ( nrcomps_ < 1 ) mErrRet("Corrupt CBVS format: No components defined")

    cnrbytes_ = new int [nrcomps_];
    samprgs = new Interval<int> [nrcomps_];
    bytespertrace = 0;

    for ( int icomp=0; icomp<nrcomps_; icomp++ )
    {
	strm_.read( ucbuf, integersize );
	BufferString bs; getText( iinterp.get(ucbuf,0), bs );
	BasicComponentInfo* newinf = new BasicComponentInfo( (const char*)bs );
	
	strm_.read( ucbuf, integersize );
	    newinf->datatype = iinterp.get( ucbuf, 0 );
	strm_.read( ucbuf, sizeof(unsigned short) );
	    newinf->datachar.set( *((unsigned short*)ucbuf) );
	strm_.read( ucbuf, sizeof(unsigned short) );
	    // reserved for compression type
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

	info_.compinfo += newinf;

	cnrbytes_[icomp] = newinf->nrsamples * newinf->datachar.nrBytes();
	bytespertrace += cnrbytes_[icomp];
	samprgs[icomp] = Interval<int>( 0, newinf->nrsamples-1 );
    }

    return true;
}


bool CBVSReader::readGeom()
{
    char buf[8*sizeof(double)];
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
    if ( xtr.valid(ytr) )
	info_.geom.b2c.setTransforms( xtr, ytr );
    else
	info_.geom.b2c = SI().binID2Coord();

    if ( info_.geom.fullyrectandreg )
	nrxlines_ = (info_.geom.stop.crl - info_.geom.start.crl)
                  / info_.geom.step.crl + 1;
    bidrg.start = bidrg.stop
		= BinID( info_.geom.start.inl, info_.geom.start.crl );
    bidrg.include( BinID( info_.geom.stop.inl, info_.geom.stop.crl ) );

    return strm_.good();
}


bool CBVSReader::readTrailer()
{
    strm_.seekg( -3, ios::end );
    BufferString buf(3*integersize);
    strm_.read( buf.buf(), 3 ); buf[3] = '\0';
    if ( strcmp(buf,"BGd") ) mErrRet("Missing required file trailer")
    
    strm_.seekg( -4-integersize, ios::end );
    strm_.read( buf.buf(), integersize );
    int nrbytes = iinterp.get( buf, 0 );

    strm_.seekg( -4-integersize-nrbytes, ios::end );
    strm_.read( buf.buf(), integersize );
    const int nrinl = iinterp.get( buf, 0 );
    if ( nrinl == 0 ) mErrRet("No traces in file")

    for ( int iinl=0; iinl<nrinl; iinl++ )
    {
	strm_.read( buf.buf(), 2 * integersize );
	CBVSInfo::SurvGeom::InlineInfo* iinf
		= new CBVSInfo::SurvGeom::InlineInfo(
						iinterp.get( buf, 0 ) );
	const int nrseg = iinterp.get( buf, 1 );
	for ( int iseg=0; iseg<nrseg; iseg++ )
	{
	    strm_.read( buf.buf(), 3 * integersize );
	    iinf->segments += CBVSInfo::SurvGeom::InlineInfo::Segment(
		iinterp.get(buf,0), iinterp.get(buf,1), iinterp.get(buf,2) );
	}
	info_.geom.inldata += iinf;
    }

    curinlinfnr = cursegnr = 0;
    curbinid_.inl = info_.geom.inldata[curinlinfnr]->inl;
    curbinid_.crl = info_.geom.inldata[curinlinfnr]->segments[cursegnr].start;
    return strm_.good();
}


bool CBVSReader::toStart()
{
    if ( strmclosed_ ) return false;

    curbinid_ = info_.geom.start;
    if ( !info_.geom.fullyrectandreg )
    {
	curinlinfnr = cursegnr = 0;
	curbinid_.inl = info_.geom.inldata[0]->inl;
	curbinid_.crl = info_.geom.inldata[0]->segments[0].start;
    }
    posidx = 0;
    toOffs( datastartfo );
    hinfofetched = false;
    return true;
}


void CBVSReader::toOffs( streampos sp )
{
    lastposfo = sp;
    strm_.seekg( lastposfo, ios::beg );
}


bool CBVSReader::goTo( const BinID& bid )
{
    if ( strmclosed_ ) return false;

    int nrposns = 0;
    if ( info_.geom.fullyrectandreg )
    {
	if ( !bidrg.includes(bid) )
	    return false;
	
	if ( info_.geom.step.inl == 1 )
	    curbinid_.inl = bid.inl;
	else
	{
	    StepInterval<int> inls( info_.geom.start.inl, info_.geom.stop.inl,
				    info_.geom.step.inl );
	    curbinid_.inl = inls.atIndex( inls.nearestIndex( bid.inl ) );
	}
	if ( info_.geom.step.crl == 1 )
	    curbinid_.crl = bid.crl;
	else
	{
	    StepInterval<int> crls( info_.geom.start.crl, info_.geom.stop.crl,
				    info_.geom.step.crl );
	    curbinid_.crl = crls.atIndex( crls.nearestIndex( bid.crl ) );
	}
	nrposns =
	    ((bid.inl-info_.geom.start.inl) / info_.geom.step.inl) * nrxlines_
	  + ((bid.crl-info_.geom.start.crl) / info_.geom.step.crl);
    }
    else
    {
	const CBVSInfo::SurvGeom::InlineInfo* curiinf =
		info_.geom.inldata[curinlinfnr];
	const CBVSInfo::SurvGeom::InlineInfo::Segment* curseg =
		&curiinf->segments[cursegnr];
	if ( bid.inl != curiinf->inl || !curseg->includes(bid.crl) )
	{
	    const int sz = info_.geom.inldata.size();
	    bool foundseg = false;
	    for ( int iinl=0; iinl<sz; iinl++ )
	    {
		curiinf = info_.geom.inldata[iinl];
		foundseg = false;
		for ( int iseg=0; iseg<curiinf->segments.size(); iseg++ )
		{
		    curseg = &curiinf->segments[iseg];
		    if ( curiinf->inl != bid.inl || !curseg->includes(bid.crl) )
			nrposns += curseg->nrSteps();
		    else
		    {
			curinlinfnr = iinl; cursegnr = iseg;
			foundseg = true;
			break;
		    }
		}

		if ( foundseg ) break;
	    }
	    if ( !foundseg ) return false;
	}

	int segposn = curseg->nearestIndex( bid.crl );
	nrposns += segposn;
	curbinid_.inl = curiinf->inl;
	curbinid_.crl = curseg->atIndex( segposn );
    }

    toOffs( datastartfo + ((streampos)nrposns) * info_.nrtrcsperposn
	      * (explicitnrbytes + bytespertrace) );
    hinfofetched = false;
    return true;
}


bool CBVSReader::nextPosIdx()
{
    posidx++;
    if ( posidx >= info_.nrtrcsperposn )
	posidx = 0;

    if ( !posidx )
    {
	if ( curbinid_ == lastbinid )
	    return false;

	curbinid_ = nextBinID();
    }
    return true;
}


bool CBVSReader::skip( bool tonextpos )
{
    if ( hinfofetched ) 
	hinfofetched = false;
    else if ( !nextPosIdx() )
	return false;

    streampos onetrcoffs = explicitnrbytes + bytespertrace;
    streampos posadd = onetrcoffs;

    if ( posidx && tonextpos )
    {
	while ( posidx )
	{
	    posadd += onetrcoffs;
	    if ( !nextPosIdx() ) return false;
	}
    }

    toOffs( lastposfo + posadd );
    return true;
}


BinID CBVSReader::nextBinID() const
{
    if ( curbinid_ == lastbinid )
	return BinID(0,0);

    BinID bid = curbinid_;
    if ( info_.geom.fullyrectandreg )
    {
	bid.crl += info_.geom.step.crl;
	if ( !bidrg.includes(bid) )
	{
	    bid.crl = info_.geom.start.crl;
	    bid.inl += info_.geom.step.inl;
	    if ( !bidrg.includes(bid) )
		// Huh?
		return BinID(0,0);
	}
    }
    else
    {
	const CBVSInfo::SurvGeom::InlineInfo* inlinf =
		info_.geom.inldata[curinlinfnr];
	const CBVSInfo::SurvGeom::InlineInfo::Segment* curseg =
		&inlinf->segments[cursegnr];
	bid.crl += curseg->step;
	if ( !curseg->includes(bid.crl) )
	{
	    const_cast<int&>(cursegnr) ++;
	    if ( cursegnr >= inlinf->segments.size() )
	    {
		const_cast<int&>(cursegnr) = 0;
		const_cast<int&>(curinlinfnr) ++;
		if ( curinlinfnr >= info_.geom.inldata.size() )
		    // Huh?
		    return BinID(0,0);
		inlinf = info_.geom.inldata[curinlinfnr];
		curseg = &inlinf->segments[cursegnr];
		bid.inl = inlinf->inl;
		bid.crl = curseg->start;
	    }
	}
    }

    return bid;
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
    if ( strmclosed_ || hinfofetched ) return true;

    expldat.binid = curbinid_;
    expldat.coord = info_.geom.b2c.transform( curbinid_ );
    expldat.startpos = info_.compinfo[0]->sd.start;
    expldat.offset = 0;
    expldat.pick = expldat.refpos = mUndefValue;

    char buf[8];
    mGet(startpos)
    mGetCoord()
    mGet(offset)
    mGet(pick)
    mGet(refpos)

    hinfofetched = true;
    return strm_.good();
}


bool CBVSReader::fetch( void** bufs, const Interval<int>* samps )
{
    if ( !hinfofetched )
    {
	CBVSInfo::ExplicitData dum;
	if ( !getHInfo(dum) ) return false;
    }

    if ( !samps ) samps = samprgs;

    for ( int icomp=0; icomp<nrcomps_; icomp++ )
    {
	BasicComponentInfo* compinfo = info_.compinfo[icomp];
	if ( samps[icomp].start > samps[icomp].stop )
	{
	    strm_.seekg( cnrbytes_[icomp], ios::cur );
	    continue;
	}

	int bps = compinfo->datachar.nrBytes();
	if ( samps[icomp].start )
	    strm_.seekg( samps[icomp].start*bps, ios::cur );
	strm_.read( (char*)bufs[icomp],
		(samps[icomp].stop-samps[icomp].start+1) * bps );
	if ( samps[icomp].stop < compinfo->nrsamples-1 )
	    strm_.seekg( (compinfo->nrsamples-samps[icomp].stop-1)*bps,
		ios::cur );
    }

    hinfofetched = false;
    return strm_.good() || strm_.eof();
}
