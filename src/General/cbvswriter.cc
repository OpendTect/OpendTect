/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert
 * DATE     : Mar 2001
 * FUNCTION : CBVS I/O
-*/


#include "cbvswriter.h"
#include "tracedata.h"
#include "genc.h"
#include "survinfo.h"

const int CBVSIO::integersize = 4;
const int CBVSIO::version = 2;
const int CBVSIO::headstartbytes = 8 + 2 * CBVSIO::integersize;

CBVSIO::~CBVSIO()
{
    delete [] cnrbytes_;
    deepErase( lds_ );
}


CBVSWriter::CBVSWriter( od_ostream* s, const CBVSInfo& i,
			const PosAuxInfo* e, CoordPol coordpol )
	: strm_(*s)
	, auxinfo_(e)
	, thrbytes_(0)
	, file_lastinl_(false)
	, prevbinid_(mUdf(int),mUdf(int))
	, trcswritten_(0)
	, nrtrcsperposn_(i.nrtrcsperposn_)
	, nrtrcsperposn_status_(2)
	, checknrtrcsperposn_(0)
	, auxinfosel_(i.auxinfosel_)
	, survgeom_(i.geom_)
	, auxnrbytes_(0)
	, input_rectnreg_(false)
	, forcedlinestep_(0,0)
	, forcetrailer_(false)
{
    coordpol_ = coordpol;
    init( i );
}


CBVSWriter::CBVSWriter( od_ostream* s, const CBVSWriter& cw,
			const CBVSInfo& ci )
	: strm_(*s)
	, auxinfo_(cw.auxinfo_)
	, thrbytes_(cw.thrbytes_)
	, file_lastinl_(false)
	, prevbinid_(mUdf(int),mUdf(int))
	, trcswritten_(0)
	, nrtrcsperposn_(cw.nrtrcsperposn_)
	, nrtrcsperposn_status_(cw.nrtrcsperposn_status_)
	, auxinfosel_(cw.auxinfosel_)
	, survgeom_(ci.geom_)
	, auxnrbytes_(0)
	, forcedlinestep_(cw.forcedlinestep_)
{
    coordpol_ = cw.coordpol_;
    init( ci );
}


void CBVSWriter::init( const CBVSInfo& i )
{
    nrbytespersample_ = 0;

    if ( !strm_.isOK() )
	{ errmsg_ = uiStrings::phrCannotOpenForWrite(strm_.fileName());
	    return; }
    if ( !survgeom_.fullyrectandreg && !auxinfo_ )
	{ pErrMsg("Survey not rectangular but no explicit inl/crl info");
	  errmsg_ = mINTERNAL("no explicit info"); return; }

    if ( auxinfo_ && survgeom_.fullyrectandreg
      && !auxinfosel_.startpos_ && !auxinfosel_.coord_
      && !auxinfosel_.offset_ && !auxinfosel_.azimuth_
      && !auxinfosel_.pick_ && !auxinfosel_.refnr_ )
	auxinfo_ = 0;

    writeHdr( i ); if ( !errmsg_.isEmpty() ) return;

    input_rectnreg_ = survgeom_.fullyrectandreg;

    const od_stream::Pos cursp = strm_.position();
    strm_.setWritePosition( 8 );
    int nrbytes = (int)cursp;
    strm_.addBin( &nrbytes, integersize );
    strm_.setWritePosition( cursp );
}


CBVSWriter::~CBVSWriter()
{
    close();
    delete &strm_;

    delete [] nrbytespersample_;
}


#define mErrRet(s) { errmsg_ = s; return; }

void CBVSWriter::writeHdr( const CBVSInfo& info )
{
    unsigned char ucbuf[headstartbytes]; OD::memZero( ucbuf, headstartbytes );
    ucbuf[0] = 'd'; ucbuf[1] = 'G'; ucbuf[2] = 'B';
    PutIsLittleEndian( ucbuf + 3 );
    ucbuf[4] = version;
    putAuxInfoSel( ucbuf + 5 );
    ucbuf[6] = (unsigned char)coordpol_;
    if ( !strm_.addBin(ucbuf,headstartbytes) )
	mErrRet(uiStrings::phrCannotStart(tr("writing to file")))

    int sz = info.stdtext_.size();
    strm_.addBin( &sz, integersize );
    strm_.addBin( info.stdtext_, sz );

    writeComps( info );
    geomsp_ = strm_.position();
    writeGeom();
    strm_.addBin( &info.seqnr_, integersize );
    BufferString bs( info.usertext_ );

    const od_stream::Pos fo = strm_.position();
    int len = bs.size();
    int endpos = (int)fo + len + 4;
    while ( endpos % 4 )
	{ bs += " "; len++; endpos++; }
    strm_.addBin( &len, integersize );
    strm_.addBin( bs, len );

    if ( !strm_.isOK() )
	mErrRet(uiStrings::phrCannotWrite(tr("complete header")));
}


