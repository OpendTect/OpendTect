/*+
 * COPYRIGHT: (C) de Groot-Bril Earth Sciences B.V.
 * AUTHOR   : A.H. Bril
 * DATE     : 18-4-1996
-*/

static const char* rcsID = "$Id: survinfo.cc,v 1.39 2003-03-18 16:04:08 nanne Exp $";

#include "survinfoimpl.h"
#include "ascstream.h"
#include "filegen.h"
#include "separstr.h"
#include "errh.h"
#include "strmprov.h"
#include "iopar.h"
#include "keystrs.h"

static const char* sKeySI = "Survey Info";
static const char* sKeyXTransf = "Coord-X-BinID";
static const char* sKeyYTransf = "Coord-Y-BinID";
const char* SurveyInfo::sKeyInlRange = "In-line range";
const char* SurveyInfo::sKeyCrlRange = "Cross-line range";
const char* SurveyInfo::sKeyZRange = "Z range";
const char* SurveyInfo::sKeyWSProjName = "Workstation Project Name";

SurveyInfo* SurveyInfo::theinst_;
bool SurveyInfo::dowarnings_ = true;

const SurveyInfo& SI()
{
    if ( !SurveyInfo::theinst_ || !SurveyInfo::theinst_->valid_ )
    {
	if ( SurveyInfo::theinst_ )
	    delete SurveyInfo::theinst_;
	SurveyInfo::theinst_ = SurveyInfo::read( GetDataDir() );
    }
    return *SurveyInfo::theinst_;
}


const char* BinID2Coord::set3Pts( const Coord c[3], const BinID b[2], int xline)
{
    if ( b[1].inl == b[0].inl )
        return "Need two different in-lines";
    if ( b[0].crl == xline )
        return "The X-line cannot be the same";

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

    if ( mIS_ZERO(nxtr.a) ) nxtr.a = 0; if ( mIS_ZERO(nxtr.b) ) nxtr.b = 0;
    if ( mIS_ZERO(nxtr.c) ) nxtr.c = 0; if ( mIS_ZERO(nytr.a) ) nytr.a = 0;
    if ( mIS_ZERO(nytr.b) ) nytr.b = 0; if ( mIS_ZERO(nytr.c) ) nytr.c = 0;

    if ( !nxtr.valid(nytr) )
	return "The transformation would not be valid";

    xtr = nxtr;
    ytr = nytr;
    return 0;
}


Coord BinID2Coord::transform( const BinID& binid ) const
{
    return Coord( xtr.a + xtr.b*binid.inl + xtr.c*binid.crl,
		  ytr.a + ytr.b*binid.inl + ytr.c*binid.crl );
}


BinID BinID2Coord::transform( const Coord& coord ) const
{
    static BinID binid;
    static double x, y;

    double det = xtr.det( ytr );
    if ( mIS_ZERO(det) ) return binid;

    x = coord.x - xtr.a;
    y = coord.y - ytr.a;
    double di = (x*ytr.c - y*xtr.c) / det;
    double dc = (y*xtr.b - x*ytr.b) / det;
    binid.inl = mNINT(di); binid.crl = mNINT(dc);

    return binid;
}


SurveyInfo::SurveyInfo()
    : valid_(false)
    , zistime_(true)
    , zinmeter_(false)
    , zinfeet_(false)
    , pars_(*new IOPar("Survey defaults"))
{
}


SurveyInfo::~SurveyInfo()
{
    delete &pars_;
}


SurveyInfo3D::SurveyInfo3D()
    : step_(1,1)
    , wstep_(1,1)
{
    rdxtr.b = rdytr.c = 1;
    set3binids[2].crl = 0;
}


void SurveyInfo::copyFrom( const SurveyInfo& si )
{
    datadir = si.datadir;
    dirname = si.dirname;
    range_ = si.range_; wrange_ = si.wrange_;
    zrange_ = si.zrange_; wzrange_ = si.wzrange_;
    setName( si.name() );
    valid_ = si.valid_;
    wsprojnm_ = si.wsprojnm_;
    wspwd_ = si.wspwd_;
    zistime_ = si.zistime_;
}


void SurveyInfo3D::copyFrom( const SurveyInfo& si )
{
    SurveyInfo::copyFrom( si );
    mDynamicCastGet(const SurveyInfo3D*,si3d,(&si))
    if ( !si3d ) return;

    step_ = si3d->step_; wstep_ = si3d->wstep_;
    b2c_ = si3d->b2c_;
    for ( int idx=0; idx<3; idx++ )
    {
	set3binids[idx] = si3d->set3binids[idx];
	set3coords[idx] = si3d->set3coords[idx];
    }
}


