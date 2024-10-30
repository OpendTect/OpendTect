/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

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
#include "envvars.h"
#include "od_istream.h"
#include "posinfo.h"
#include "ptrman.h"
#include "strmoper.h"
#include "survinfo.h"
#include "tracedata.h"
#include "varlenarray.h"


#define mGetAuxFromStrm(auxinf,buf,memb,strm) \
    strm.getBin( buf, sizeof(auxinf.memb) ); \
    auxinf.memb = finterp_.get( buf, 0 )

#define mGetCoordAuxFromStrm(auxinf,buf,strm) \
    strm.getBin( buf, 2*sizeof(auxinf.coord.x_) ); \
    auxinf.coord.x_ = dinterp_.get( buf, 0 ); \
    auxinf.coord.y_ = dinterp_.get( buf, 1 )

#define mAuxSetting(ptr,n) (*ptr & (unsigned char)n)

mStartAllowDeprecatedSection

PosAuxInfo::PosAuxInfo( bool is2d )
    : PosAuxInfo()
{
    set2D( is2d );
}


PosAuxInfo::PosAuxInfo()
    : trckey_(OD::GeomSynth,Pos::IdxPair(0,0))
    , coord(0.,0.)
    , binid(const_cast<BinID&>(trckey_.position()))
{
}

mStopAllowDeprecatedSection


void PosAuxInfo::clear()
{
    if ( trckey_.is3D() )
	trckey_.setPosition( BinID(0,0) );
    else
	trckey_.setTrcNr( 0 );

    coord.x_ = coord.y_ = 0.;
    startpos = offset = azimuth = 0.f;
    pick = refnr = mUdf(float);
}


void PosAuxInfo::set2D( bool yn )
{
    if ( !trckey_.is2D() && yn )
	trckey_.setGeomID( Survey::getDefault2DGeomID() ).setTrcNr( 0 );
    else if ( !trckey_.is3D() && !yn )
	trckey_.setPosition( BinID(0,0) );
}



CBVSReader::CBVSReader( od_istream* s, bool glob_info_only,
				bool forceusecbvsinfo )
	: strm_(*s)
	, singlinemode_(false)
	, hinfofetched_(false)
	, bytespertrace_(0)
	, idxatpos_(0)
	, iinterp_(DataCharacteristics())
	, finterp_(DataCharacteristics())
	, dinterp_(DataCharacteristics())
	, hs_(false)
	, worktrcdata_(*new TraceData)
	, lastposfo_(0)
	, datastartfo_(0)
{
    hs_.step_.inl() = hs_.step_.crl() = 1;
    if ( readInfo(!glob_info_only,forceusecbvsinfo) )
	toStart();
}


CBVSReader::~CBVSReader()
{
    close();
    delete &worktrcdata_;
}


void CBVSReader::close()
{
    if ( !strmclosed_  )
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
    char* ptr = buf.getCStr();
    strm_.getBin( ptr, headstartbytes );

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

    strm_.getBin( ptr, integersize );
    getText( iinterp_.get(ptr,0), info_.stdtext_ );

    if ( !readComps() || !readGeom(forceusecbvsinfo) )
	return false;

    strm_.getBin( ptr, 2 * integersize );
    info_.seqnr_ = iinterp_.get( ptr, 0 );
    getText( iinterp_.get(ptr,1), info_.usertext_ );
    removeTrailingBlanks( info_.usertext_.getCStr() );

    datastartfo_ = strm_.position();

    CBVSInfo::SurvGeom& geom = info_.geom_;
    if ( !wanttrailer )
	geom.fullyrectandreg_ = true;

    bool needtrailer = !geom.fullyrectandreg_ || coordpol_ == InTrailer;
    if ( wanttrailer && needtrailer && !readTrailer() )
	return false;

    if ( geom.fullyrectandreg_ )
    {
	geom.cubedata_.erase();
	geom.cubedata_.generate( geom.start_, geom.stop_, geom.step_, true );
	lds_.generate( geom.start_, geom.stop_, geom.step_, true );
    }
    geom.reCalcBounds();

    firstbinid_ = geom.start_;
    curgeomcubepos_.toStart();
    setCubePos( true );
    if ( !geom.cubedata_.isValid(curgeomcubepos_) )
	return true; // empty file
    updCurBinID();
    firstbinid_ = geom.cubedata_.binID( curgeomcubepos_ );

    mkPosNrs();
    return true;
}


