/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : A.H. Bril
 * DATE     : Mar 2001
 * FUNCTION : CBVS I/O
-*/

static const char* rcsID = "$Id: cbvsreader.cc,v 1.82 2010-12-29 15:30:03 cvskris Exp $";

/*!

The CBVS header starts with the following bytes:
0: 'd'
1: 'G'
2: 'B'
3: 0 = big endian(Sun,SGI), 1 = little endian (Win,Lin,Mac)
4: version of CBVS.
5: info on "aux pos info"
6: coordinate storage policy
7: not used (yet)

The next 8 bytes are reserved for 2 integers:
8-11: total nr bytes in the CBVS header
12-15: not used (yet) here, 12 is used for aux flags in aux file.

*/

#include "cbvsreader.h"
#include "datainterp.h"
#include "survinfo.h"
#include "envvars.h"
#include "errh.h"
#include "ptrman.h"
#include "varlenarray.h"
#include "strmoper.h"


CBVSReader::CBVSReader( std::istream* s, bool glob_info_only, 
				bool forceusecbvsinfo )
	: strm_(*s)
	, iinterp(DataCharacteristics())
	, finterp(DataCharacteristics())
	, dinterp(DataCharacteristics())
	, bytespertrace(0)
	, hinfofetched(false)
	, posidx(0)
	, datastartfo(0)
	, lastposfo(0)
	, hs(false)
    	, needaux(true)
{
    hs.step.inl = hs.step.crl = 1;
    if ( readInfo(!glob_info_only,forceusecbvsinfo) )
	toStart();
}


CBVSReader::~CBVSReader()
{
    close();
}


void CBVSReader::close()
{
    if ( !strmclosed_ && &strm_ != &std::cin )
	delete &strm_;
    strmclosed_ = true;
}


#define mErrRet(s) { errmsg_ = s; return 0; }

