/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : A.H. Bril
 * DATE     : Mar 2001
 * FUNCTION : CBVS I/O
-*/

static const char* rcsID mUnusedVar = "$Id$";

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
12-15: not used

*/

#include "cbvsreader.h"
#include "datainterp.h"
#include "survinfo.h"
#include "envvars.h"
#include "errh.h"
#include "ptrman.h"
#include "varlenarray.h"
#include "strmoper.h"
#include "posinfo.h"


#define mGetAuxFromStrm(auxinf,buf,memb,strm) \
    strm.read( buf, sizeof(auxinf.memb) ); \
    auxinf.memb = finterp_.get( buf, 0 )

#define mGetCoordAuxFromStrm(auxinf,buf,strm) \
    strm.read( buf, 2*sizeof(auxinf.coord.x) ); \
    auxinf.coord.x = dinterp_.get( buf, 0 ); \
    auxinf.coord.y = dinterp_.get( buf, 1 )

#define mAuxSetting(ptr,n) (*ptr & (unsigned char)n)



CBVSReader::CBVSReader( std::istream* s, bool glob_info_only, 
				bool forceusecbvsinfo )
	: strm_(*s)
	, iinterp_(DataCharacteristics())
	, finterp_(DataCharacteristics())
	, dinterp_(DataCharacteristics())
	, bytespertrace_(0)
	, hinfofetched_(false)
	, idxatpos_(0)
	, datastartfo_(0)
	, lastposfo_(0)
	, hs_(false)
{
    hs_.step.inl = hs_.step.crl = 1;
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
    finterp_.set( dc );
    dc.setNrBytes( BinDataDesc::N8 );
    dinterp_.set( dc );
    dc.BinDataDesc::set( true, true, BinDataDesc::N4 );
    iinterp_.set( dc );

    // const int version = (int)ptr[4];
    coordpol_ = (CoordPol)ptr[6]; // Must be got before getAuxInfoSel() called!
    getAuxInfoSel( ptr + 5 );
    // const int nrbytesinheader = iinterp_.get( ptr+8 );

    strm_.read( ptr, integersize );
    getText( iinterp_.get(ptr,0), info_.stdtext_ );

    if ( !readComps() || !readGeom(forceusecbvsinfo) )
	return false;

    strm_.read( ptr, 2 * integersize );
    info_.seqnr_ = iinterp_.get( ptr, 0 );
    getText( iinterp_.get(ptr,1), info_.usertext_ );
    removeTrailingBlanks( info_.usertext_.buf() );

    datastartfo_ = strm_.tellg();

    CBVSInfo::SurvGeom& geom = info_.geom_;
    if ( !wanttrailer )
	geom.fullyrectandreg = true;

    bool needtrailer = !geom.fullyrectandreg || coordpol_ == InTrailer;
    if ( wanttrailer && needtrailer && !readTrailer() )
	return false;

    if ( geom.fullyrectandreg )
    {
	geom.cubedata.erase();
	geom.cubedata.generate( geom.start, geom.stop, geom.step );
	lds_.generate( geom.start, geom.stop, geom.step );
    }
    geom.reCalcBounds();

    firstbinid_ = geom.start;
    curgeomcubepos_.toStart();
    setCubePos( true );
    if ( !geom.cubedata.isValid(curgeomcubepos_) )
	return true; // empty file
    updCurBinID();
    firstbinid_ = geom.cubedata.binID( curgeomcubepos_ );

    mkPosNrs();
    return true;
}


void CBVSReader::setCubePos( bool fromgeom ) const
{
    PosInfo::CubeDataPos& inp = fromgeom ? curgeomcubepos_ : curldscubepos_;
    PosInfo::CubeDataPos& out = fromgeom ? curldscubepos_ : curgeomcubepos_;
    const PosInfo::CubeData& inpcd = fromgeom ? info_.geom_.cubedata : lds_;
    const PosInfo::CubeData& outcd = fromgeom ? lds_ : info_.geom_.cubedata;

    out = inp; // guess
    if ( outcd.binID(out) != inpcd.binID(inp) )
	out = outcd.cubeDataPos( inpcd.binID(inp) );
}