SurveyInfo* SurveyInfo::read( const char* survdir )
{
    FileNameString fname( File_getFullPath( survdir, ".survey" ) );
    StreamData sd = StreamProvider( fname ).makeIStream();

    if ( !sd.istrm || sd.istrm->fail() )
    {
	sd.close();
	return new SurveyInfo3D;
    }

    ascistream astream( *sd.istrm );
    static bool errmsgdone = false;
    if ( !astream.isOfFileType(sKeySI) )
    {
	ErrMsg( "Survey definition file is corrupt!" );
	errmsgdone = true;
	sd.close();
	return new SurveyInfo3D;
    }
    errmsgdone = false;

    astream.next();
    BufferString keyw = astream.keyWord();
    SurveyInfo* si = keyw == sKey::Type && *astream.value() == '2'
		   ? (SurveyInfo*) new SurveyInfo2D
		   : (SurveyInfo*) new SurveyInfo3D;

    si->datadir = File_getPathOnly( survdir );
    si->dirname = File_getFileName( survdir );
    if ( !survdir || si->dirname == "" ) return si;

    BinIDRange bir; BinID bid( 1, 1 );
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
	    bir.start.inl = atoi(fms[0]);
	    bir.stop.inl = atoi(fms[1]);
	    bid.inl = atoi(fms[2]);
	}
	else if ( keyw == sKeyCrlRange )
	{
	    FileMultiString fms( astream.value() );
	    bir.start.crl = atoi(fms[0]);
	    bir.stop.crl = atoi(fms[1]);
	    bid.crl = atoi(fms[2]);
	}
	else if ( keyw == sKeyZRange )
	{
	    FileMultiString fms( astream.value() );
	    si->zrange_.start = atof(fms[0]);
	    si->zrange_.stop = atof(fms[1]);
	    si->zrange_.step = atof(fms[2]);
	    if ( mIsUndefined(si->zrange_.step) || mIS_ZERO(si->zrange_.step) )
		si->zrange_.step = 0.004;
	    if ( fms.size() > 3 )
	    {
		si->zistime_ = *fms[3] == 'T';
		si->zinmeter_ = *fms[3] == 'D';
		si->zinfeet_ = *fms[3] == 'F';
	    }
	    si->zrange_.sort();
	}
	else
	    si->handleLineRead( keyw, astream.value() );

	astream.next();
    }

    char buf[1024];
    while ( astream.stream().getline(buf,1024) )
    {
	if ( *((const char*)si->comment_) )
	    si->comment_ += "\n";
	si->comment_ += buf;
    }
    sd.close();

    si->setRange( bir, false );
    si->setStep( bid, false );
    if ( si->wrapUpRead() )
	si->valid_ = true;
    si->wrange_ = si->range_;
    si->wzrange_ = si->zrange_;

    fname = File_getFullPath( survdir, ".defs" );
    si->pars().read( fname );
    si->pars().setName( "Survey defaults" );
    return si;
}


bool SurveyInfo3D::wrapUpRead()
{
    wstep_ = step_;
    if ( set3binids[2].crl == 0 )
	get3Pts( set3coords, set3binids, set3binids[2].crl );
    b2c_.setTransforms( rdxtr, rdytr );
    return b2c_.isValid();
}


void SurveyInfo3D::handleLineRead( const BufferString& keyw, const char* val )
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


bool SurveyInfo::write( const char* basedir ) const
{
    if ( !valid_ ) return false;

    FileNameString fname( File_getFullPath(basedir,dirname) );
    fname = File_getFullPath( fname, ".survey" );

    StreamData sd = StreamProvider( fname ).makeOStream();
    if ( !sd.ostrm || !sd.usable() ) { sd.close(); return false; }

    ostream& strm = *sd.ostrm;
    ascostream astream( strm );
    if ( !astream.putHeader(sKeySI) ) { sd.close(); return false; }

    astream.put( sNameKey, name() );
    FileMultiString fms;
    fms += range_.start.inl; fms += range_.stop.inl;
    if ( haveStep() ) fms += getStep( true, false );
    astream.put( sKeyInlRange, fms );
    fms = ""; fms += range_.start.crl; fms += range_.stop.crl;
    if ( haveStep() ) fms += getStep( false, false );
    astream.put( sKeyCrlRange, fms );
    if ( !mIS_ZERO(zrange_.width()) )
    {
	fms = ""; fms += zrange_.start; fms += zrange_.stop;
	fms += zrange_.step; fms += zistime_ ? "T" : ( zinmeter_ ? "D" : "F" );
	astream.put( sKeyZRange, fms );
    }
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
    if ( retval )
	retval = wrapUpWrite( strm, basedir );
    sd.close();

    savePars( basedir );
    return retval;
}


