/*+
 * COPYRIGHT: (C) de Groot-Bril Earth Sciences B.V.
 * AUTHOR   : A.H. Bril
 * DATE     : Mar 2001
 * FUNCTION : CBVS I/O
-*/

static const char* rcsID = "$Id: cbvsreader.cc,v 1.36 2002-07-25 12:56:24 bert Exp $";

/*!

The CBVS header starts with the following bytes:
0: 'd'
1: 'G'
2: 'B'
3: platform indicator set by put_platform()
4: version of CBVS.
5: info on "aux pos info"
6: not used (yet)
7: not used (yet)

The next 8 bytes are reserved for 2 integers:
8-11: total nr bytes in the CBVS header
12-15: not used (yet) here, 12 is used for aux flags in aux file.

*/

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
    	, needaux(true)
{
    if ( readInfo() )
	toOffs( datastartfo );
}


CBVSReader::~CBVSReader()
{
    close();
    delete &bidrg;
}


void CBVSReader::close()
{
    if ( !strmclosed_ && &strm_ != &cin )
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
    unsigned char* ptr = (unsigned char*)buf.buf();
    strm_.read( ptr, headstartbytes );

    DataCharacteristics dc;
    dc.littleendian = ptr[3] != 0;
    finterp.set( dc );
    dc.setNrBytes( BinDataDesc::N8 );
    dinterp.set( dc );
    dc.BinDataDesc::set( true, true, BinDataDesc::N4 );
    iinterp.set( dc );

    // const int version = (int)ptr[4];
    getAuxInfoSel( ptr + 5 );
    // const int nrbytesinheader = iinterp.get( ptr+8 );

    strm_.read( ptr, integersize );
    getText( iinterp.get(ptr,0), info_.stdtext );

    if ( !readComps() || !readGeom() )
	return false;

    strm_.read( ptr, 2 * integersize );
    info_.seqnr = iinterp.get( ptr, 0 );
    getText( iinterp.get(ptr,1), info_.usertext );
    removeTrailingBlanks( info_.usertext.buf() );

    datastartfo = strm_.tellg();
    if ( !info_.geom.fullyrectandreg && !readTrailer() )
	return false;

    firstbinid.inl = info_.geom.step.inl > 0
		   ? info_.geom.start.inl : info_.geom.stop.inl;
    firstbinid.crl = info_.geom.step.crl > 0
		   ? info_.geom.start.crl : info_.geom.stop.crl;
    lastbinid.inl  = info_.geom.step.inl > 0
		   ? info_.geom.stop.inl : info_.geom.start.inl;
    lastbinid.crl  = info_.geom.step.crl > 0
		   ? info_.geom.stop.crl : info_.geom.start.crl;
    if ( info_.geom.fullyrectandreg )
	nrxlines_ = (lastbinid.crl - firstbinid.crl) / info_.geom.step.crl + 1;
    else
    {
	CBVSInfo::SurvGeom::InlineInfo* iinf =
		info_.geom.inldata[ info_.geom.inldata.size()-1 ];
	lastbinid.inl = iinf->inl;
	lastbinid.crl = iinf->segments[ iinf->segments.size()-1 ].stop;
	iinf = info_.geom.inldata[0];
	firstbinid.inl = iinf->inl;
	firstbinid.crl = iinf->segments[0].start;
    }

    curbinid_ = firstbinid;
    return true;
}


void CBVSReader::getText( int nrchar, BufferString& txt )
{
    if ( nrchar > 0 )
    {
	if ( txt.bufSize() <= nrchar )
	    txt.setBufSize(nrchar+1);
	strm_.read( txt.buf(), nrchar );
	txt[nrchar] = '\0';
    }
}


#undef mErrRet
#define mErrRet { strm.seekg( 0, ios::beg ); return msg; }

const char* CBVSReader::check( istream& strm )
{
    if ( strm.bad() ) return "Input stream cannot be used";
    if ( strm.fail() ) strm.clear();

    strm.seekg( 0, ios::beg );
    char buf[4]; memset( buf, 0, 4 );
    strm.read( buf, 3 );
    const char* msg = "Input stream cannot be used";
    if ( !strm.good() ) mErrRet;

    msg = "File is not in CBVS format";
    if ( strcmp(buf,"dGB") ) mErrRet;

    char plf; strm.read( &plf, 1 );
    if ( plf > 2 ) mErrRet;

    strm.seekg( 0, ios::beg );
    return 0;
}