bool CBVSReader::readInfo( bool wanttrailer, bool forceusecbvsinfo )
{
    info_.clean();
    errmsg_ = check( strm_ );
    if ( errmsg_ ) return false;

    BufferString buf( headstartbytes, false );
    char* ptr = buf.buf();
    strm_.read( ptr, headstartbytes );

    DataCharacteristics dc;
    dc.littleendian_ = ptr[3] != 0;
    finterp.set( dc );
    dc.setNrBytes( BinDataDesc::N8 );
    dinterp.set( dc );
    dc.BinDataDesc::set( true, true, BinDataDesc::N4 );
    iinterp.set( dc );

    // const int version = (int)ptr[4];
    coordpol_ = (CoordPol)ptr[6]; // Must be got before getAuxInfoSel() called!
    getAuxInfoSel( ptr + 5 );
    // const int nrbytesinheader = iinterp.get( ptr+8 );

    strm_.read( ptr, integersize );
    getText( iinterp.get(ptr,0), info_.stdtext );

    if ( !readComps() || !readGeom(forceusecbvsinfo) )
	return false;

    strm_.read( ptr, 2 * integersize );
    info_.seqnr = iinterp.get( ptr, 0 );
    getText( iinterp.get(ptr,1), info_.usertext );
    removeTrailingBlanks( info_.usertext.buf() );

    datastartfo = strm_.tellg();

    CBVSInfo::SurvGeom& geom = info_.geom;
    if ( !wanttrailer )
	geom.fullyrectandreg = true;
    bool needtrailer = !geom.fullyrectandreg || coordpol_ == InTrailer;
    if ( wanttrailer && needtrailer && !readTrailer() )
	return false;

    if ( wanttrailer )
	geom.reCalcBounds();

    firstbinid = geom.start; lastbinid = geom.stop;
    if ( geom.fullyrectandreg || !wanttrailer )
	nrxlines_ = (lastbinid.crl - firstbinid.crl) / geom.step.crl + 1;
    else
    {
	firstbinid.inl = firstbinid.crl = mUdf(int);
	lastbinid.inl = lastbinid.crl = -mUdf(int);
	for ( int idx=0; idx<geom.cubedata.size(); idx++ )
	{
	    PosInfo::LineData* iinf = geom.cubedata[idx];
	    if ( iinf->linenr_ > lastbinid.inl )
	    {
		lastbinid.inl = iinf->linenr_;
		lastbinid.crl = iinf->segments_[iinf->segments_.size()-1].stop;
	    }
	    if ( iinf->linenr_ < firstbinid.inl )
	    {
		firstbinid.inl = iinf->linenr_;
		firstbinid.crl = iinf->segments_[0].start;
	    }
	}
	curinlinfnr_ = geom.findNextInfIdx( -1 );
	cursegnr_ = 0;
    }

    curbinid_ = firstbinid;
    if ( wanttrailer )
	mkPosNrs();

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
#define mErrRet { strm.seekg( 0, std::ios::beg ); return msg; }

const char* CBVSReader::check( std::istream& strm )
{
    if ( strm.bad() ) return "Input stream cannot be used";
    if ( strm.fail() ) strm.clear();

    strm.seekg( 0, std::ios::beg );
    char buf[4]; memset( buf, 0, 4 );
    strm.read( buf, 3 );
    const char* msg = "Input stream cannot be used";
    if ( !strm.good() ) mErrRet;

    msg = "File is not in CBVS format";
    if ( strcmp(buf,"dGB") ) mErrRet;

    char plf; strm.read( &plf, 1 );
    if ( plf > 2 ) mErrRet;

    strm.seekg( 0, std::ios::beg );
    return 0;
}


void CBVSReader::getAuxInfoSel( const char* ptr )
{
    info_.auxinfosel.startpos =	mAuxSetting(ptr,1);
    info_.auxinfosel.coord =	mAuxSetting(ptr,2);
    info_.auxinfosel.offset =	mAuxSetting(ptr,4);
    info_.auxinfosel.pick =	mAuxSetting(ptr,8);
    info_.auxinfosel.refnr =	mAuxSetting(ptr,16);
    info_.auxinfosel.azimuth =	mAuxSetting(ptr,32);

#define mAddBytes(memb,t) \
    if ( info_.auxinfosel.memb ) auxnrbytes += sizeof(t)
    auxnrbytes = 0;
    mAddBytes(startpos,float);
    if ( coordpol_ == InAux )
    {
	mAddBytes(coord,double); // x
	mAddBytes(coord,double); // y
    }
    mAddBytes(offset,float);
    mAddBytes(pick,float);
    mAddBytes(refnr,float);
    mAddBytes(azimuth,float);
}


#undef mErrRet
#define mErrRet(s) { toOffs(0); errmsg_ = s; return false; }


bool CBVSReader::readComps()
{
    mAllocVarLenArr( char, ucbuf, 4*integersize );
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
	    newinf->datachar.fmt_ = DataCharacteristics::Ieee;
	strm_.read( ucbuf, sizeof(float) );
	    info_.sd.start = finterp.get( ucbuf, 0 );
	strm_.read( ucbuf, sizeof(float) );
	    info_.sd.step = finterp.get( ucbuf, 0 );
	int nrsamples;
	strm_.read( ucbuf, integersize ); // nr samples
	    info_.nrsamples = iinterp.get( ucbuf, 0 );
	strm_.read( ucbuf, 2*sizeof(float) );
	    // reserved for per-component scaling: LinScaler( a, b )

	if ( info_.nrsamples < 0 || newinf->datatype < 0 )
	{
	    delete newinf;
	    mErrRet("Corrupt CBVS format: Component desciption error")
	}

	info_.compinfo += newinf;

	cnrbytes_[icomp] = info_.nrsamples * newinf->datachar.nrBytes();
	bytespertrace += cnrbytes_[icomp];
	samprg = Interval<int>( 0, info_.nrsamples-1 );
    }

    return true;
}


