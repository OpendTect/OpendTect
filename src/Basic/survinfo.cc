/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : A.H. Bril
 * DATE     : 18-4-1996
-*/

static const char* rcsID = "$Id: survinfo.cc,v 1.58 2004-07-29 21:41:26 bert Exp $";

#include "survinfo.h"
#include "ascstream.h"
#include "filepath.h"
#include "separstr.h"
#include "errh.h"
#include "strmprov.h"
#include "iopar.h"
#include "cubesampling.h"
#include "keystrs.h"

static const char* sKeySI = "Survey Info";
static const char* sKeyXTransf = "Coord-X-BinID";
static const char* sKeyYTransf = "Coord-Y-BinID";
const char* SurveyInfo::sKeyInlRange = "In-line range";
const char* SurveyInfo::sKeyCrlRange = "Cross-line range";
const char* SurveyInfo::sKeyZRange = "Z range";
const char* SurveyInfo::sKeyWSProjName = "Workstation Project Name";
const char* SurveyInfo::sKeyDpthInFt = "Show depth in feet";


SurveyInfo* SurveyInfo::theinst_ = 0;
bool SurveyInfo::dowarnings_ = true;

const SurveyInfo& SI()
{
    if ( !SurveyInfo::theinst_ || !SurveyInfo::theinst_->valid_ )
    {
	if ( SurveyInfo::theinst_ )
	{
	    SurveyInfo * myinst = SurveyInfo::theinst_;
	    SurveyInfo::theinst_ = 0;
	    delete myinst;
	}
	SurveyInfo::theinst_ = SurveyInfo::read( GetDataDir() );
    }
    return *SurveyInfo::theinst_;
}


const char* BinID2Coord::set3Pts( const Coord c[3], const BinID b[2], int xline)
{
    if ( b[1].inl == b[0].inl )
        return "Need two different in-lines";
    if ( b[0].crl == xline )
        return "No Cross-line range present";

    BCTransform nxtr, nytr;
    int crld = b[0].crl - xline;
    nxtr.c = ( c[0].x - c[2].x ) / crld;
    nytr.c = ( c[0].y - c[2].y ) / crld;
    int inld = b[0].inl - b[1].inl;
    crld = b[0].crl - b[1].crl;
    nxtr.b = ( c[0].x - c[1].x ) / inld - ( nxtr.c * crld / inld );
    nytr.b = ( c[0].y - c[1].y ) / inld - ( nytr.c * crld / inld );
    nxtr.a = c[0].x - nxtr.b * b[0].inl - nxtr.c * b[0].crl;
    nytr.a = c[0].y - nytr.b * b[0].inl - nytr.c * b[0].crl;

    if ( mIsZero(nxtr.a,mDefEps) ) nxtr.a = 0;
    if ( mIsZero(nxtr.b,mDefEps) ) nxtr.b = 0;
    if ( mIsZero(nxtr.c,mDefEps) ) nxtr.c = 0;
    if ( mIsZero(nytr.a,mDefEps) ) nytr.a = 0;
    if ( mIsZero(nytr.b,mDefEps) ) nytr.b = 0;
    if ( mIsZero(nytr.c,mDefEps) ) nytr.c = 0;

    if ( !nxtr.valid(nytr) )
	return "Cannot construct a valid transformation matrix from this input";

    xtr = nxtr;
    ytr = nytr;
    return 0;
}


Coord BinID2Coord::transform( const BinID& binid ) const
{
    return Coord( xtr.a + xtr.b*binid.inl + xtr.c*binid.crl,
		  ytr.a + ytr.b*binid.inl + ytr.c*binid.crl );
}


BinID BinID2Coord::transform( const Coord& coord,
				const StepInterval<int>* inlrg,
				const StepInterval<int>* crlrg) const
{
    static BinID binid;
    static double x, y;

    if ( mIsUndefined(coord.x) || mIsUndefined(coord.y) )
	return binid;

    double det = xtr.det( ytr );
    if ( mIsZero(det,mDefEps) ) return binid;

    x = coord.x - xtr.a;
    y = coord.y - ytr.a;
    double di = (x*ytr.c - y*xtr.c) / det;
    double dc = (y*xtr.b - x*ytr.b) / det;
    binid.inl = inlrg ? inlrg->snap(di) : mNINT(di);
    binid.crl = crlrg ? crlrg->snap(dc) : mNINT(dc);

    return binid;
}