void SurveyInfo::savePars( const char* basedir ) const
{
    if ( !pars_.size() ) return;

    if ( !basedir || !*basedir )
	basedir = GetDataDir();

    BufferString fname( basedir );
    fname = File_getFullPath( fname, ".defs" );
    pars_.dump( fname );
}


void SurveyInfo3D::writeSpecLines( ascostream& astream ) const
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


void SurveyInfo::setRange( const BinIDRange& br, bool work )
{
    BinIDRange& rg = work ? wrange_ : range_;
    rg.start = br.start;
    rg.stop = br.stop;
    if ( br.start.inl > br.stop.inl )
    {
	rg.start.inl = br.stop.inl;
	rg.stop.inl = br.start.inl;
    }
    if ( br.start.crl > br.stop.crl )
    {
	rg.start.crl = br.stop.crl;
	rg.stop.crl = br.start.crl;
    }
}


void SurveyInfo::setZRange( const StepInterval<double>& zr, bool work )
{
    StepInterval<double>& rg = work ? wzrange_ : zrange_;
    double prevstep = rg.step;
    rg = zr;
    if ( (rg.step < 1e-15 && rg.step > -1e15)
      || mIsUndefined(rg.step) )
	rg.step = prevstep;

    rg.sort();
}


void SurveyInfo::setZRange( const Interval<double>& zr, bool work )
{
    StepInterval<double> newzr( zr.start, zr.stop,
	    			(work ? wzrange_ : zrange_).step );
    setZRange( newzr, work );
}


bool SurveyInfo::isReasonable( const BinID& b ) const
{
    BinID w( range_.stop.inl - range_.start.inl,
             range_.stop.crl - range_.start.crl );
    return b.inl > range_.start.inl - w.inl
	&& b.inl < range_.stop.inl  + w.crl
	&& b.crl > range_.start.crl - w.crl
	&& b.crl < range_.stop.crl  + w.crl;
}


#define mChkCoord(c) \
    if ( c.x < minc.x ) minc.x = c.x; if ( c.y < minc.y ) minc.y = c.y;

Coord SurveyInfo::minCoord( bool work ) const
{
    const BinIDRange& rg = work ? wrange_ : range_;

    Coord minc = transform( rg.start );
    Coord c = transform( rg.stop );
    mChkCoord(c);

    BinID bid( rg.start.inl, rg.stop.crl );
    c = transform( bid );
    mChkCoord(c);

    bid = BinID( rg.stop.inl, rg.start.crl );
    c = transform( bid );
    mChkCoord(c);

    return minc;
}


#undef mChkCoord
#define mChkCoord(c) \
    if ( c.x < maxc.x ) maxc.x = c.x; if ( c.y < maxc.y ) maxc.y = c.y;

Coord SurveyInfo::maxCoord( bool work ) const
{
    const BinIDRange& rg = work ? wrange_ : range_;

    Coord maxc = transform( rg.start );
    Coord c = transform( rg.stop );
    mChkCoord(c);

    BinID bid( rg.start.inl, rg.stop.crl );
    c = transform( bid );
    mChkCoord(c);

    bid = BinID( rg.stop.inl, rg.start.crl );
    c = transform( bid );
    mChkCoord(c);

    return maxc;
}


void SurveyInfo::checkInlRange( Interval<int>& intv, bool work ) const
{
    const BinIDRange& rg = work ? wrange_ : range_;
    intv.sort();
    if ( intv.start < rg.start.inl ) intv.start = rg.start.inl;
    if ( intv.start > rg.stop.inl )  intv.start = rg.stop.inl;
    if ( intv.stop > rg.stop.inl )   intv.stop = rg.stop.inl;
    if ( intv.stop < rg.start.inl )  intv.stop = rg.start.inl;
}

void SurveyInfo::checkCrlRange( Interval<int>& intv, bool work ) const
{
    const BinIDRange& rg = work ? wrange_ : range_;
    intv.sort();
    if ( intv.start < rg.start.crl ) intv.start = rg.start.crl;
    if ( intv.start > rg.stop.crl )  intv.start = rg.stop.crl;
    if ( intv.stop > rg.stop.crl )   intv.stop = rg.stop.crl;
    if ( intv.stop < rg.start.crl )  intv.stop = rg.start.crl;
}

void SurveyInfo::checkRange( BinIDRange& chk, bool work ) const
{
    const BinIDRange& rg = work ? wrange_ : range_;
    if ( chk.start.inl < rg.start.inl ) chk.start.inl = rg.start.inl;
    if ( chk.start.inl > rg.stop.inl )  chk.start.inl = rg.stop.inl;
    if ( chk.stop.inl > rg.stop.inl )   chk.stop.inl = rg.stop.inl;
    if ( chk.stop.inl < rg.start.inl )  chk.stop.inl = rg.start.inl;
    if ( chk.start.crl < rg.start.crl ) chk.start.crl = rg.start.crl;
    if ( chk.start.crl > rg.stop.crl )  chk.start.crl = rg.stop.crl;
    if ( chk.stop.crl > rg.stop.crl )   chk.stop.crl = rg.stop.crl;
    if ( chk.stop.crl < rg.start.crl )  chk.stop.crl = rg.start.crl;
    snap( chk.start, BinID(1,1), false );
    snap( chk.stop, BinID(-1,-1), false );
}