bool CBVSReader::readGeom( bool forceusecbvsinfo )
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
    if ( info_.geom.step.inl == 0 ) info_.geom.step.inl = 1;
    info_.geom.step.crl = iinterp.get( buf, 7 );
    if ( info_.geom.step.crl == 0 ) info_.geom.step.crl = 1;

    strm_.read( buf, 6*sizeof(double) );
    RCol2Coord::RCTransform xtr, ytr;
    xtr.a = dinterp.get( buf, 0 ); xtr.b = dinterp.get( buf, 1 );
    xtr.c = dinterp.get( buf, 2 ); ytr.a = dinterp.get( buf, 3 );
    ytr.b = dinterp.get( buf, 4 ); ytr.c = dinterp.get( buf, 5 );
    static const bool useinfvar = GetEnvVarYN("DTECT_CBVS_USE_STORED_SURVINFO");
    const bool useinfo = forceusecbvsinfo ? true : useinfvar;
    if ( useinfo && xtr.valid(ytr) )
	info_.geom.b2c.setTransforms( xtr, ytr );
    else
	info_.geom.b2c = SI().binID2Coord();

    hs.start = hs.stop = BinID( info_.geom.start.inl, info_.geom.start.crl );
    hs.include( BinID( info_.geom.stop.inl, info_.geom.stop.crl ) );

    return strm_.good();
}


bool CBVSReader::readTrailer()
{
    StrmOper::seek( strm_, -3, std::ios::end );
    char buf[40];
    strm_.read( buf, 3 ); buf[3] = '\0';
    if ( strcmp(buf,"BGd") ) mErrRet("Missing required file trailer")
    
    StrmOper::seek( strm_,-4-integersize, std::ios::end );
    strm_.read( buf, integersize );
    const int nrbytes = iinterp.get( buf, 0 );

    StrmOper::seek( strm_, -4-integersize-nrbytes, std::ios::end );
    if ( coordpol_ == InTrailer )
    {
	strm_.read( buf, integersize );
	const int sz = iinterp.get( buf, 0 );
	for ( int idx=0; idx<sz; idx++ )
	{
	    strm_.read( buf, 2 * sizeof(double) );
	    trailercoords_ += Coord( dinterp.get(buf,0), dinterp.get(buf,1) );
	}
    }

    if ( !info_.geom.fullyrectandreg )
    {
	strm_.read( buf, integersize );
	const int nrinl = iinterp.get( buf, 0 );
	if ( nrinl < 0 ) mErrRet("File trailer corrupt")
	if ( nrinl == 0 ) mErrRet("No traces in file")

	for ( int iinl=0; iinl<nrinl; iinl++ )
	{
	    strm_.read( buf, 2 * integersize );
	    PosInfo::LineData* iinf
		= new PosInfo::LineData( iinterp.get( buf, 0 ) );
	    if ( !iinl )
		hs.start.inl = hs.stop.inl = iinf->linenr_;

	    const int nrseg = iinterp.get( buf, 1 );
	    PosInfo::LineData::Segment crls;
	    for ( int iseg=0; iseg<nrseg; iseg++ )
	    {
		strm_.read( buf, 3 * integersize );

		crls.start = iinterp.get(buf,0);
		crls.stop = iinterp.get(buf,1);
		crls.step = iinterp.get(buf,2);
		iinf->segments_ += crls;

		if ( !iinl && !iseg )
		    hs.start.crl = hs.stop.crl = crls.start;
		else
		    hs.include( BinID(iinf->linenr_,crls.start) );
		hs.include( BinID(iinf->linenr_,crls.stop) );
	    }
	    info_.geom.cubedata.add( iinf );
	}

	info_.geom.start = hs.start;
	info_.geom.stop = hs.stop;

	curinlinfnr_ = cursegnr_ = 0;
	curbinid_.inl = info_.geom.cubedata[curinlinfnr_]->linenr_;
	curbinid_.crl = info_.geom.cubedata[curinlinfnr_]
			->segments_[cursegnr_].start;
    }

    return strm_.good();
}