SurveyInfo::SurveyInfo()
    : cs_(*new CubeSampling(false))
    , wcs_(*new CubeSampling(false))
    , valid_(false)
    , zistime_(true)
    , zinfeet_(false)
    , pars_(*new IOPar("Survey defaults"))
{
    rdxtr.b = rdytr.c = 1;
    set3binids[2].crl = 0;
}


SurveyInfo::SurveyInfo( const SurveyInfo& si )
    : cs_(*new CubeSampling(false))
    , wcs_(*new CubeSampling(false))
    , pars_(*new IOPar("Survey defaults"))
{
    *this = si;
}


SurveyInfo::~SurveyInfo()
{
    delete &pars_;
    delete &cs_;
    delete &wcs_;
}



SurveyInfo& SurveyInfo::operator =( const SurveyInfo& si )
{
    if ( &si == this ) return *this;

    setName( si.name() );
    valid_ = si.valid_;
    datadir = si.datadir;
    dirname = si.dirname;
    wsprojnm_ = si.wsprojnm_;
    wspwd_ = si.wspwd_;
    zistime_ = si.zistime_;
    zinfeet_ = si.zinfeet_;
    b2c_ = b2c_;
    for ( int idx=0; idx<3; idx++ )
    {
	set3binids[idx] = set3binids[idx];
	set3coords[idx] = set3coords[idx];
    }
    cs_ = si.cs_; wcs_ = si.wcs_; pars_ = si.pars_;

    return *this;
}


SurveyInfo* SurveyInfo::read( const char* survdir )
{
    FilePath fpsurvdir( survdir );
    FilePath fp( fpsurvdir ); fp.add( ".survey" );
    StreamData sd = StreamProvider( fp.fullPath() ).makeIStream();

    if ( !sd.istrm || sd.istrm->fail() )
    {
	sd.close();
	return new SurveyInfo;
    }

    ascistream astream( *sd.istrm );
    static bool errmsgdone = false;
    if ( !astream.isOfFileType(sKeySI) )
    {
	ErrMsg( "Survey definition file is corrupt!" );
	errmsgdone = true;
	sd.close();
	return new SurveyInfo;
    }
    errmsgdone = false;

    astream.next();
    BufferString keyw = astream.keyWord();
    SurveyInfo* si = new SurveyInfo;

    si->dirname = fpsurvdir.fileName();
    si->datadir = fpsurvdir.pathOnly();
    if ( !survdir || si->dirname == "" ) return si;

    while ( !atEndOfSection(astream) )
    {
	keyw = astream.keyWord();
	if ( keyw == sNameKey )
	    si->setName( astream.value() );
	else if ( keyw == sKeyWSProjName )
	    si->wsprojnm_ = astream.value();
	else if ( keyw == sKeyInlRange )
	{
	    FileMultiString fms( astream.value() );
	    si->cs_.hrg.start.inl = atoi(fms[0]);
	    si->cs_.hrg.stop.inl = atoi(fms[1]);
	    si->cs_.hrg.step.inl = atoi(fms[2]);
	}
	else if ( keyw == sKeyCrlRange )
	{
	    FileMultiString fms( astream.value() );
	    si->cs_.hrg.start.crl = atoi(fms[0]);
	    si->cs_.hrg.stop.crl = atoi(fms[1]);
	    si->cs_.hrg.step.crl = atoi(fms[2]);
	}
	else if ( keyw == sKeyZRange )
	{
	    FileMultiString fms( astream.value() );
	    si->cs_.zrg.start = atof(fms[0]);
	    si->cs_.zrg.stop = atof(fms[1]);
	    si->cs_.zrg.step = atof(fms[2]);
	    if ( mIsUndefined(si->cs_.zrg.step)
	      || mIsZero(si->cs_.zrg.step,mDefEps) )
		si->cs_.zrg.step = 0.004;
	    if ( fms.size() > 3 )
	    {
		si->zistime_ = *fms[3] == 'T';
		si->zinfeet_ = *fms[3] == 'F';
	    }
	}
	else
	    si->handleLineRead( keyw, astream.value() );

	astream.next();
    }
    si->cs_.normalise();
    si->wcs_ = si->cs_;

    char buf[1024];
    while ( astream.stream().getline(buf,1024) )
    {
	if ( *((const char*)si->comment_) )
	    si->comment_ += "\n";
	si->comment_ += buf;
    }
    sd.close();

    if ( si->wrapUpRead() )
	si->valid_ = true;

    fp = fpsurvdir; fp.add( ".defs" );
    si->pars().read( fp.fullPath() );
    si->pars().setName( "Survey defaults" );
    return si;
}