void CBVSReader::getAuxInfoSel( const unsigned char* ptr )
{
    info_.auxinfosel.startpos =	mAuxSetting(ptr,1);
    info_.auxinfosel.coord =	mAuxSetting(ptr,2);
    info_.auxinfosel.offset =	mAuxSetting(ptr,4);
    info_.auxinfosel.pick =	mAuxSetting(ptr,8);
    info_.auxinfosel.refpos =	mAuxSetting(ptr,16);
    info_.auxinfosel.azimuth =	mAuxSetting(ptr,32);

#define mAddBytes(memb,t) \
    if ( info_.auxinfosel.memb ) auxnrbytes += sizeof(t)
    auxnrbytes = 0;
    mAddBytes(startpos,float);
    mAddBytes(coord,double); mAddBytes(coord,double); // both x and y
    mAddBytes(offset,float);
    mAddBytes(pick,float);
    mAddBytes(refpos,float);
    mAddBytes(azimuth,float);
}


#undef mErrRet
#define mErrRet(s) { toOffs(0); errmsg_ = s; return false; }


bool CBVSReader::readComps()
{
#ifdef __msvc__
    BufferString mscbuf(4*integersize);
    unsigned char* ucbuf = (unsigned char*)mscbuf.buf();
#else
    unsigned char ucbuf[4*integersize];
#endif

    strm_.read( ucbuf, integersize );
    nrcomps_ = iinterp.get( ucbuf, 0 );
    if ( nrcomps_ < 1 ) mErrRet("Corrupt CBVS format: No components defined")

    cnrbytes_ = new int [nrcomps_];
    bytespertrace = 0;

    for ( int icomp=0; icomp<nrcomps_; icomp++ )
    {
	strm_.read( ucbuf, integersize );
	BufferString bs; getText( iinterp.get(ucbuf,0), bs );
	BasicComponentInfo* newinf = new BasicComponentInfo( (const char*)bs );
	
	strm_.read( ucbuf, integersize );
	    newinf->datatype = iinterp.get( ucbuf, 0 );
	strm_.read( ucbuf, 4 );
	    newinf->datachar.set( ucbuf[0], ucbuf[1] );
	    // extra 2 bytes reserved for compression type
	    newinf->datachar.fmt = DataCharacteristics::Ieee;
	    //TODO find out how to allow SGI!!
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
	samprg = Interval<int>( 0, newinf->nrsamples-1 );
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
    const int nrbytes = iinterp.get( buf, 0 );

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

    curinlinfnr_ = cursegnr_ = 0;
    curbinid_.inl = info_.geom.inldata[curinlinfnr_]->inl;
    curbinid_.crl = info_.geom.inldata[curinlinfnr_]->segments[cursegnr_].start;
    return strm_.good();
}


bool CBVSReader::toStart()
{
    if ( strmclosed_ ) return false;

    curbinid_ = firstbinid;
    if ( !info_.geom.fullyrectandreg )
    {
	curinlinfnr_ = cursegnr_ = 0;
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
    int ci = curinlinfnr_, cc = cursegnr_;
    int posnr = getPosNr( bid, ci, cc );
    if ( posnr < 0 ) return false;

    return goTo( posnr, bid, ci, cc );
}


bool CBVSReader::goTo( int posnr, const BinID& bid, int ci, int cs )
{
    // Be careful: offsets can be larger than what fits in an int!
    streampos sp = posnr * info_.nrtrcsperposn;
    sp *= auxnrbytes + bytespertrace;

    toOffs( datastartfo + sp );
    hinfofetched = false;
    curbinid_ = bid;
    curinlinfnr_ = ci; cursegnr_ = cs;
    return true;
}


int CBVSReader::getPosNr( const BinID& bid,
			  int& curinlinfnr, int& cursegnr ) const
{
    if ( strmclosed_ ) return -1;

    int posnr = 0;
    BinID curbinid;
    if ( info_.geom.fullyrectandreg )
    {
	if ( !bidrg.includes(bid) )
	    return -1;
	
	if ( info_.geom.step.inl == 1 )
	    curbinid.inl = bid.inl;
	else
	{
	    StepInterval<int> inls( firstbinid.inl, lastbinid.inl,
				    info_.geom.step.inl );
	    curbinid.inl = inls.atIndex( inls.nearestIndex( bid.inl ) );
	}
	if ( info_.geom.step.crl == 1 )
	    curbinid.crl = bid.crl;
	else
	{
	    StepInterval<int> crls( firstbinid.crl, lastbinid.crl,
				    info_.geom.step.crl );
	    curbinid.crl = crls.atIndex( crls.nearestIndex( bid.crl ) );
	}
	posnr =
	    ((bid.inl-firstbinid.inl) / info_.geom.step.inl) * nrxlines_
	  + ((bid.crl-firstbinid.crl) / info_.geom.step.crl);
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
			posnr += curseg->nrSteps() + 1;
		    else
		    {
			curinlinfnr = iinl; cursegnr = iseg;
			foundseg = true;
			break;
		    }
		}

		if ( foundseg ) break;
	    }
	    if ( !foundseg ) return -1;
	}

	int segposn = curseg->nearestIndex( bid.crl );
	posnr += segposn;
	curbinid.inl = curiinf->inl;
	curbinid.crl = curseg->atIndex( segposn );
    }

    return posnr;
}