void CBVSReader::updCurBinID() const
{
    curbinid_ = info_.geom_.cubedata.binID( curgeomcubepos_ );
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
    info_.auxinfosel_.startpos =	mAuxSetting(ptr,1);
    info_.auxinfosel_.coord =	mAuxSetting(ptr,2);
    info_.auxinfosel_.offset =	mAuxSetting(ptr,4);
    info_.auxinfosel_.pick =	mAuxSetting(ptr,8);
    info_.auxinfosel_.refnr =	mAuxSetting(ptr,16);
    info_.auxinfosel_.azimuth =	mAuxSetting(ptr,32);

#define mAddBytes(memb,t) \
    if ( info_.auxinfosel_.memb ) auxnrbytes_ += sizeof(t)
    auxnrbytes_ = 0;
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
    nrcomps_ = iinterp_.get( ucbuf, 0 );
    if ( nrcomps_ < 1 ) mErrRet("Corrupt CBVS format: No components defined")

    cnrbytes_ = new int [nrcomps_];
    bytespertrace_ = 0;

    for ( int icomp=0; icomp<nrcomps_; icomp++ )
    {
	strm_.read( ucbuf, integersize );
	BufferString bs; getText( iinterp_.get(ucbuf,0), bs );
	BasicComponentInfo* newinf = new BasicComponentInfo( (const char*)bs );
	
	strm_.read( ucbuf, integersize );
	    newinf->datatype = iinterp_.get( ucbuf, 0 );
	strm_.read( ucbuf, 4 );
	    newinf->datachar.set( ucbuf[0], ucbuf[1] );
	    // extra 2 bytes reserved for compression type
	    newinf->datachar.fmt_ = DataCharacteristics::Ieee;
	strm_.read( ucbuf, sizeof(float) );
	    info_.sd_.start = finterp_.get( ucbuf, 0 );
	strm_.read( ucbuf, sizeof(float) );
	    info_.sd_.step = finterp_.get( ucbuf, 0 );

	strm_.read( ucbuf, integersize ); // nr samples
	    info_.nrsamples_ = iinterp_.get( ucbuf, 0 );
	strm_.read( ucbuf, 2*sizeof(float) );
	    // reserved for per-component scaling: LinScaler( a, b )

	if ( info_.nrsamples_ < 0 || newinf->datatype < 0 )
	{
	    delete newinf;
	    mErrRet("Corrupt CBVS format: Component desciption error")
	}

	info_.compinfo_ += newinf;

	cnrbytes_[icomp] = info_.nrsamples_ * newinf->datachar.nrBytes();
	bytespertrace_ += cnrbytes_[icomp];
	samprg_ = Interval<int>( 0, info_.nrsamples_-1 );
    }

    return true;
}