bool SurveyInfo::wrapUpRead()
{
    if ( set3binids[2].crl == 0 )
	get3Pts( set3coords, set3binids, set3binids[2].crl );
    b2c_.setTransforms( rdxtr, rdytr );
    return b2c_.isValid();
}


void SurveyInfo::handleLineRead( const BufferString& keyw, const char* val )
{
    if ( keyw == sKeyXTransf )
	setTr( rdxtr, val );
    else if ( keyw == sKeyYTransf )
	setTr( rdytr, val );
    else if ( matchString("Set Point",(const char*)keyw) )
    {
	const char* ptr = strchr( (const char*)keyw, '.' );
	if ( !ptr ) return;
	int ptidx = atoi( ptr + 1 ) - 1;
	if ( ptidx < 0 ) ptidx = 0;
	if ( ptidx > 3 ) ptidx = 2;
	FileMultiString fms( val );
	if ( fms.size() < 2 ) return;
	set3binids[ptidx].use( fms[0] );
	set3coords[ptidx].use( fms[1] );
    }
}


StepInterval<int> SurveyInfo::inlRange( bool work ) const
{
    StepInterval<int> ret; Interval<int> dum;
    sampling(work).hrg.get( ret, dum );
    return ret;
}


StepInterval<int> SurveyInfo::crlRange( bool work ) const
{
    StepInterval<int> ret; Interval<int> dum;
    sampling(work).hrg.get( dum, ret );
    return ret;
}


const StepInterval<float>& SurveyInfo::zRange( bool work ) const
{ return sampling(work).zrg; }
int SurveyInfo::inlStep( bool work ) const
{ return sampling(work).hrg.step.inl; }
int SurveyInfo::crlStep( bool work ) const
{ return sampling(work).hrg.step.crl; }
int SurveyInfo::maxNrTraces( bool work ) const
{ return sampling(work).hrg.nrInl() * sampling(work).hrg.nrCrl(); }


void SurveyInfo::setRange( const CubeSampling& cs, bool work )
{
    if ( work )
	wcs_ = cs;
    else
	cs_ = cs;
    wcs_.limitTo( cs_ );
}


bool SurveyInfo::isReasonable( const BinID& b ) const
{
    BinID w( cs_.hrg.stop.inl - cs_.hrg.start.inl,
             cs_.hrg.stop.crl - cs_.hrg.start.crl );
    return b.inl > cs_.hrg.start.inl - w.inl
	&& b.inl < cs_.hrg.stop.inl  + w.crl
	&& b.crl > cs_.hrg.start.crl - w.crl
	&& b.crl < cs_.hrg.stop.crl  + w.crl;
}


bool SurveyInfo::isReasonable( const Coord& crd ) const
{
    if ( mIsUndefined(crd.x) || mIsUndefined(crd.y) )
	return false;

    return isReasonable( transform(crd) ); 
}


#define mChkCoord(c) \
    if ( c.x < minc.x ) minc.x = c.x; if ( c.y < minc.y ) minc.y = c.y;