void SurveyInfo::checkZRange( Interval<double>& intv, bool work ) const
{
    const StepInterval<double>& rg = work ? wzrange_ : zrange_;
    intv.sort();
    if ( intv.start < rg.start ) intv.start = rg.start;
    if ( intv.start > rg.stop )  intv.start = rg.stop;
    if ( intv.stop > rg.stop )   intv.stop = rg.stop;
    if ( intv.stop < rg.start )  intv.stop = rg.start;
}


bool SurveyInfo::includes( const BinID bid, const float z, bool work ) const
{
    const BinIDRange& rg = work ? wrange_ : range_;
    bool bidin = !rg.excludes( bid );

    const StepInterval<double>& zrg = work ? wzrange_ : zrange_;
    bool zin = zrg.includes( z );

    return bidin && zin;
}


const char* SurveyInfo::getZUnit() const
{
    if ( zistime_ )
	return "ms";
    else if ( zinfeet_ )
	return "ft";
    else
	return "m";
}


void SurveyInfo::setZUnit( bool istime, bool meter )
{
    zistime_ = istime;
    zinmeter_ = istime ? false : meter;
    zinfeet_ = istime ? false : !meter;
}


BinID SurveyInfo3D::transform( const Coord& c ) const
{
    if ( !valid_ ) return BinID(0,0);
    BinID binid = b2c_.transform( c );

    if ( step_.inl > 1 )
    {
	float relinl = binid.inl - range_.start.inl;
	int nrsteps = (int)(relinl/step_.inl + (relinl>0?.5:-.5));
	binid.inl = range_.start.inl + nrsteps*step_.inl;
    }
    if ( step_.crl > 1 )
    {
	float relcrl = binid.crl - range_.start.crl;
	int nrsteps = (int)( relcrl / step_.crl + (relcrl>0?.5:-.5));
	binid.crl = range_.start.crl + nrsteps*step_.crl;
    }

    return binid;
}


void SurveyInfo3D::get3Pts( Coord c[3], BinID b[2], int& xline ) const
{
    if ( set3binids[0].inl )
    {
	b[0] = set3binids[0]; c[0] = set3coords[0];
	b[1] = set3binids[1]; c[1] = set3coords[1];
	c[2] = set3coords[2]; xline = set3binids[2].crl;
    }
    else
    {
	b[0] = range_.start; c[0] = transform( b[0] );
	b[1] = range_.stop; c[1] = transform( b[1] );
	BinID b2 = range_.stop; b2.inl = b[0].inl;
	c[2] = transform( b2 ); xline = b2.crl;
    }
}


const char* SurveyInfo3D::set3Pts( const Coord c[3], const BinID b[2],
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


void SurveyInfo3D::snap( BinID& binid, BinID rounding, bool work ) const
{
    const BinIDRange& rg = work ? wrange_ : range_;
    const BinID& stp = work ? wstep_ : step_;
    if ( stp.inl == 1 && stp.crl == 1 ) return;
    doSnap( binid.inl, rg.start.inl, stp.inl, rounding.inl );
    doSnap( binid.crl, rg.start.crl, stp.crl, rounding.crl );
}


void SurveyInfo3D::snapStep( BinID& s, BinID rounding, bool work ) const
{
    const BinID& stp = work ? wstep_ : step_;
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


void SurveyInfo3D::setStep( const BinID& bid, bool work )
{
    BinID& stp = work ? wstep_ : step_;
    stp = bid;
    if ( !stp.inl ) stp.inl = 1; if ( !stp.crl ) stp.crl = 1;
    if ( stp.inl < 0 ) stp.inl = -stp.inl;
    if ( stp.crl < 0 ) stp.crl = -stp.crl;
}


void SurveyInfo3D::setTr( BinID2Coord::BCTransform& tr, const char* str )
{
    FileMultiString fms( str );
    tr.a = atof(fms[0]); tr.b = atof(fms[1]); tr.c = atof(fms[2]);
}


void SurveyInfo3D::putTr( const BinID2Coord::BCTransform& tr,
			  ascostream& astream, const char* key ) const
{
    FileMultiString fms;
    fms += tr.a; fms += tr.b; fms += tr.c;
    astream.put( key, fms );
}



