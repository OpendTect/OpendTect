/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert
 * DATE     : Mar 2001
 * FUNCTION : CBVS I/O
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
#include "survinfo.h"
#include "envvars.h"
#include "ptrman.h"
#include "varlenarray.h"
#include "posinfo.h"
#include "tracedata.h"
#include "od_istream.h"

#define mGetAuxFromStrm(auxinf,buf,memb,strm) \
    strm.getBin( buf, sizeof(auxinf.memb) ); \
    auxinf.memb = finterp_.get( buf, 0 )

#define mGetCoordAuxFromStrm(auxinf,buf,strm) \
    strm.getBin( buf, 2*sizeof(auxinf.coord_.x_) ); \
    auxinf.coord_.x_ = dinterp_.get( buf, 0 ); \
    auxinf.coord_.y_ = dinterp_.get( buf, 1 )

#define mAuxSetting(ptr,n) (*ptr & (unsigned char)n)



CBVSReader::CBVSReader( od_istream* s, bool glob_info_only,
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
	, singlinemode_(false)
	, worktrcdata_(*new TraceData)
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
    if ( !errmsg_.isEmpty() ) return false;

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
	geom.fullyrectandreg = true;

    bool needtrailer = !geom.fullyrectandreg || coordpol_ == InTrailer;
    if ( wanttrailer && needtrailer && !readTrailer() )
	return false;

    if ( geom.fullyrectandreg )
    {
	geom.cubedata.erase();
	geom.cubedata.generate( geom.start, geom.stop, geom.step, true );
	lds_.generate( geom.start, geom.stop, geom.step, true );
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
	strm_.getBin( txt.getCStr(), nrchar );
	txt[nrchar] = '\0';
    }
}


#undef mErrRet
#define mErrRet { strm.setReadPosition( 0, od_stream::Abs ); return msg; }

uiString CBVSReader::check( od_istream& strm )
{
    if ( strm.isBad() ) return uiStrings::phrInput(tr("stream cannot be used"));

    strm.setReadPosition( 0, od_stream::Abs );
    char buf[4]; OD::memZero( buf, 4 );
    strm.getBin( buf, 3 );
    uiString msg = uiStrings::phrInput(tr("stream cannot be used"));
    if ( !strm.isOK() ) mErrRet;

    msg = tr("File is not in CBVS format");
    if ( FixedString(buf)!="dGB" ) mErrRet;

    char plf; strm.getBin( &plf, 1 );
    if ( plf > 2 ) mErrRet;

    strm.setReadPosition( 0, od_stream::Abs );
    return uiString::empty();
}


void CBVSReader::getAuxInfoSel( const char* ptr )
{
    info_.auxinfosel_.startpos_ =	mAuxSetting(ptr,1);
    info_.auxinfosel_.coord_ =	mAuxSetting(ptr,2);
    info_.auxinfosel_.offset_ =	mAuxSetting(ptr,4);
    info_.auxinfosel_.pick_ =	mAuxSetting(ptr,8);
    info_.auxinfosel_.refnr_ =	mAuxSetting(ptr,16);
    info_.auxinfosel_.azimuth_ =	mAuxSetting(ptr,32);

#define mAddBytes(memb,t) \
    if ( info_.auxinfosel_.memb ) auxnrbytes_ += sizeof(t)
    auxnrbytes_ = 0;
    mAddBytes(startpos_,float);
    if ( coordpol_ == InAux )
    {
	mAddBytes(coord_,double); // x
	mAddBytes(coord_,double); // y
    }
    mAddBytes(offset_,float);
    mAddBytes(pick_,float);
    mAddBytes(refnr_,float);
    mAddBytes(azimuth_,float);
}


#undef mErrRet
#define mErrRet(s) { toOffs(0); errmsg_ = s; return false; }