Coord SurveyInfo::minCoord( bool work ) const
{
    const CubeSampling& cs = sampling(work);
    Coord minc = transform( cs.hrg.start );
    Coord c = transform( cs.hrg.stop );
    mChkCoord(c);
    BinID bid( cs.hrg.start.inl, cs.hrg.stop.crl );
    c = transform( bid );
    mChkCoord(c);
    bid = BinID( cs.hrg.stop.inl, cs.hrg.start.crl );
    c = transform( bid );
    mChkCoord(c);
    return minc;
}


#undef mChkCoord
#define mChkCoord(c) \
    if ( c.x > maxc.x ) maxc.x = c.x; if ( c.y > maxc.y ) maxc.y = c.y;

Coord SurveyInfo::maxCoord( bool work ) const
{
    const CubeSampling& cs = sampling(work);
    Coord maxc = transform( cs.hrg.start );
    Coord c = transform( cs.hrg.stop );
    mChkCoord(c);
    BinID bid( cs.hrg.start.inl, cs.hrg.stop.crl );
    c = transform( bid );
    mChkCoord(c);
    bid = BinID( cs.hrg.stop.inl, cs.hrg.start.crl );
    c = transform( bid );
    mChkCoord(c);
    return maxc;
}


void SurveyInfo::checkInlRange( Interval<int>& intv, bool work ) const
{
    const CubeSampling& cs = sampling(work);
    intv.sort();
    if ( intv.start < cs.hrg.start.inl ) intv.start = cs.hrg.start.inl;
    if ( intv.start > cs.hrg.stop.inl )  intv.start = cs.hrg.stop.inl;
    if ( intv.stop > cs.hrg.stop.inl )   intv.stop = cs.hrg.stop.inl;
    if ( intv.stop < cs.hrg.start.inl )  intv.stop = cs.hrg.start.inl;
    BinID bid( intv.start, 0 );
    snap( bid, BinID(1,1), work ); intv.start = bid.inl;
    bid.inl = intv.stop; snap( bid, BinID(-1,-1), work ); intv.stop = bid.inl;
}

void SurveyInfo::checkCrlRange( Interval<int>& intv, bool work ) const
{
    const CubeSampling& cs = sampling(work);
    intv.sort();
    if ( intv.start < cs.hrg.start.crl ) intv.start = cs.hrg.start.crl;
    if ( intv.start > cs.hrg.stop.crl )  intv.start = cs.hrg.stop.crl;
    if ( intv.stop > cs.hrg.stop.crl )   intv.stop = cs.hrg.stop.crl;
    if ( intv.stop < cs.hrg.start.crl )  intv.stop = cs.hrg.start.crl;
    BinID bid( 0, intv.start );
    snap( bid, BinID(1,1), work ); intv.start = bid.crl;
    bid.crl = intv.stop; snap( bid, BinID(-1,-1), work ); intv.stop = bid.crl;
}



void SurveyInfo::checkZRange( Interval<float>& intv, bool work ) const
{
    const StepInterval<float>& rg = sampling(work).zrg;
    intv.sort();
    if ( intv.start < rg.start ) intv.start = rg.start;
    if ( intv.start > rg.stop )  intv.start = rg.stop;
    if ( intv.stop > rg.stop )   intv.stop = rg.stop;
    if ( intv.stop < rg.start )  intv.stop = rg.start;
    snapZ( intv.start, 1, work );
    snapZ( intv.stop, -1, work );
}


bool SurveyInfo::includes( const BinID& bid, const float z, bool work ) const
{
    const CubeSampling& cs = sampling(work);
    const float eps = 1e-8;
    return cs.hrg.includes( bid )
	&& cs.zrg.start < z + eps && cs.zrg.stop > z - eps;
}


const char* SurveyInfo::getZUnit( bool wb ) const
{
    static BufferString lbl;
    lbl = wb ? "(" : "";
    if ( zistime_ )
	lbl += "ms";
    else if ( zinfeet_ )
	lbl += "ft";
    else
	lbl += "m";
    if ( wb ) lbl += ")";
    
    return lbl.buf();
}