void CBVSReader::setCubePos( bool fromgeom ) const
{
    PosInfo::CubeDataPos& inp = fromgeom ? curgeomcubepos_ : curldscubepos_;
    PosInfo::CubeDataPos& out = fromgeom ? curldscubepos_ : curgeomcubepos_;
    const PosInfo::CubeData& inpcd = fromgeom ? info_.geom_.cubedata_ : lds_;
    const PosInfo::CubeData& outcd = fromgeom ? lds_ : info_.geom_.cubedata_;

    out = inp; // guess
    if ( outcd.binID(out) != inpcd.binID(inp) )
	out = outcd.cubeDataPos( inpcd.binID(inp) );
}


void CBVSReader::updCurBinID() const
{
    curbinid_ = info_.geom_.cubedata_.binID( curgeomcubepos_ );
}


void CBVSReader::getText( int nrchar, BufferString& txt )
{
    if ( nrchar > 0 )
    {
	if ( txt.bufSize() <= nrchar )
	    txt.setBufSize(nrchar+1);
	strm_.getBin( txt.getCStr(), nrchar );
	txt[nrchar] = '\0';
    }
}


#undef mErrRet
#define mErrRet { strm.setReadPosition( 0, od_stream::Abs ); return msg; }

const char* CBVSReader::check( od_istream& strm )
{
    if ( strm.isBad() ) return "Input stream cannot be used";

    strm.setReadPosition( 0, od_stream::Abs );
    char buf[4]; OD::memZero( buf, 4 );
    strm.getBin( buf, 3 );
    const char* msg = "Input stream cannot be used";
    if ( !strm.isOK() ) mErrRet;

    msg = "File is not in CBVS format";
    if ( StringView(buf)!="dGB" ) mErrRet;

    char plf; strm.getBin( &plf, 1 );
    if ( plf > 2 ) mErrRet;

    strm.setReadPosition( 0, od_stream::Abs );
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
    strm_.getBin( mVarLenArr(ucbuf), integersize );
    nrcomps_ = iinterp_.get( mVarLenArr(ucbuf), 0 );
    if ( nrcomps_ < 1 ) mErrRet("Corrupt CBVS format: No components defined")

    cnrbytes_ = new int [nrcomps_];
    bytespertrace_ = 0;

    for ( int icomp=0; icomp<nrcomps_; icomp++ )
    {
	strm_.getBin( mVarLenArr(ucbuf), integersize );
	BufferString bs; getText( iinterp_.get(mVarLenArr(ucbuf),0), bs );
	BasicComponentInfo* newinf = new BasicComponentInfo( (const char*)bs );

	strm_.getBin( mVarLenArr(ucbuf), integersize );
	newinf->datatype_ = iinterp_.get( mVarLenArr(ucbuf), 0 );
	strm_.getBin( mVarLenArr(ucbuf), 4 );
	newinf->datachar_.set( ucbuf[0], ucbuf[1] );
	// extra 2 bytes reserved for compression type
	newinf->datachar_.fmt_ = DataCharacteristics::Ieee;
	strm_.getBin( mVarLenArr(ucbuf), sizeof(float) );
	info_.sd_.start_ = finterp_.get( mVarLenArr(ucbuf), 0 );
	strm_.getBin( mVarLenArr(ucbuf), sizeof(float) );
	info_.sd_.step_ = finterp_.get( mVarLenArr(ucbuf), 0 );

	strm_.getBin( mVarLenArr(ucbuf), integersize ); // nr samples
	    info_.nrsamples_ = iinterp_.get( mVarLenArr(ucbuf), 0 );
	strm_.getBin( mVarLenArr(ucbuf), 2*sizeof(float) );
	    // reserved for per-component scaling: LinScaler( a, b )

	if ( info_.nrsamples_ < 0 || newinf->datatype_ < 0 )
	{
	    delete newinf;
	    mErrRet("Corrupt CBVS format: Component desciption error")
	}

	info_.compinfo_ += newinf;

	cnrbytes_[icomp] = info_.nrsamples_ * newinf->datachar_.nrBytes();
	bytespertrace_ += cnrbytes_[icomp];
	samprg_.set( 0, info_.nrsamples_-1, 1 );
    }

    return true;
}