bool CBVSReader::readGeom( bool forceusecbvsinfo )
{
    char buf[8*sizeof(double)];

    strm_.read( buf, 8*integersize );
    info_.geom_.fullyrectandreg = (bool)iinterp_.get( buf, 0 );
    info_.nrtrcsperposn_ = iinterp_.get( buf, 1 );
    info_.geom_.start.inl = iinterp_.get( buf, 2 );
    info_.geom_.start.crl = iinterp_.get( buf, 3 );
    info_.geom_.stop.inl = iinterp_.get( buf, 4 );
    info_.geom_.stop.crl = iinterp_.get( buf, 5 );
    info_.geom_.step.inl = iinterp_.get( buf, 6 );
    if ( info_.geom_.step.inl == 0 ) info_.geom_.step.inl = 1;
    info_.geom_.step.crl = iinterp_.get( buf, 7 );
    if ( info_.geom_.step.crl == 0 ) info_.geom_.step.crl = 1;

    strm_.read( buf, 6*sizeof(double) );
    RCol2Coord::RCTransform xtr, ytr;
    xtr.a = dinterp_.get( buf, 0 ); xtr.b = dinterp_.get( buf, 1 );
    xtr.c = dinterp_.get( buf, 2 ); ytr.a = dinterp_.get( buf, 3 );
    ytr.b = dinterp_.get( buf, 4 ); ytr.c = dinterp_.get( buf, 5 );
    static const bool useinfvar = GetEnvVarYN("DTECT_CBVS_USE_STORED_SURVINFO");
    const bool useinfo = forceusecbvsinfo ? true : useinfvar;
    if ( useinfo && xtr.valid(ytr) )
	info_.geom_.b2c.setTransforms( xtr, ytr );
    else
	info_.geom_.b2c = SI().binID2Coord();

    hs_.start = hs_.stop = BinID( info_.geom_.start.inl, info_.geom_.start.crl );
    hs_.include( BinID( info_.geom_.stop.inl, info_.geom_.stop.crl ) );

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
    const int nrbytes = iinterp_.get( buf, 0 );

    StrmOper::seek( strm_, -4-integersize-nrbytes, std::ios::end );
    if ( coordpol_ == InTrailer )
    {
	strm_.read( buf, integersize );
	const int sz = iinterp_.get( buf, 0 );
	for ( int idx=0; idx<sz; idx++ )
	{
	    strm_.read( buf, 2 * sizeof(double) );
	    trailercoords_ += Coord( dinterp_.get(buf,0), dinterp_.get(buf,1) );
	}
    }

    if ( !info_.geom_.fullyrectandreg )
    {
	strm_.read( buf, integersize );
	const int nrinl = iinterp_.get( buf, 0 );
	if ( nrinl < 0 ) mErrRet("File trailer corrupt")
	if ( nrinl == 0 ) mErrRet("No traces in file")

	for ( int iinl=0; iinl<nrinl; iinl++ )
	{
	    strm_.read( buf, 2 * integersize );
	    PosInfo::LineData* iinf
		= new PosInfo::LineData( iinterp_.get( buf, 0 ) );
	    if ( !iinl )
		hs_.start.inl = hs_.stop.inl = iinf->linenr_;

	    const int nrseg = iinterp_.get( buf, 1 );
	    PosInfo::LineData::Segment crls;
	    for ( int iseg=0; iseg<nrseg; iseg++ )
	    {
		strm_.read( buf, 3 * integersize );

		crls.start = iinterp_.get(buf,0);
		crls.stop = iinterp_.get(buf,1);
		crls.step = iinterp_.get(buf,2);
		iinf->segments_ += crls;

		if ( !iinl && !iseg )
		    hs_.start.crl = hs_.stop.crl = crls.start;
		else
		    hs_.include( BinID(iinf->linenr_,crls.start) );
		hs_.include( BinID(iinf->linenr_,crls.stop) );
	    }
	    lds_ += iinf;
	}

	info_.geom_.start = hs_.start;
	info_.geom_.stop = hs_.stop;
    }

    info_.geom_.fullyrectandreg = lds_.isFullyRectAndReg();;
    if ( !info_.geom_.fullyrectandreg )
	info_.geom_.cubedata = lds_;
    return strm_.good();
}


bool CBVSReader::toStart()
{
    if ( strmclosed_ ) return false;

    goTo( firstbinid_ );
    return true;
}


BinID CBVSReader::nextBinID() const
{
    PosInfo::CubeDataPos cdp( curgeomcubepos_ );
    info_.geom_.cubedata.toNext( cdp );
    return info_.geom_.cubedata.binID( cdp );
}


void CBVSReader::toOffs( od_int64 sp )
{
    lastposfo_ = sp;
    StrmOper::seek( strm_, lastposfo_, std::ios::beg );
}


bool CBVSReader::goTo( const BinID& bid )
{
    if ( strmclosed_ ) return false;
    PosInfo::CubeDataPos cdp = lds_.cubeDataPos( bid );
    if ( !cdp.isValid() )
	return false;

    const int posnr = getPosNr( cdp, true );
    if ( posnr < 0 ) return false;

    // Be careful: offsets can be larger than what fits in an int!
#ifdef __win32__
    od_int64 so;
#else
    std::streamoff so;
#endif
    so = posnr * (info_.nrtrcsperposn_ < 2
	    	      ? 1 : info_.nrtrcsperposn_);
    so *= auxnrbytes_ + bytespertrace_;

#ifdef __win32__
    toOffs( datastartfo_ + so );
#else
    toOffs( datastartfo_ + std::streampos(so) );
#endif

    hinfofetched_ = false;
    idxatpos_ = 0;
    return true;
}


int CBVSReader::getPosNr( const PosInfo::CubeDataPos& cdp,
			  bool setcurrent ) const
{
    int posnr = -1;
    const BinID reqbid( lds_.binID(cdp) );

    if ( lds_.isEmpty() )
	return posnr;

    posnr = posnrs_[cdp.lidx_];

    const PosInfo::LineData& iinf = *lds_[cdp.lidx_];
    for ( int iseg=0; iseg<cdp.segnr_; iseg++ )
	posnr += iinf.segments_[iseg].nrSteps() + 1;

    posnr += cdp.sidx_;

    if ( setcurrent )
    {
	curldscubepos_ = cdp;
	setCubePos( false );
	updCurBinID();
    }

    return posnr;
}