void CBVSWriter::putAuxInfoSel( unsigned char* ptr ) const
{
    *ptr = 0;
    int auxbts = 0;

#define mDoMemb(memb,n) \
    if ( auxinfosel_.memb ) \
    { \
	*ptr |= (unsigned char)n; \
	auxbts += sizeof(auxinfo_->memb); \
    }

    mDoMemb(startpos_,1)
    if ( coordpol_ == InAux && auxinfosel_.coord_ )
    {
	*ptr |= (unsigned char)2;
	auxbts += 2 * sizeof(double);
    }
    mDoMemb(offset_,4)
    mDoMemb(pick_,8)
    mDoMemb(refnr_,16)
    mDoMemb(azimuth_,32)

    const_cast<CBVSWriter*>(this)->auxnrbytes_ = auxbts;
}


void CBVSWriter::writeComps( const CBVSInfo& info )
{
    nrcomps_ = info.compinfo_.size();
    strm_.addBin( &nrcomps_, integersize );

    cnrbytes_ = new int [nrcomps_];
    nrbytespersample_ = new int [nrcomps_];

    unsigned char dcdump[4];
    dcdump[2] = dcdump[3] = 0; // added space for compression type

    for ( int icomp=0; icomp<nrcomps_; icomp++ )
    {
	const BasicComponentInfo& cinf = *info.compinfo_[icomp];
	int sz = cinf.name().size();
	strm_.addBin( &sz, integersize );
	strm_.addBin( cinf.name(), sz );
	strm_.addBin( &cinf.datatype_, integersize );
	cinf.datachar_.dump( dcdump[0], dcdump[1] );
	strm_.addBin( dcdump, 4 );
	strm_.addBin( &info.sd_.start, sizeof(float) );
	strm_.addBin( &info.sd_.step, sizeof(float) );
	strm_.addBin( &info.nrsamples_, integersize );
	float a = 0, b = 1; // LinScaler( a, b ) - future use?
	strm_.addBin( &a, sizeof(float) );
	strm_.addBin( &b, sizeof(float) );

	nrbytespersample_[icomp] = cinf.datachar_.nrBytes();
	cnrbytes_[icomp] = info.nrsamples_ * nrbytespersample_[icomp];
    }
}


void CBVSWriter::writeGeom()
{
    int irect = 0;
    if ( survgeom_.fullyrectandreg )
    {
	irect = 1;
	nrxlines_ = (survgeom_.stop.crl() - survgeom_.start.crl())
		  / survgeom_.step.crl() + 1;
    }
    strm_.addBin( &irect, integersize );
    strm_.addBin( &nrtrcsperposn_, integersize );
    strm_.addBin( &survgeom_.start.inl(), 2 * integersize );
    strm_.addBin( &survgeom_.stop.inl(), 2 * integersize );
    strm_.addBin( &survgeom_.step.inl(), 2 * integersize );
    strm_.addBin( &survgeom_.b2c.getTransform(true).a,
		    3*sizeof(double) );
    strm_.addBin( &survgeom_.b2c.getTransform(false).a,
		    3*sizeof(double) );
}


void CBVSWriter::newSeg( bool newinl )
{
    bool goodgeom = nrtrcsperposn_status_ != 2 && nrtrcsperposn_ > 0;
    if ( !goodgeom && !newinl )
    {
	lds_[lds_.size()-1]->segments_[0].stop = curbinid_.crl();
	return;
    }
    goodgeom = nrtrcsperposn_status_ == 0 && nrtrcsperposn_ > 0;

    if ( !trcswritten_ ) prevbinid_ = curbinid_;

    int newstep = forcedlinestep_.crl() ? forcedlinestep_.crl()
					: SI().crlStep();
    if ( newinl )
    {
	if ( goodgeom && lds_.size() )
	    newstep = lds_[lds_.size()-1]->segments_[0].step;
	lds_ += new PosInfo::LineData( curbinid_.inl() );
    }
    else if ( goodgeom )
	newstep = lds_[lds_.size()-1]->segments_[0].step;

    lds_[lds_.size()-1]->segments_ +=
	PosInfo::LineData::Segment(curbinid_.crl(),curbinid_.crl(), newstep);
}