bool CBVSReader::readGeom( bool forceusecbvsinfo )
{
    char buf[8*sizeof(double)];

    strm_.getBin( buf, 8*integersize );
    info_.geom_.fullyrectandreg_ = (bool)iinterp_.get( buf, 0 );
    info_.nrtrcsperposn_ = iinterp_.get( buf, 1 );
    info_.geom_.start_.inl() = iinterp_.get( buf, 2 );
    info_.geom_.start_.crl() = iinterp_.get( buf, 3 );
    info_.geom_.stop_.inl() = iinterp_.get( buf, 4 );
    info_.geom_.stop_.crl() = iinterp_.get( buf, 5 );
    info_.geom_.step_.inl() = iinterp_.get( buf, 6 );
    if ( info_.geom_.step_.inl() == 0 ) info_.geom_.step_.inl() = 1;
    info_.geom_.step_.crl() = iinterp_.get( buf, 7 );
    if ( info_.geom_.step_.crl() == 0 ) info_.geom_.step_.crl() = 1;
    if ( info_.geom_.step_.inl()<0 )
    {
	const int startinl = info_.geom_.start_.inl();
	info_.geom_.start_.inl() = info_.geom_.stop_.inl();
	info_.geom_.stop_.inl() = startinl;
    }

    if ( info_.geom_.step_.crl()<0 )
    {
	const int startcrl = info_.geom_.start_.crl();
	info_.geom_.start_.crl() = info_.geom_.stop_.crl();
	info_.geom_.stop_.crl() = startcrl;
    }
    strm_.getBin( buf, 6*sizeof(double) );
    Pos::IdxPair2Coord::DirTransform xtr, ytr;
    xtr.a = dinterp_.get( buf, 0 ); xtr.b = dinterp_.get( buf, 1 );
    xtr.c = dinterp_.get( buf, 2 ); ytr.a = dinterp_.get( buf, 3 );
    ytr.b = dinterp_.get( buf, 4 ); ytr.c = dinterp_.get( buf, 5 );
    mDefineStaticLocalObject( const bool, useinfvar,
                              =GetEnvVarYN("DTECT_CBVS_USE_STORED_SURVINFO") );
    const bool useinfo = forceusecbvsinfo ? true : useinfvar;
    if ( useinfo && xtr.valid(ytr) )
	info_.geom_.b2c_.setTransforms( xtr, ytr );
    else
	info_.geom_.b2c_ = SI().binID2Coord();

    hs_.start_ = hs_.stop_ = BinID( info_.geom_.start_.inl(),
				    info_.geom_.start_.crl() );
    hs_.include( BinID( info_.geom_.stop_.inl(), info_.geom_.stop_.crl() ) );

    return strm_.isOK();
}


bool CBVSReader::readTrailer()
{
    strm_.setReadPosition( -3, od_stream::End );
    char buf[40];
    strm_.getBin( buf, 3 ); buf[3] = '\0';
    if ( StringView(buf)!="BGd" ) mErrRet("Missing required file trailer")

    strm_.setReadPosition( -4-integersize, od_stream::End );
    strm_.getBin( buf, integersize );
    const int nrbytes = iinterp_.get( buf, 0 );

    strm_.setReadPosition( -4-integersize-nrbytes, od_stream::End );
    if ( coordpol_ == InTrailer )
    {
	strm_.getBin( buf, integersize );
	const int sz = iinterp_.get( buf, 0 );
	for ( int idx=0; idx<sz; idx++ )
	{
	    strm_.getBin( buf, 2 * sizeof(double) );
	    trailercoords_ += Coord( dinterp_.get(buf,0), dinterp_.get(buf,1) );
	}
    }

    if ( !info_.geom_.fullyrectandreg_ )
    {
	strm_.getBin( buf, integersize );
	const int nrinl = iinterp_.get( buf, 0 );
	if ( nrinl < 0 ) mErrRet("File trailer corrupt")
	if ( nrinl == 0 ) mErrRet("No traces in file")

	StepInterval<int> inlrg, crlrg;
	for ( int iinl=0; iinl<nrinl; iinl++ )
	{
	    strm_.getBin( buf, 2 * integersize );
	    PosInfo::LineData* iinf
		= new PosInfo::LineData( iinterp_.get( buf, 0 ) );
	    if ( !iinl )
                inlrg.start_ = inlrg.stop_ = iinf->linenr_;
	    else
		inlrg.include( iinf->linenr_, true );

	    const int nrseg = iinterp_.get( buf, 1 );
	    PosInfo::LineData::Segment crls;
	    for ( int iseg=0; iseg<nrseg; iseg++ )
	    {
		strm_.getBin( buf, 3 * integersize );

                crls.start_ = iinterp_.get(buf,0);
                crls.stop_ = iinterp_.get(buf,1);
                crls.step_ = iinterp_.get(buf,2);
		iinf->segments_ += crls;

		if ( !iseg )
		    crlrg = crls;
		crlrg.include( crls, true );
	    }
	    lds_ += iinf;
	}

	hs_.setInlRange( inlrg );
	hs_.setCrlRange( crlrg );
	info_.geom_.start_ = hs_.start_;
	info_.geom_.stop_ = hs_.stop_;
    }

    if ( !info_.geom_.fullyrectandreg_ )
	info_.geom_.cubedata_ = lds_;
    return strm_.isOK();
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
    info_.geom_.cubedata_.toNext( cdp );
    return info_.geom_.cubedata_.binID( cdp );
}


