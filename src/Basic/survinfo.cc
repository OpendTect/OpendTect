/*+
 * COPYRIGHT: (C) de Groot-Bril Earth Sciences B.V.
 * AUTHOR   : A.H. Bril
 * DATE     : 18-4-1996
-*/

static const char* rcsID = "$Id: survinfo.cc,v 1.25 2002-06-21 16:02:41 bert Exp $";

#include "survinfo.h"
#include "ascstream.h"
#include "filegen.h"
#include "separstr.h"
#include "errh.h"
#include "strmprov.h"

static const char* sKey = "Survey Info";
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
	SurveyInfo::theinst_ = new SurveyInfo( GetDataDir() );
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


SurveyInfo::SurveyInfo( const SurveyInfo& si )
{
    b2c_ = si.b2c_;
    dirname = si.dirname;
    range_ = si.range_; wrange_ = si.wrange_;
    zrange_ = si.zrange_; wzrange_ = si.wzrange_;
    step_ = si.step_; wstep_ = si.wstep_;
    setName( si.name() );
    valid_ = si.valid_;
    for ( int idx=0; idx<3; idx++ )
    {
	set3binids[idx] = si.set3binids[idx];
	set3coords[idx] = si.set3coords[idx];
    }
    wsprojnm_ = si.wsprojnm_;
    wspwd_ = si.wspwd_;
    zistime_ = si.zistime_;
}


SurveyInfo::SurveyInfo( const char* rootdir )
	: dirname(File_getFileName(rootdir))
    	, valid_(false)
    	, zistime_(true)
{
    set3binids[2].crl = 0;
    if ( !rootdir || dirname == "" ) return;

    FileNameString fname( File_getFullPath( rootdir, ".survey" ) );
    StreamData sd = StreamProvider( fname ).makeIStream();

    static bool errmsgdone = false;
    if ( !sd.istrm || sd.istrm->fail() )
    {
	if ( !errmsgdone && dowarnings_ )
	    ErrMsg( "No available survey definition." );
	errmsgdone = true;
	sd.close();
	return;
    }
    ascistream astream( *sd.istrm );
    if ( !astream.isOfFileType(sKey) )
    {
	ErrMsg( "Survey definition file is corrupt!" );
	errmsgdone = true;
	sd.close();
	return;
    }
    errmsgdone = false;

    BinID2Coord::BCTransform xtr, ytr;
    xtr.b = 1;
    ytr.c = 1;
    BinIDRange bir; BinID bid( 1, 1 );
    while ( !atEndOfSection( astream.next() ) )
    {
	BufferString keyw = astream.keyWord();
	if ( keyw == sNameKey )
	    setName( astream.value() );
	else if ( keyw == "Coord-X-BinID" )
	    setTr( xtr, astream.value() );
	else if ( keyw == "Coord-Y-BinID" )
	    setTr( ytr, astream.value() );
	else if ( keyw == sKeyWSProjName )
	    wsprojnm_ = astream.value();
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
	    zrange_.start = atof(fms[0]);
	    zrange_.stop = atof(fms[1]);
	    zrange_.step = atof(fms[2]);
	    if ( mIsUndefined(zrange_.step) || mIS_ZERO(zrange_.step) )
		zrange_.step = 0.004;
	    if ( fms.size() > 3 )
		zistime_ = *fms[3] != 'D';
	    zrange_.sort();
	}
	else if ( matchString("Set Point",astream.keyWord()) )
	{
	    const char* ptr = strchr( astream.keyWord(), '.' );
	    if ( !ptr ) continue;
	    int ptidx = atoi( ptr + 1 ) - 1;
	    if ( ptidx < 0 ) ptidx = 0;
	    if ( ptidx > 3 ) ptidx = 2;
	    FileMultiString fms( astream.value() );
	    if ( fms.size() < 2 ) continue;
	    set3binids[ptidx].use( fms[0] );
	    set3coords[ptidx].use( fms[1] );
	}
    }

    char buf[1024];
    while ( astream.stream().getline(buf,1024) )
    {
	if ( *((const char*)comment_) )
	    comment_ += "\n";
	comment_ += buf;
    }
    sd.close();

    if ( set3binids[2].crl == 0 )
	get3Pts( set3coords, set3binids, set3binids[2].crl );

    setRange( bir, false );
    setStep( bid, false );
    b2c_.setTransforms( xtr, ytr );
    valid_ = true;
    wrange_ = range_; wstep_ = step_; wzrange_ = zrange_;
}