bool CBVSReader::toStart()
{
    if ( strmclosed_ ) return false;

    goTo( firstbinid );
    return true;
}


void CBVSReader::toOffs( od_int64 sp )
{
    lastposfo = sp;
    StrmOper::seek( strm_, lastposfo, std::ios::beg );
}


bool CBVSReader::goTo( const BinID& bid, bool nearestok )
{
    if ( strmclosed_ ) return false;

    const int posnr = getPosNr( bid, nearestok, true );
    return goToPosNrOffs( posnr );
}


void CBVSReader::setPos( int posnr, const BinID& bid, int iinlinf, int iseg )
{
    curbinid_ = bid;
    curinlinfnr_ = iinlinf; cursegnr_ = iseg;
    goToPosNrOffs( posnr );
}


bool CBVSReader::goToPosNrOffs( int posnr )
{
    if ( posnr < 0 ) return false;

    // Be careful: offsets can be larger than what fits in an int!
#ifdef __win32__
    od_int64 so;
#else
    std::streamoff so;
#endif
    so = posnr * (info_.nrtrcsperposn < 2
	    	      ? 1 : info_.nrtrcsperposn);
    so *= auxnrbytes + bytespertrace;

#ifdef __win32__
    toOffs( datastartfo + so );
#else
    toOffs( datastartfo + std::streampos(so) );
#endif

    hinfofetched = false;
    posidx = 0;
    return true;
}


int CBVSReader::getPosNr( const BinID& bid, bool nearestok,
			  bool setcurrent ) const
{
    int posnr = -1;
    BinID nearestbinid( curbinid_ );
    int inlinfnr = curinlinfnr_;
    int segnr = cursegnr_;

    if ( info_.geom.fullyrectandreg )
    {
	if ( !hs.includes(bid) )
	    return -1;
	const int inlstep = abs(info_.geom.step.inl);
	if ( inlstep == 1 )
	    nearestbinid.inl = bid.inl;
	else
	{
	    StepInterval<int> inls( firstbinid.inl, lastbinid.inl, inlstep );
	    nearestbinid.inl = inls.atIndex( inls.nearestIndex( bid.inl ) );
	}
	if ( info_.geom.step.crl == 1 ) // nowadays, always crossline-sorted
	    nearestbinid.crl = bid.crl;
	else
	{
	    StepInterval<int> crls( firstbinid.crl, lastbinid.crl,
				    info_.geom.step.crl );
	    nearestbinid.crl = crls.atIndex( crls.nearestIndex( bid.crl ) );
	}
	int inldiff = info_.geom.step.inl < 0 ? lastbinid.inl-nearestbinid.inl
	    				      : nearestbinid.inl-firstbinid.inl;
	posnr = (inldiff / inlstep) * nrxlines_
	      + ((nearestbinid.crl-firstbinid.crl) / info_.geom.step.crl);
    }
    else
    {
	const PosInfo::LineData* iinf = info_.geom.cubedata[inlinfnr];
	posnr = posnrs[inlinfnr];

	// Optimisation: Still on right inline?
	if ( bid.inl != iinf->linenr_ )
	{
	    // Nope. We need to search.
	    const int sz = info_.geom.cubedata.size();
	    for ( int iinl=0; iinl<sz; iinl++ )
	    {
		iinf = info_.geom.cubedata[iinl];
		if ( iinf->linenr_ == bid.inl )
		{
		    inlinfnr = iinl;
		    posnr = posnrs[iinl];
		    break;
		}
	    }
	    if ( iinf->linenr_ != bid.inl )
		return -1;
	}

	// Now we know we have the right inline, find segment:
	const PosInfo::LineData::Segment* seg = 0;
	for ( int iseg=0; iseg<iinf->segments_.size(); iseg++ )
	{
	    const PosInfo::LineData::Segment& curseg = iinf->segments_[iseg];
	    if ( !curseg.includes(bid.crl) )
		posnr += curseg.nrSteps() + 1;
	    else
	    {
		segnr = iseg;
		seg = &curseg;
		break;
	    }
	}
	if ( !seg ) return -1;

	int segposn = seg->nearestIndex( bid.crl );
	posnr += segposn;
	nearestbinid.inl = iinf->linenr_;
	nearestbinid.crl = seg->atIndex( segposn );
    }

    const int ret = nearestok || bid == nearestbinid ? posnr : -1;
    if ( ret >= 0 && setcurrent )
    {
	curbinid_ = nearestbinid;
	curinlinfnr_ = inlinfnr; cursegnr_ = segnr;
    }
    return ret;
}