void CBVSReader::toOffs( od_int64 sp )
{
    lastposfo_ = sp;
    if ( strm_.position() != sp )
	strm_.setReadPosition( lastposfo_, od_stream::Abs );
}


bool CBVSReader::goTo( const BinID& inpbid )
{
    if ( strmclosed_ || lds_.isEmpty() )
	return false;

    BinID bid( inpbid );
    if ( singlinemode_ )
	bid.inl() = lds_[0]->linenr_;

    PosInfo::CubeDataPos cdp = lds_.cubeDataPos( bid );
    if ( !cdp.isValid() )
	return false;

    const int posnr = getPosNr( cdp, true );
    if ( posnr < 0 ) return false;

    od_stream::Pos so;
    so = posnr * (info_.nrtrcsperposn_ < 2
		      ? 1 : info_.nrtrcsperposn_);
    so *= auxnrbytes_ + bytespertrace_;

    toOffs( datastartfo_ + so );

    hinfofetched_ = false;
    idxatpos_ = 0;
    return true;
}


int CBVSReader::getPosNr( const PosInfo::CubeDataPos& cdp,
			  bool setcurrent ) const
{
    int posnr = -1;

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
	if ( !info_.geom_.cubedata_.toNext(curgeomcubepos_) )
	    return false;
	setCubePos( true );
	updCurBinID();
    }

    const od_stream::Pos onetrcoffs = auxnrbytes_ + bytespertrace_;
    od_stream::Pos posadd = onetrcoffs;
    toOffs( lastposfo_ + posadd );
    return true;
}


int CBVSReader::bytesOverheadPerTrace() const
{
    return auxnrbytes_;
}


int CBVSReader::estimatedNrTraces() const
{
    return info_.estimatedNrTraces();
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
    od_stream::Pos curfo mUnusedVar = strm_.position();
#endif

    if ( auxinf.trckey_.is3D() )
	auxinf.trckey_.setLineNr( curbinid_.inl() );

    auxinf.trckey_.setTrcNr( curbinid_.crl() );
    auxinf.coord = info_.geom_.b2c_.transform( curbinid_ );
    auxinf.startpos = info_.sd_.start_;
    auxinf.offset = auxinf.azimuth = 0;
    auxinf.pick = mSetUdf(auxinf.refnr);

    if ( auxnrbytes_ < 1 )
	return true;

    if ( hinfofetched_ )
	strm_.setReadPosition(-auxnrbytes_, od_stream::Rel );

    char buf[2*sizeof(double)];
    mCondGetAux(startpos)
    if ( coordpol_ == InAux && info_.auxinfosel_.coord )
	{ mGetCoordAuxFromStrm(auxinf,buf,strm_); }
    else if ( coordpol_ == InTrailer )
	auxinf.coord = getTrailerCoord( curbinid_ );
    mCondGetAux(offset)
    mCondGetAux(pick)
    mCondGetAux(refnr)
    mCondGetAux(azimuth)

    hinfofetched_ = true;
    return strm_.isOK();
}


Coord CBVSReader::getTrailerCoord( const BinID& bid ) const
{
    int arridx = 0;
    if ( info_.geom_.fullyrectandreg_ )
    {
	if ( bid.inl() != info_.geom_.start_.inl() )
	{
	    const int nrcrl = (info_.geom_.stop_.crl()-info_.geom_.start_.crl())
			      / info_.geom_.step_.crl() + 1;
	    arridx = nrcrl * (bid.inl() - info_.geom_.start_.inl());
	}
	arridx += (bid.crl()-info_.geom_.start_.crl()) /
						info_.geom_.step_.crl();
    }
    else
    {
	for ( int iinl=0; iinl<lds_.size(); iinl++ )
	{
	    const PosInfo::LineData& inlinf = *lds_[iinl];
	    bool inlmatches = inlinf.linenr_ == bid.inl();
	    for ( int icrl=0; icrl<inlinf.segments_.size(); icrl++ )
	    {
		const StepInterval<int>& seg = inlinf.segments_[icrl];
		if ( !inlmatches || !seg.includes(bid.crl(),true) )
		    arridx += seg.nrSteps() + 1;
		else
		{
		    arridx += (bid.crl() - seg.start_) / seg.step_;
		    break;
		}
	    }

	    if ( inlmatches )
		break;
	}
    }

    if ( arridx < trailercoords_.size() )
	return trailercoords_[arridx];

    return info_.geom_.b2c_.transform( bid );
}