void SurveyInfo::setZUnit( bool istime, bool meter )
{
    zistime_ = istime;
    zinfeet_ = meter;
}


bool SurveyInfo::depthsInFeetByDefault() const
{
    bool ret = zIsTime() ? false : zInFeet();
    pars().getYN( sKeyDpthInFt, ret );
    return ret;
}


BinID SurveyInfo::transform( const Coord& c ) const
{
    if ( !valid_ ) return BinID(0,0);
    static StepInterval<int> inlrg, crlrg;
    cs_.hrg.get( inlrg, crlrg );
    return b2c_.transform( c, &inlrg, &crlrg );
}


void SurveyInfo::get3Pts( Coord c[3], BinID b[2], int& xline ) const
{
    if ( set3binids[0].inl )
    {
	b[0] = set3binids[0]; c[0] = set3coords[0];
	b[1] = set3binids[1]; c[1] = set3coords[1];
	c[2] = set3coords[2]; xline = set3binids[2].crl;
    }
    else
    {
	b[0] = cs_.hrg.start; c[0] = transform( b[0] );
	b[1] = cs_.hrg.stop; c[1] = transform( b[1] );
	BinID b2 = cs_.hrg.stop; b2.inl = b[0].inl;
	c[2] = transform( b2 ); xline = b2.crl;
    }
}


const char* SurveyInfo::set3Pts( const Coord c[3], const BinID b[2],
				   int xline )
{
    const char* msg = b2c_.set3Pts( c, b, xline );
    if ( msg ) return msg;

    set3coords[0].x = c[0].x; set3coords[0].y = c[0].y;
    set3coords[1].x = c[1].x; set3coords[1].y = c[1].y;
    set3coords[2].x = c[2].x; set3coords[2].y = c[2].y;
    set3binids[0] = transform( set3coords[0] );
    set3binids[1] = transform( set3coords[1] );
    set3binids[2] = transform( set3coords[2] );

    return 0;
}


static void doSnap( int& idx, int start, int step, int dir )
{
    if ( step < 2 ) return;
    int rel = idx - start;
    int rest = rel % step;
    if ( !rest ) return;
 
    idx -= rest;
 
    if ( !dir ) dir = rest > step / 2 ? 1 : -1;
    if ( rel > 0 && dir > 0 )      idx += step;
    else if ( rel < 0 && dir < 0 ) idx -= step;
}


void SurveyInfo::snap( BinID& binid, BinID rounding, bool work ) const
{
    const CubeSampling& cs = sampling(work);
    const BinID& stp = cs.hrg.step;
    if ( stp.inl == 1 && stp.crl == 1 ) return;
    doSnap( binid.inl, cs.hrg.start.inl, stp.inl, rounding.inl );
    doSnap( binid.crl, cs.hrg.start.crl, stp.crl, rounding.crl );
}


void SurveyInfo::snapStep( BinID& s, BinID rounding, bool work ) const
{
    const BinID& stp = sampling(work).hrg.step;
    if ( s.inl < 0 ) s.inl = -s.inl;
    if ( s.crl < 0 ) s.crl = -s.crl;
    if ( s.inl < stp.inl ) s.inl = stp.inl;
    if ( s.crl < stp.crl ) s.crl = stp.crl;
    if ( s == stp || (stp.inl == 1 && stp.crl == 1) )
	return;

    int rest;
#define mSnapStep(ic) \
    rest = s.ic % stp.ic; \
    if ( rest ) \
    { \
	int hstep = stp.ic / 2; \
	bool upw = rounding.ic > 0 || (rounding.ic == 0 && rest > hstep); \
	s.ic -= rest; \
	if ( upw ) s.ic += stp.ic; \
    }

    mSnapStep(inl)
    mSnapStep(crl)
}


void SurveyInfo::snapZ( float& z, int dir, bool work ) const
{
    const StepInterval<float>& zrg = sampling(work).zrg;
    if ( z < zrg.start ) z = zrg.stop; if ( z > zrg.stop ) z = zrg.stop;
    int idx = zrg.nearestIndex(z);
    float newz = zrg.atIndex(idx);
    if ( dir == 1 && newz < z-1e-8 )
	newz = zrg.atIndex(idx+1);
    if ( dir == -1 && newz > z+1e-8 )
	newz = zrg.atIndex(idx-1);
    z = newz;
    if ( z < zrg.start ) z = zrg.stop; if ( z > zrg.stop ) z = zrg.stop;
}


