/*+
 * COPYRIGHT: (C) de Groot-Bril Earth Sciences B.V.
 * AUTHOR   : A.H. Bril
 * DATE     : 18-4-1996
-*/

static const char* rcsID = "$Id: survinfo.cc,v 1.3 2000-06-23 14:11:11 bert Exp $";

#include "survinfo.h"
#include "ascstream.h"
#include "filegen.h"
#include "separstr.h"
#include "errh.h"
#include <fstream.h>

static const char* sKey = "Survey Info";
SurveyInfo* SurveyInfo::theinst_;

const SurveyInfo& SI()
{
    if ( !SurveyInfo::theinst_ )
	SurveyInfo::theinst_ = new SurveyInfo( GetDataDir() );
    return *SurveyInfo::theinst_;
}


double SurveyInfo::BCTransform::det( const BCTransform& bct ) const
{
    double rv = b * bct.c - bct.b * c;
    return rv;
}


int SurveyInfo::BCTransform::valid( const BCTransform& bct ) const
{
    double d = det( bct );
    return !mIS_ZERO(d);
}


SurveyInfo::SurveyInfo( const SurveyInfo& si )
{
    xtr = si.xtr;
    ytr = si.ytr;
    dirname = si.dirname;
    range_ = si.range_;
    step_ = si.step_;
    setName( si.name() );
}


SurveyInfo::SurveyInfo( const char* rootdir )
	: dirname(File_getFileName(rootdir))
{
    FileNameString fname( File_getFullPath( rootdir, ".survey" ) );
    ifstream strm( fname );
    if ( strm.fail() )
    {
	ErrMsg( "Cannot read survey definition file!" );
	return;
    }
    ascistream astream( strm );
    if ( !astream.isOfFileType(sKey) ) return;

    xtr.b = 1;
    ytr.c = 1;
    BinIDRange bir; BinID bid( 1, 1 );
    while ( !atEndOfSection( astream.next() ) )
    {
	if ( astream.hasKeyword(sNameKey) )
	    setName( astream.value() );
	else if ( astream.hasKeyword("Coord-X-BinID") )
	    setTr( xtr, astream.value() );
	else if ( astream.hasKeyword("Coord-Y-BinID") )
	    setTr( ytr, astream.value() );
	else if ( astream.hasKeyword("In-line range") )
	{
	    FileMultiString fms( astream.value() );
	    bir.start.inl = atoi(fms[0]);
	    bir.stop.inl = atoi(fms[1]);
	    bid.inl = atoi(fms[2]);
	}
	else if ( astream.hasKeyword("Cross-line range") )
	{
	    FileMultiString fms( astream.value() );
	    bir.start.crl = atoi(fms[0]);
	    bir.stop.crl = atoi(fms[1]);
	    bid.crl = atoi(fms[2]);
	}
    }

    setRange( bir );
    setStep( bid );
}


int SurveyInfo::write( const char* basedir ) const
{
    FileNameString fname( File_getFullPath(basedir,dirname) );
    fname = File_getFullPath( fname, ".survey" );
    ofstream strm( fname );
    if ( strm.fail() ) return NO;
    ascostream astream( strm );
    if ( !astream.putHeader(sKey) ) return NO;

    astream.put( sNameKey, name() );
    putTr( xtr, astream, "Coord-X-BinID" );
    putTr( ytr, astream, "Coord-Y-BinID" );
    FileMultiString fms;
    fms += range_.start.inl; fms += range_.stop.inl; fms += step_.inl;
    astream.put( "In-line range", fms );
    fms = ""; fms += range_.start.crl; fms += range_.stop.crl; fms += step_.crl;
    astream.put( "Cross-line range", fms );
    return !strm.fail();
}


void SurveyInfo::setTr( BCTransform& tr, const char* str )
{
    FileMultiString fms( str );
    tr.a = atof(fms[0]); tr.b = atof(fms[1]); tr.c = atof(fms[2]);
}


void SurveyInfo::putTr( const BCTransform& tr, ascostream& astream,
			const char* key ) const
{
    FileMultiString fms;
    fms += tr.a; fms += tr.b; fms += tr.c;
    astream.put( key, fms );
}


Coord SurveyInfo::transform( const BinID& binid ) const
{
    return Coord( xtr.a + xtr.b*binid.inl + xtr.c*binid.crl,
		  ytr.a + ytr.b*binid.inl + ytr.c*binid.crl );
}


BinID SurveyInfo::transform( const Coord& coord ) const
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


void SurveyInfo::snap( BinID& binid, const BinID& direction ) const
{
    if ( step_.inl == 1 && step_.crl == 1 ) return;
    doSnap( binid.inl, range_.start.inl, step_.inl, direction.inl );
    doSnap( binid.crl, range_.start.crl, step_.crl, direction.crl );
}


void SurveyInfo::setRange( const BinIDRange& br )
{
    range_.start = br.start;
    range_.stop = br.stop;
    if ( br.start.inl > br.stop.inl )
    {
	range_.start.inl = br.stop.inl;
	range_.stop.inl = br.start.inl;
    }
    if ( br.start.crl > br.stop.crl )
    {
	range_.start.crl = br.stop.crl;
	range_.stop.crl = br.start.crl;
    }
}


void SurveyInfo::setStep( const BinID& bid )
{
    step_ = bid;
    if ( !step_.inl ) step_.inl = 1; if ( !step_.crl ) step_.crl = 1;
    if ( step_.inl < 0 ) step_.inl = -step_.inl;
    if ( step_.crl < 0 ) step_.crl = -step_.crl;
}