bool CBVSReader::fetch( void** bufs, const bool* comps,
			const Interval<int>* samps, int offs )
{
    if ( !hinfofetched_ && auxnrbytes_ )
    {
	PosAuxInfo dum( false );
	if ( !getAuxInfo(dum) ) return false;
    }

    if ( !samps ) samps = &samprg_;

    int iselc = -1;
    for ( int icomp=0; icomp<nrcomps_; icomp++ )
    {
	if ( comps && !comps[icomp] )
	{
	    strm_.ignore( cnrbytes_[icomp] );
	    continue;
	}
	iselc++;

	BasicComponentInfo* compinfo = info_.compinfo_[icomp];
	int bps = compinfo->datachar_.nrBytes();
        if ( samps->start_ )
            strm_.ignore( samps->start_*bps );
	if ( !strm_.getBin(((char*)bufs[iselc]) + offs*bps,
                           (samps->stop_-samps->start_+1) * bps ) )
	    break;

        if ( samps->stop_ < info_.nrsamples_-1 )
            strm_.ignore((info_.nrsamples_-samps->stop_-1)*bps );
    }

    hinfofetched_ = false;
    return !strm_.isBad();
}


bool CBVSReader::fetch( TraceData& tdtofill, const bool* comps,
			const Interval<int>* samprg, int offs )
{
    if ( samprg )
    {
	const StepInterval<int> samprgint( *samprg );
	return fetch( tdtofill, comps, &samprgint, offs );
    }
    else
	return fetch( tdtofill, comps, (StepInterval<int>*)nullptr, offs );
}


bool CBVSReader::fetch( TraceData& tdtofill, const bool* comps,
			const StepInterval<int>* samprg, int offs )
{
    if ( !hinfofetched_ && auxnrbytes_ )
    {
	PosAuxInfo dum( false );
	if ( !getAuxInfo(dum) ) return false;
    }

    if ( !samprg )
	samprg = &samprg_;

    int iselc = -1;
    int nrcompsselected = nrcomps_;
    if ( comps )
    {
	for ( int icomp=0; icomp<nrcomps_; icomp++ )
	{
	    if ( !comps[icomp] )
		nrcompsselected--;
	}
    }

    if ( tdtofill.nrComponents() != nrcompsselected )
	tdtofill.setNrComponents( nrcompsselected, DataCharacteristics::Auto );

    tdtofill.convertTo( info_.compinfo_[0]->datachar_ );
    const int outnrsamps = samprg->nrSteps() + 1;
    if ( tdtofill.size(0) < outnrsamps )
	tdtofill.reSize( outnrsamps );

    TraceData* td = &tdtofill;
    if ( samprg->step_ > 1 )
    {
	worktrcdata_.setNrComponents( nrcompsselected,
				      DataCharacteristics::Auto );
	worktrcdata_.reSize( outnrsamps );
	td = &worktrcdata_;
    }

    const int nrsamps2skip = samprg->start_;
    const int nrsamps2read = samprg->stop_ - samprg->start_ + 1;
    const int nrsampsleftatend = info_.nrsamples_ - samprg->stop_ - 1;
    const int bps = info_.compinfo_[0]->datachar_.nrBytes();
    for ( int icomp=0; icomp<nrcomps_; icomp++ )
    {
	if ( comps && !comps[icomp] )
	    { strm_.ignore( cnrbytes_[icomp] ); continue; }
	iselc++;

	if ( nrsamps2skip > 0 )
	    strm_.ignore( nrsamps2skip*bps );

	char* bufptr = (char*)td->getComponent(iselc)->data();
	if ( !strm_.getBin(bufptr+offs*bps,nrsamps2read*bps) )
	    break;

	if ( nrsampsleftatend > 0 )
	    strm_.ignore( nrsampsleftatend*bps );
    }

    if ( td != &tdtofill )
    {
	for ( auto icomp=0; icomp<nrcomps_; icomp++ )
	{
	    for (auto isamp=0; isamp<outnrsamps; isamp++ )
		tdtofill.setValue( isamp,
			td->getValue(isamp*samprg->step_, icomp), icomp );
	}
    }

    hinfofetched_ = false;
    return !strm_.isBad();
}