void CBVSReader::mkPosNrs()
{
    posnrs.erase(); posnrs += 0;

    const int sz = info_.geom.cubedata.size();
    int posnr = 0;
    for ( int iinl=0; iinl<sz; iinl++ )
    {
	const PosInfo::LineData& iinf = *info_.geom.cubedata[iinl];

	for ( int iseg=0; iseg<iinf.segments_.size(); iseg++ )
	    posnr += iinf.segments_[iseg].nrSteps() + 1;

	posnrs += posnr;
    }
}


int CBVSReader::nextPosIdx()
{
    posidx++;
    if ( posidx >= info_.nrtrcsperposn )
	posidx = 0;

    if ( posidx == 0 )
	return getNextBinID( curbinid_, curinlinfnr_, cursegnr_ );

    return 1; // info_.nrtrcsperposn > 1 and we're returning 2nd or more
}


bool CBVSReader::toNext()
{
    hinfofetched = false;
    int res = nextPosIdx();
    if ( res == 0 )
	return false;
    else if ( res == 2 )
	return goTo( curbinid_ );

    // OK - just need to go to the next trace
    const std::streampos onetrcoffs = auxnrbytes + bytespertrace;
    std::streampos posadd = onetrcoffs;
    toOffs( lastposfo + posadd );
    return true;
}


BinID CBVSReader::nextBinID() const
{
    BinID bid( curbinid_ ); int ci = curinlinfnr_, cc = cursegnr_;
    if ( getNextBinID(bid,ci,cc) == 0 )
	bid.inl = bid.crl = 0;
    return bid;
}


#define mRetNoMore \
{ \
    bid = firstbinid; segnr = 0; \
    inlinfnr = info_.geom.findNextInfIdx(-1); \
    return 0; \
}


int CBVSReader::getNextBinID( BinID& bid, int& inlinfnr, int& segnr ) const
{
    if ( bid == lastbinid )
	mRetNoMore

    if ( info_.geom.fullyrectandreg )
    {
	bid.crl += info_.geom.step.crl;
	if ( hs.includes(bid) )
	    return 1;

	bid.crl = firstbinid.crl;
	bid.inl += abs(info_.geom.step.inl);
	if ( !hs.includes(bid) )
	    { bid.inl = hs.start.inl; return 0; }
	return info_.geom.step.inl < 0 ? 2 : 1;
    }

    const PosInfo::LineData* inlinf = info_.geom.cubedata[inlinfnr];
    const PosInfo::LineData::Segment* curseg = &inlinf->segments_[segnr];
    bid.crl += curseg->step;
    if ( curseg->includes(bid.crl) )
	return 1;

    if ( curseg->step > 0 && bid.crl < curseg->start )
	bid.crl = curseg->start; // So the crl wasn't in the seg before ...
    else if ( curseg->step < 0 && bid.crl > curseg->start )
	bid.crl = curseg->stop; // ... not likely, defensive programming
    else
    {
	if ( segnr < inlinf->segments_.size()-1 )
	{
	    segnr++;
	    bid.crl = inlinf->segments_[segnr].start;
	    return 2;
	}

	segnr = 0;
	inlinfnr = info_.geom.findNextInfIdx( inlinfnr );
	if ( inlinfnr < 0 )
	    mRetNoMore

	inlinf = info_.geom.cubedata[inlinfnr];
	curseg = &inlinf->segments_[segnr];
	bid.inl = inlinf->linenr_;
	bid.crl = curseg->start;
    }

    return 2;
}