void SurveyInfo::setTr( BinID2Coord::BCTransform& tr, const char* str )
{
    FileMultiString fms( str );
    tr.a = atof(fms[0]); tr.b = atof(fms[1]); tr.c = atof(fms[2]);
}


void SurveyInfo::putTr( const BinID2Coord::BCTransform& tr,
			  ascostream& astream, const char* key ) const
{
    FileMultiString fms;
    fms += tr.a; fms += tr.b; fms += tr.c;
    astream.put( key, fms );
}


bool SurveyInfo::isClockWise() const
{
    float xinl = b2c_.getTransform(true).b;
    float xcrl = b2c_.getTransform(true).c;
    float yinl = b2c_.getTransform(false).b;
    float ycrl = b2c_.getTransform(false).c;

    float det = xinl*ycrl - xcrl*yinl;
    return det < 0;
}


bool SurveyInfo::write( const char* basedir ) const
{
    if ( !valid_ ) return false;
    if ( !basedir ) basedir = GetDataDir();

    FilePath fp( basedir ); fp.add( dirname ).add( ".survey" );
    StreamData sd = StreamProvider( fp.fullPath() ).makeOStream();
    if ( !sd.ostrm || !sd.usable() ) { sd.close(); return false; }

    std::ostream& strm = *sd.ostrm;
    ascostream astream( strm );
    if ( !astream.putHeader(sKeySI) ) { sd.close(); return false; }

    astream.put( sNameKey, name() );
    FileMultiString fms;
    fms += cs_.hrg.start.inl; fms += cs_.hrg.stop.inl; fms += cs_.hrg.step.inl;
    astream.put( sKeyInlRange, fms );
    fms = "";
    fms += cs_.hrg.start.crl; fms += cs_.hrg.stop.crl; fms += cs_.hrg.step.crl;
    astream.put( sKeyCrlRange, fms );
    fms = ""; fms += cs_.zrg.start; fms += cs_.zrg.stop;
    fms += cs_.zrg.step; fms += zistime_ ? "T" : ( zinfeet_ ? "F" : "D" );
    astream.put( sKeyZRange, fms );

    if ( wsprojnm_ != "" )
	astream.put( sKeyWSProjName, wsprojnm_ );

    writeSpecLines( astream );

    astream.newParagraph();
    const char* ptr = (const char*)comment_;
    if ( *ptr )
    {
	while ( 1 )
	{
	    char* nlptr = const_cast<char*>( strchr( ptr, '\n' ) );
	    if ( !nlptr )
	    {
		if ( *ptr )
		    strm << ptr;
		break;
	    }
	    *nlptr = '\0';
	    strm << ptr << '\n';
	    *nlptr = '\n';
	    ptr = nlptr + 1;
	}
	strm << '\n';
    }

    bool retval = !strm.fail();
    sd.close();

    savePars( basedir );
    return retval;
}


void SurveyInfo::writeSpecLines( ascostream& astream ) const
{
    putTr( b2c_.getTransform(true), astream, sKeyXTransf );
    putTr( b2c_.getTransform(false), astream, sKeyYTransf );
    FileMultiString fms;
    for ( int idx=0; idx<3; idx++ )
    {
	SeparString ky( "Set Point", '.' );
	ky += idx + 1;
	char buf[80];
	set3binids[idx].fill( buf ); fms = buf;
	set3coords[idx].fill( buf ); fms += buf;
	astream.put( (const char*)ky, (const char*)fms );
    }
}


void SurveyInfo::savePars( const char* basedir ) const
{
    if ( !pars_.size() ) return;

    if ( !basedir || !*basedir )
	basedir = GetDataDir();

    pars_.dump( FilePath(basedir).add(".defs").fullPath() );
}