void CBVSWriter::getBinID()
{
    const int nrtrcpp = nrtrcsperposn_ < 2 ? 1 : nrtrcsperposn_;
    if ( input_rectnreg_ || !auxinfo_ )
    {
	int posidx = trcswritten_ / nrtrcpp;
	curbinid_.inl() = survgeom_.start.inl()
		   + survgeom_.step.inl() * (posidx / nrxlines_);
	curbinid_.crl() = survgeom_.start.crl()
		   + survgeom_.step.crl() * (posidx % nrxlines_);
    }
    else if ( !(trcswritten_ % nrtrcpp) )
    {
	curbinid_ = auxinfo_->trckey_.binID();
	if ( !trcswritten_ || prevbinid_.inl() != curbinid_.inl() )
	    newSeg( true );
	else
	{
	    PosInfo::LineData& inlinf = *lds_[lds_.size()-1];
	    PosInfo::LineData::Segment& seg =
				inlinf.segments_[inlinf.segments_.size()-1];
	    if ( !forcedlinestep_.crl() && seg.stop == seg.start )
	    {
		if ( seg.stop != curbinid_.crl() )
		{
		    seg.stop = curbinid_.crl();
		    seg.step = seg.stop - seg.start;
		}
	    }
	    else
	    {
		if ( curbinid_.crl() != seg.stop + seg.step )
		    newSeg( false );
		else
		    seg.stop = curbinid_.crl();
	    }
	}
    }
}


int CBVSWriter::put( const TraceData& cdat, int offs )
{
#ifdef __debug__
    // gdb says: "Couldn't find method ostream::tellp"
    const od_stream::Pos curfo mUnusedVar = strm_.position();
#endif

    getBinID();
    if ( prevbinid_.inl() != curbinid_.inl() )
    {
	// getBinID() has added a new segment, so remove it from list ...
	PosInfo::LineData* newinldat = lds_[lds_.size()-1];
	lds_.removeAndTake( lds_.size()-1 );
	if ( file_lastinl_ )
	{
	    delete newinldat;
	    close();
	    return !errmsg_.isEmpty() ? -1 : 1;
	}

	doClose( false );
	lds_ += newinldat;

	if ( !errmsg_.isEmpty() ) return -1;
    }

    if ( !writeAuxInfo() )
    {
	errmsg_ = uiStrings::phrCannotWrite(tr("Trace header data"));
	return -1;
    }

    for ( int icomp=0; icomp<nrcomps_; icomp++ )
    {
	const char* ptr = ((const char*)cdat.getComponent(icomp)->data())
			+ offs * nrbytespersample_[icomp];

	if ( !strm_.addBin(ptr,cnrbytes_[icomp]) )
	    { errmsg_ = uiStrings::phrCannotWrite(tr("CBVS data")); return -1; }
    }

    if ( thrbytes_ && strm_.position() >= thrbytes_ )
	file_lastinl_ = true;

    if ( nrtrcsperposn_status_ && trcswritten_ )
    {
	if ( trcswritten_ == 1 )
	    nrtrcsperposn_ = 1;

	if ( nrtrcsperposn_status_ == 2 )
	{
	    if ( prevbinid_ == curbinid_ )
		nrtrcsperposn_++;
	    else
	    {
		nrtrcsperposn_status_ = 1;
		checknrtrcsperposn_ = 1;
	    }
	}
	else
	{
	    if ( prevbinid_ == curbinid_ )
		checknrtrcsperposn_++;
	    else
	    {
		nrtrcsperposn_status_ = 0;
		if ( checknrtrcsperposn_ != nrtrcsperposn_ )
		{
		    input_rectnreg_ = true;
		    nrtrcsperposn_ = -1;
		}
	    }
	}
    }
    prevbinid_ = curbinid_;
    trcswritten_++;
    return 0;
}