int SurveyInfo::write( const char* basedir ) const
{
    if ( !valid_ ) return NO;

    FileNameString fname( File_getFullPath(basedir,dirname) );
    fname = File_getFullPath( fname, ".survey" );

    StreamData sd = StreamProvider( fname ).makeOStream();

    if ( !sd.ostrm || !sd.usable() ) { sd.close(); return NO; }

    ostream& strm = *sd.ostrm;

    ascostream astream( strm );
    if ( !astream.putHeader(sKey) ) { sd.close(); return NO; }

    astream.put( sNameKey, name() );
    putTr( b2c_.getTransform(true), astream, "Coord-X-BinID" );
    putTr( b2c_.getTransform(false), astream, "Coord-Y-BinID" );

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

    fms = "";
    fms += range_.start.inl; fms += range_.stop.inl; fms += step_.inl;
    astream.put( sKeyInlRange, fms );
    fms = ""; fms += range_.start.crl; fms += range_.stop.crl; fms += step_.crl;
    astream.put( sKeyCrlRange, fms );
    if ( !mIS_ZERO(zrange_.width()) )
    {
	fms = ""; fms += zrange_.start; fms += zrange_.stop;
	fms += zrange_.step; fms += zistime_ ? "T" : "D";
	astream.put( sKeyZRange, fms );
    }
    if ( wsprojnm_ != "" )
	astream.put( sKeyWSProjName, wsprojnm_ );

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

    int retval = !strm.fail();
    sd.close();
    return retval;
}


void SurveyInfo::setTr( BinID2Coord::BCTransform& tr, const char* str )
{
    FileMultiString fms( str );
    tr.a = atof(fms[0]); tr.b = atof(fms[1]); tr.c = atof(fms[2]);
}


void SurveyInfo::putTr( const BinID2Coord::BCTransform& tr, ascostream& astream,
			const char* key ) const
{
    FileMultiString fms;
    fms += tr.a; fms += tr.b; fms += tr.c;
    astream.put( key, fms );
}


BinID SurveyInfo::transform( const Coord& c ) const
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


bool SurveyInfo::isReasonable( const BinID& b ) const
{
    BinID w( range_.stop.inl - range_.start.inl,
             range_.stop.crl - range_.start.crl );
    return b.inl > range_.start.inl - w.inl
	&& b.inl < range_.stop.inl  + w.crl
	&& b.crl > range_.start.crl - w.crl
	&& b.crl < range_.stop.crl  + w.crl;
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
	b[0] = range_.start; c[0] = transform( b[0] );
	b[1] = range_.stop; c[1] = transform( b[1] );
	BinID b2 = range_.stop; b2.inl = b[0].inl;
	c[2] = transform( b2 ); xline = b2.crl;
    }
}


const char* SurveyInfo::set3Pts( const Coord c[3], const BinID b[2], int xline )
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
    const BinIDRange& rg = work ? wrange_ : range_;
    const BinID& stp = work ? wstep_ : step_;
    if ( stp.inl == 1 && stp.crl == 1 ) return;
    doSnap( binid.inl, rg.start.inl, stp.inl, rounding.inl );
    doSnap( binid.crl, rg.start.crl, stp.crl, rounding.crl );
}


void SurveyInfo::snapStep( BinID& s, BinID rounding, bool work ) const
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
    rg = zr;
    rg.sort();
}


void SurveyInfo::setZRange( const Interval<double>& zr, bool work )
{
    StepInterval<double>& rg = work ? wzrange_ : zrange_;
    assign( rg, zr );
    rg.sort();
}


void SurveyInfo::setStep( const BinID& bid, bool work )
{
    BinID& stp = work ? wstep_ : step_;
    stp = bid;
    if ( !stp.inl ) stp.inl = 1; if ( !stp.crl ) stp.crl = 1;
    if ( stp.inl < 0 ) stp.inl = -stp.inl;
    if ( stp.crl < 0 ) stp.crl = -stp.crl;
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


void SurveyInfo::checkInlRange( Interval<int>& intv, bool work ) const
{
    const BinIDRange& rg = work ? wrange_ : range_;
    intv.sort();
    if ( intv.start < rg.start.inl ) intv.start = rg.start.inl;
    if ( intv.stop > rg.stop.inl )   intv.stop = rg.stop.inl;
}

void SurveyInfo::checkCrlRange( Interval<int>& intv, bool work ) const
{
    const BinIDRange& rg = work ? wrange_ : range_;
    intv.sort();
    if ( intv.start < rg.start.crl ) intv.start = rg.start.crl;
    if ( intv.stop > rg.stop.crl )   intv.stop = rg.stop.crl;
}

void SurveyInfo::checkZRange( Interval<double>& intv, bool work ) const
{
    const StepInterval<double>& rg = work ? wzrange_ : zrange_;
    intv.sort();
    if ( intv.start < rg.start ) intv.start = rg.start;
    if ( intv.stop > rg.stop )   intv.stop = rg.stop;
}