bool CBVSReader::readComps()
{
    mAllocVarLenArr( char, ucbuf, 4*integersize );
    strm_.getBin( ucbuf, integersize );
    nrcomps_ = iinterp_.get( ucbuf, 0 );
    if ( nrcomps_ < 1 )
	mErrRet(tr("Corrupt CBVS format: No components defined"))

    cnrbytes_ = new int [nrcomps_];
    bytespertrace_ = 0;

    for ( int icomp=0; icomp<nrcomps_; icomp++ )
    {
	strm_.getBin( ucbuf, integersize );
	BufferString bs; getText( iinterp_.get(ucbuf,0), bs );
	BasicComponentInfo* newinf = new BasicComponentInfo( (const char*)bs );

	strm_.getBin( ucbuf, integersize );
	    newinf->datatype_ = iinterp_.get( ucbuf, 0 );
	strm_.getBin( ucbuf, 4 );
	    newinf->datachar_.set( ucbuf[0], ucbuf[1] );
	    // extra 2 bytes reserved for compression type
	    newinf->datachar_.fmt_ = DataCharacteristics::Ieee;
	strm_.getBin( ucbuf, sizeof(float) );
	    info_.sd_.start = finterp_.get( ucbuf, 0 );
	strm_.getBin( ucbuf, sizeof(float) );
	    info_.sd_.step = finterp_.get( ucbuf, 0 );

	strm_.getBin( ucbuf, integersize ); // nr samples
	    info_.nrsamples_ = iinterp_.get( ucbuf, 0 );
	strm_.getBin( ucbuf, 2*sizeof(float) );
	    // reserved for per-component scaling: LinScaler( a, b )

	if ( info_.nrsamples_ < 0 )
	{
	    delete newinf;
	    mErrRet(tr("Corrupt CBVS format: Component desciption error"))
	}

	info_.compinfo_ += newinf;

	cnrbytes_[icomp] = info_.nrsamples_ * newinf->datachar_.nrBytes();
	bytespertrace_ += cnrbytes_[icomp];
	samprg_ = StepInterval<int>( 0, info_.nrsamples_-1, 1 );
    }

    return true;
}