bool CBVSWriter::writeAuxInfo()
{
    if ( auxinfo_ && auxnrbytes_ )
    {
#define mDoWrAI(memb)  \
    if ( auxinfosel_.memb ) \
	strm_.addBin( &auxinfo_->memb, sizeof(auxinfo_->memb) );

	mDoWrAI(startpos_)
	if ( coordpol_ == InTrailer )
	    trailercoords_ += auxinfo_->coord_;
	else if ( coordpol_ == InAux && auxinfosel_.coord_ )
	{
	    strm_.addBin( &auxinfo_->coord_.x_, sizeof(double) );
	    strm_.addBin( &auxinfo_->coord_.y_, sizeof(double) );
	}
	mDoWrAI(offset_)
	mDoWrAI(pick_)
	mDoWrAI(refnr_)
	mDoWrAI(azimuth_)
    }

    return strm_.isOK();
}


void CBVSWriter::doClose( bool islast )
{
    if ( strmclosed_ ) return;

    getRealGeometry();
    const od_stream::Pos kp = strm_.position();
    strm_.setWritePosition( geomsp_ );
    writeGeom();
    strm_.setWritePosition( kp );

    if ( !writeTrailer() )
    {
	// shazbut! we were almost there!
	errmsg_ = uiStrings::phrCannotWrite(tr("CBVS trailer"));
	ErrMsg( mFromUiStringTodo(errmsg_) );
    }

    strm_.flush();
    if ( islast )
	strmclosed_ = true;
    else
	strm_.setWritePosition( kp );
}


void CBVSWriter::getRealGeometry()
{
    PosInfo::CubeData& cd = survgeom_.cubedata;
    deepErase( cd );
    for ( int idx=0; idx<lds_.size(); idx++ )
	cd += new PosInfo::LineData( *lds_[idx] );

    survgeom_.fullyrectandreg = forcetrailer_ ? false : cd.isFullyRegular();
    StepInterval<int> rg;
    cd.getInlRange( rg ); survgeom_.step.inl() = rg.step;
    survgeom_.start.inl() = rg.start; survgeom_.stop.inl() = rg.stop;
    cd.getCrlRange( rg ); survgeom_.step.crl() = rg.step;
    survgeom_.start.crl() = rg.start; survgeom_.stop.crl() = rg.stop;
    if ( cd.isCrlReversed() )
	survgeom_.step.crl() *= -1;

    if ( !cd.haveCrlStepInfo() )
	survgeom_.step.crl() = SI().crlStep();
    if ( !cd.haveInlStepInfo() )
	survgeom_.step.inl() = SI().inlStep();
    else if ( lds_[0]->linenr_ > lds_[1]->linenr_ )
	survgeom_.step.inl() = -survgeom_.step.inl();

    if ( survgeom_.fullyrectandreg )
	deepErase( cd );
    else if ( forcedlinestep_.inl() )
	survgeom_.step.inl() = forcedlinestep_.inl();
}


bool CBVSWriter::writeTrailer()
{
    const od_stream::Pos trailerstart = strm_.position();

    if ( coordpol_ == InTrailer )
    {
	const int sz = trailercoords_.size();
	strm_.addBin( &sz, integersize );
	for ( int idx=0; idx<sz; idx++ )
	{
	    Coord c( trailercoords_[idx] );
	    strm_.addBin( &c.x_, sizeof(double) );
	    strm_.addBin( &c.y_, sizeof(double) );
	}
    }

    if ( !survgeom_.fullyrectandreg )
    {
	const int nrinl = lds_.size();
	strm_.addBin( &nrinl, integersize );
	for ( int iinl=0; iinl<nrinl; iinl++ )
	{
	    PosInfo::LineData& inlinf = *lds_[iinl];
	    strm_.addBin( &inlinf.linenr_, integersize );
	    const int nrcrl = inlinf.segments_.size();
	    strm_.addBin( &nrcrl, integersize );

	    for ( int icrl=0; icrl<nrcrl; icrl++ )
	    {
		PosInfo::LineData::Segment& seg = inlinf.segments_[icrl];
		strm_.addBin( &seg.start, integersize );
		strm_.addBin( &seg.stop, integersize );
		strm_.addBin( &seg.step, integersize );
	    }
	    if ( !strm_.isOK() ) return false;
	}
    }

    int bytediff = (int)(strm_.position() - trailerstart);
    strm_.addBin( &bytediff, integersize );
    unsigned char buf[4];
    PutIsLittleEndian( buf );
    buf[1] = 'B'; buf[2] = 'G'; buf[3] = 'd';
    strm_.addBin( buf, integersize );

    return true;
}