#define mCondGetAux(memb) \
    if ( info_.auxinfosel.memb ) \
	{ mGetAuxFromStrm(auxinf,buf,memb,strm_); }

bool CBVSReader::getAuxInfo( PosAuxInfo& auxinf )
{
    if ( strmclosed_ )
	return true;
#ifdef __debug__
    // gdb says: "Couldn't find method ostream::tellp"
    std::streampos curfo = strm_.tellg();
#endif

    auxinf.binid = curbinid_;
    auxinf.coord = info_.geom.b2c.transform( curbinid_ );
    auxinf.startpos = info_.sd.start;
    auxinf.offset = auxinf.azimuth = 0;
    auxinf.pick = mSetUdf(auxinf.refnr);

    if ( !auxnrbytes )
	return true;

    if ( !needaux )
    {
	if ( !hinfofetched )
	    StrmOper::seek( strm_, auxnrbytes, std::ios::cur );
	return true;
    }
    else if ( hinfofetched )
	StrmOper::seek( strm_,-auxnrbytes, std::ios::cur );

    char buf[2*sizeof(double)];
    mCondGetAux(startpos)
    if ( coordpol_ == InAux && info_.auxinfosel.coord )
	{ mGetCoordAuxFromStrm(auxinf,buf,strm_); }
    else if ( coordpol_ == InTrailer )
	auxinf.coord = getTrailerCoord( auxinf.binid );
    mCondGetAux(offset)
    mCondGetAux(pick)
    mCondGetAux(refnr)
    mCondGetAux(azimuth)

    hinfofetched = true;
    return strm_.good();
}


Coord CBVSReader::getTrailerCoord( const BinID& bid ) const
{
    int arridx = 0;
    if ( info_.geom.fullyrectandreg )
    {
	if ( bid.inl != info_.geom.start.inl )
	{
	    const int nrcrl = (info_.geom.stop.crl-info_.geom.start.crl)
			    / info_.geom.step.crl + 1;
	    arridx = nrcrl * (bid.inl - info_.geom.start.inl);
	}
	arridx += (bid.crl-info_.geom.start.crl) / info_.geom.step.crl;
    }
    else
    {
	for ( int iinl=0; iinl<info_.geom.cubedata.size(); iinl++ )
	{
	    const PosInfo::LineData& inlinf = *info_.geom.cubedata[iinl];
	    bool inlmatches = inlinf.linenr_ == bid.inl;
	    for ( int icrl=0; icrl<inlinf.segments_.size(); icrl++ )
	    {
		const StepInterval<int>& seg = inlinf.segments_[icrl];
		if ( !inlmatches || !seg.includes(bid.crl,false) )
		    arridx += seg.nrSteps() + 1;
		else
		{
		    arridx += (bid.crl - seg.start) / seg.step;
		    break;
		}
	    }
	    if ( inlmatches ) break;
	}
    }

    if ( arridx < trailercoords_.size() )
	return trailercoords_[arridx];

    return info_.geom.b2c.transform( bid );
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
	    StrmOper::seek( strm_, cnrbytes_[icomp], std::ios::cur );
	    continue;
	}
	iselc++;

	BasicComponentInfo* compinfo = info_.compinfo[icomp];
	int bps = compinfo->datachar.nrBytes();
	if ( samps->start )
	    StrmOper::seek( strm_, samps->start*bps, std::ios::cur );
	if ( !StrmOper::readBlock( strm_, ((char*)bufs[iselc]) + offs*bps,
		    		   (samps->stop-samps->start+1) * bps ) )
	    break;

	if ( samps->stop < info_.nrsamples-1 )
	    StrmOper::seek( strm_, (info_.nrsamples-samps->stop-1)*bps,
		    std::ios::cur );
    }

    hinfofetched = false;
    return strm_.good() || strm_.eof();
}