void CBVSReader::mkPosNrs()
{
    posnrs_.erase(); posnrs_ += 0;

    const int sz = lds_.size();
    int posnr = 0;
    for ( int iinl=0; iinl<sz; iinl++ )
    {
	const PosInfo::LineData& iinf = *lds_[iinl];

	for ( int iseg=0; iseg<iinf.segments_.size(); iseg++ )
	    posnr += iinf.segments_[iseg].nrSteps() + 1;

	posnrs_ += posnr;
    }
}


bool CBVSReader::toNext()
{
    hinfofetched_ = false;

    idxatpos_++;
    if ( idxatpos_ >= info_.nrtrcsperposn_ )
	idxatpos_ = 0;
    if ( idxatpos_ == 0 )
    {
	if ( !info_.geom_.cubedata.toNext(curgeomcubepos_) )
	    return false;
	setCubePos( true );
	updCurBinID();
    }

    const std::streampos onetrcoffs = auxnrbytes_ + bytespertrace_;
    std::streampos posadd = onetrcoffs;
    toOffs( lastposfo_ + posadd );
    return true;
}


#define mCondGetAux(memb) \
    if ( info_.auxinfosel_.memb ) \
	{ mGetAuxFromStrm(auxinf,buf,memb,strm_); }

bool CBVSReader::getAuxInfo( PosAuxInfo& auxinf )
{
    if ( strmclosed_ )
	return true;
#ifdef __debug__
    // gdb says: "Couldn't find method ostream::tellp"
    std::streampos curfo mUnusedVar = strm_.tellg();
#endif

    auxinf.binid = curbinid_;
    auxinf.coord = info_.geom_.b2c.transform( curbinid_ );
    auxinf.startpos = info_.sd_.start;
    auxinf.offset = auxinf.azimuth = 0;
    auxinf.pick = mSetUdf(auxinf.refnr);

    if ( auxnrbytes_ < 1 )
	return true;

    if ( hinfofetched_ )
	StrmOper::seek( strm_,-auxnrbytes_, std::ios::cur );

    char buf[2*sizeof(double)];
    mCondGetAux(startpos)
    if ( coordpol_ == InAux && info_.auxinfosel_.coord )
	{ mGetCoordAuxFromStrm(auxinf,buf,strm_); }
    else if ( coordpol_ == InTrailer )
	auxinf.coord = getTrailerCoord( auxinf.binid );
    mCondGetAux(offset)
    mCondGetAux(pick)
    mCondGetAux(refnr)
    mCondGetAux(azimuth)

    hinfofetched_ = true;
    return strm_.good();
}


Coord CBVSReader::getTrailerCoord( const BinID& bid ) const
{
    int arridx = 0;
    if ( info_.geom_.fullyrectandreg )
    {
	if ( bid.inl != info_.geom_.start.inl )
	{
	    const int nrcrl = (info_.geom_.stop.crl-info_.geom_.start.crl)
			    / info_.geom_.step.crl + 1;
	    arridx = nrcrl * (bid.inl - info_.geom_.start.inl);
	}
	arridx += (bid.crl-info_.geom_.start.crl) / info_.geom_.step.crl;
    }
    else
    {
	for ( int iinl=0; iinl<lds_.size(); iinl++ )
	{
	    const PosInfo::LineData& inlinf = *lds_[iinl];
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

    return info_.geom_.b2c.transform( bid );
}


bool CBVSReader::fetch( void** bufs, const bool* comps,
			const Interval<int>* samps, int offs )
{
    if ( !hinfofetched_ && auxnrbytes_ )
    {
	static PosAuxInfo dum;
	if ( !getAuxInfo(dum) ) return false;
    }

    if ( !samps ) samps = &samprg_;

    int iselc = -1;
    for ( int icomp=0; icomp<nrcomps_; icomp++ )
    {
	if ( comps && !comps[icomp] )
	{
	    StrmOper::seek( strm_, cnrbytes_[icomp], std::ios::cur );
	    continue;
	}
	iselc++;

	BasicComponentInfo* compinfo = info_.compinfo_[icomp];
	int bps = compinfo->datachar.nrBytes();
	if ( samps->start )
	    StrmOper::seek( strm_, samps->start*bps, std::ios::cur );
	if ( !StrmOper::readBlock( strm_, ((char*)bufs[iselc]) + offs*bps,
		    		   (samps->stop-samps->start+1) * bps ) )
	    break;

	if ( samps->stop < info_.nrsamples_-1 )
	    StrmOper::seek( strm_, (info_.nrsamples_-samps->stop-1)*bps,
		    std::ios::cur );
    }

    hinfofetched_ = false;
    return strm_.good() || strm_.eof();
}