bool CBVSReader::nextPosIdx()
{
    posidx++;
    if ( posidx >= info_.nrtrcsperposn )
	posidx = 0;

    if ( !posidx )
	return getNextBinID( curbinid_, curinlinfnr_, cursegnr_ );

    return true;
}


bool CBVSReader::skip( bool tonextpos )
{
    if ( hinfofetched ) 
	hinfofetched = false;
    else if ( !nextPosIdx() )
	return false;

    streampos onetrcoffs = auxnrbytes + bytespertrace;
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
    BinID bid; int ci = curinlinfnr_; int cc = cursegnr_;
    const_cast<CBVSReader*>(this)->getNextBinID( bid, ci, cc );
    return bid;
}


bool CBVSReader::getNextBinID( BinID& bid, int& curinlinfnr, int& cursegnr )
{
    if ( curbinid_ == lastbinid )
	{ bid = BinID(0,0); return false; }

    if ( &bid != &curbinid_ )
	bid = curbinid_;

    if ( info_.geom.fullyrectandreg )
    {
	bid.crl += info_.geom.step.crl;
	if ( !bidrg.includes(bid) )
	{
	    bid.crl = firstbinid.crl;
	    bid.inl += info_.geom.step.inl;
	    if ( !bidrg.includes(bid) )
		// Huh?
		{ bid = BinID(0,0); return false; }
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
	    if ( curseg->step > 0 && bid.crl < curseg->start )
		bid.crl = curseg->start;
	    else if ( curseg->step < 0 && bid.crl > curseg->start )
		bid.crl = curseg->stop;
	    else
	    {
		cursegnr++;
		if ( cursegnr < inlinf->segments.size() )
		    bid.crl = inlinf->segments[cursegnr].start;
		else
		{
		    cursegnr = 0;
		    curinlinfnr++;
		    if ( curinlinfnr >= info_.geom.inldata.size() )
			// Huh?
			{ bid = BinID(0,0); return false; }
		    inlinf = info_.geom.inldata[curinlinfnr];
		    curseg = &inlinf->segments[cursegnr];
		    bid.inl = inlinf->inl;
		    bid.crl = curseg->start;
		}
	    }
	}
    }

    return true;
}


#define mCondGetAux(memb) \
    if ( info_.auxinfosel.memb ) \
	{ mGetAuxFromStrm(auxinf,buf,memb,strm_); }
#define mCondGetCoordAux() \
    if ( info_.auxinfosel.coord ) \
	{ mGetCoordAuxFromStrm(auxinf,buf,strm_); }

bool CBVSReader::getAuxInfo( PosAuxInfo& auxinf )
{
    if ( strmclosed_ )
	return true;

    auxinf.binid = curbinid_;
    auxinf.coord = info_.geom.b2c.transform( curbinid_ );
    auxinf.startpos = info_.compinfo[0]->sd.start;
    auxinf.offset = auxinf.azimuth = 0;
    auxinf.pick = auxinf.refpos = mUndefValue;

    if ( !auxnrbytes )
	return true;

    if ( !needaux )
    {
	if ( !hinfofetched )
	    strm_.seekg( auxnrbytes, ios::cur );
	return true;
    }
    else if ( hinfofetched )
	strm_.seekg( -auxnrbytes, ios::cur );

    char buf[2*sizeof(double)];
    mCondGetAux(startpos)
    mCondGetCoordAux()
    mCondGetAux(offset)
    mCondGetAux(pick)
    mCondGetAux(refpos)
    mCondGetAux(azimuth)

    hinfofetched = true;
    return strm_.good();
}


bool CBVSReader::fetch( void** bufs, const bool* comps,
			const Interval<int>* samps, int offs )
{
    if ( !hinfofetched && auxnrbytes )
    {
	static PosAuxInfo dum;
	if ( !getAuxInfo(dum) ) return false;
    }

    if ( !samps ) samps = &samprg;

    int iselc = -1;
    for ( int icomp=0; icomp<nrcomps_; icomp++ )
    {
	if ( comps && !comps[icomp] )
	{
	    strm_.seekg( cnrbytes_[icomp], ios::cur );
	    continue;
	}
	iselc++;

	BasicComponentInfo* compinfo = info_.compinfo[icomp];
	int bps = compinfo->datachar.nrBytes();
	if ( samps->start )
	    strm_.seekg( samps->start*bps, ios::cur );
	strm_.read( (char*)bufs[iselc] + offs * bps,
		(samps->stop-samps->start+1) * bps );
	if ( samps->stop < compinfo->nrsamples-1 )
	    strm_.seekg( (compinfo->nrsamples-samps->stop-1)*bps,
		ios::cur );
    }

    hinfofetched = false;
    return strm_.good() || strm_.eof();
}