bool CBVSReader::readGeom( bool forceusecbvsinfo )
{
    char buf[8*sizeof(double)];

    strm_.getBin( buf, 8*integersize );
    info_.geom_.fullyrectandreg = (bool)iinterp_.get( buf, 0 );
    info_.nrtrcsperposn_ = iinterp_.get( buf, 1 );
    info_.geom_.start.inl() = iinterp_.get( buf, 2 );
    info_.geom_.start.crl() = iinterp_.get( buf, 3 );
    info_.geom_.stop.inl() = iinterp_.get( buf, 4 );
    info_.geom_.stop.crl() = iinterp_.get( buf, 5 );
    info_.geom_.step.inl() = iinterp_.get( buf, 6 );
    if ( info_.geom_.step.inl() == 0 ) info_.geom_.step.inl() = 1;
    info_.geom_.step.crl() = iinterp_.get( buf, 7 );
    if ( info_.geom_.step.crl() == 0 ) info_.geom_.step.crl() = 1;
    if ( info_.geom_.step.inl()<0 )
    {
	const int startinl = info_.geom_.start.inl();
	info_.geom_.start.inl() = info_.geom_.stop.inl();
	info_.geom_.stop.inl() = startinl;
    }

    if ( info_.geom_.step.crl()<0 )
    {
	const int startcrl = info_.geom_.start.crl();
	info_.geom_.start.crl() = info_.geom_.stop.crl();
	info_.geom_.stop.crl() = startcrl;
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
	info_.geom_.b2c.setTransforms( xtr, ytr );
    else
	info_.geom_.b2c = SI().binID2Coord();

    hs_.start_ = hs_.stop_ = BinID( info_.geom_.start.inl(),
                                  info_.geom_.start.crl() );
    hs_.include( BinID( info_.geom_.stop.inl(), info_.geom_.stop.crl() ) );

    return strm_.isOK();
}


bool CBVSReader::readTrailer()
{
    strm_.setReadPosition( -3, od_stream::End );
    char buf[40];
    strm_.getBin( buf, 3 ); buf[3] = '\0';
    if ( FixedString(buf)!="BGd" ) mErrRet(tr("Missing required file trailer"))

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

    if ( !info_.geom_.fullyrectandreg )
    {
	strm_.getBin( buf, integersize );
	const int nrinl = iinterp_.get( buf, 0 );
	if ( nrinl < 0 ) mErrRet(tr("File trailer corrupt"))
	if ( nrinl == 0 ) mErrRet(tr("No traces in file"))
	StepInterval<int> inlrg, crlrg;
	for ( int iinl=0; iinl<nrinl; iinl++ )
	{
	    strm_.getBin( buf, 2 * integersize );
	    PosInfo::LineData* iinf
		= new PosInfo::LineData( iinterp_.get( buf, 0 ) );
	    if ( !iinl )
		inlrg.start = inlrg.stop = iinf->linenr_;
	    else
		inlrg.include( iinf->linenr_, true );

	    const int nrseg = iinterp_.get( buf, 1 );
	    PosInfo::LineData::Segment crls;
	    for ( int iseg=0; iseg<nrseg; iseg++ )
	    {
		strm_.getBin( buf, 3 * integersize );

		crls.start = iinterp_.get(buf,0);
		crls.stop = iinterp_.get(buf,1);
		crls.step = iinterp_.get(buf,2);
		iinf->segments_ += crls;

		if ( !iseg )
		    crlrg = crls;
		crlrg.include( crls, true );
	    }
	    lds_ += iinf;
	}

	hs_.setInlRange( inlrg );
	hs_.setCrlRange( crlrg );
	info_.geom_.start = hs_.start_;
	info_.geom_.stop = hs_.stop_;
    }

    if ( !info_.geom_.fullyrectandreg )
	info_.geom_.cubedata = lds_;
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
    info_.geom_.cubedata.toNext( cdp );
    return info_.geom_.cubedata.binID( cdp );
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
    if ( posnr < 0 )
	return false;

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
	if ( !info_.geom_.cubedata.toNext(curgeomcubepos_) )
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

    auxinf.trckey_.setLineNr( curbinid_.inl() );
    auxinf.trckey_.setTrcNr( curbinid_.crl() );
    auxinf.coord_ = info_.geom_.b2c.transform( curbinid_ );
    auxinf.startpos_ = info_.sd_.start;
    auxinf.offset_ = auxinf.azimuth_ = 0;
    auxinf.pick_ = mSetUdf( auxinf.refnr_ );

    if ( auxnrbytes_ < 1 )
	return true;

    if ( hinfofetched_ )
	strm_.setReadPosition( -auxnrbytes_, od_stream::Rel );

    char buf[2*sizeof(double)];
    mCondGetAux(startpos_)
    if ( coordpol_ == InAux && info_.auxinfosel_.coord_ )
	{ mGetCoordAuxFromStrm(auxinf,buf,strm_); }
    else if ( coordpol_ == InTrailer )
	auxinf.coord_ = getTrailerCoord( auxinf.trckey_.binID() );
    mCondGetAux(offset_)
    mCondGetAux(pick_)
    mCondGetAux(refnr_)
    mCondGetAux(azimuth_)

    hinfofetched_ = true;
    return strm_.isOK();
}


Coord CBVSReader::getTrailerCoord( const BinID& bid ) const
{
    int arridx = 0;
    if ( info_.geom_.fullyrectandreg )
    {
	if ( bid.inl() != info_.geom_.start.inl() )
	{
	    const int nrcrl = (info_.geom_.stop.crl()-info_.geom_.start.crl())
			    / info_.geom_.step.crl() + 1;
	    arridx = nrcrl * (bid.inl() - info_.geom_.start.inl());
	}
	arridx += (bid.crl()-info_.geom_.start.crl()) / info_.geom_.step.crl();
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
		    arridx += (bid.crl() - seg.start) / seg.step;
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


bool CBVSReader::fetch( TraceData& tdtofill, const bool* comps,
			const StepInterval<int>* samprg, int offs )
{
    if ( !hinfofetched_ && auxnrbytes_ )
    {
	PosAuxInfo dum;
	if ( !getAuxInfo(dum) )
	    return false;
    }

    if ( !samprg )
	samprg = &samprg_;

    int iselc = -1;
    int nrcompsselected = nrcomps_;
    if ( comps )
	for ( int icomp=0; icomp<nrcomps_; icomp++ )
	    if ( !comps[icomp] )
		nrcompsselected--;
    if ( tdtofill.nrComponents() != nrcompsselected )
	tdtofill.setNrComponents( nrcompsselected, OD::AutoDataRep );

    tdtofill.convertTo( info_.compinfo_[0]->datachar_ );
    const auto outnrsamps = samprg->nrSteps() + 1;
    if ( tdtofill.size(0) < outnrsamps )
	tdtofill.reSize( outnrsamps );

    TraceData* td = &tdtofill;
    if ( samprg->step > 1 )
    {
	worktrcdata_.setNrComponents( nrcompsselected, OD::AutoDataRep );
	worktrcdata_.reSize( samprg->stop-samprg->start+1 );
	td = &worktrcdata_;
    }

    const auto nrsamps2skip = samprg->start;
    const auto nrsamps2read = samprg->stop - samprg->start + 1;
    const auto nrsampsleftatend = info_.nrsamples_ - samprg->stop - 1;
    const auto bps = info_.compinfo_[0]->datachar_.nrBytes();
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
	for ( int icomp=0; icomp<nrcomps_; icomp++ )
	    for ( auto isamp=0; isamp<outnrsamps; isamp++ )
		tdtofill.setValue( isamp,
				   td->getValue(isamp*samprg->step,icomp),
				   icomp );

    hinfofetched_ = false;
    return !strm_.isBad();
}
