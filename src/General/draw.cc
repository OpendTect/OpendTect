/*+
 * COPYRIGHT: (C) de Groot-Bril Earth Sciences B.V.
 * AUTHOR   : A.H. Bril
 * DATE     : 18-4-1996
-*/

static const char* rcsID = "$Id: draw.cc,v 1.10 2001-05-14 13:21:50 bert Exp $";

/*! \brief Several implementations for UI-related things.

The main chunk is color table related.
*/

#include "draw.h"
#include "colortab.h"
#include "separstr.h"
#include "iopar.h"
#include "settings.h"
#include "uidset.h"

// First some implementations for a couple of header files ...

DefineEnumNames(MarkerStyle,Type,2,"Marker type")
    { "None", "Square", "Circle", "Cross", 0 };
DefineEnumNames(LineStyle,Type,0,"Line style")
    { "None", "Solid", "Dashed", "Dotted", "Dash-Dotted", "Dash-Dot-Dotted",0 };
DefineEnumNames(Alignment,Pos,3,"Alignment position")
    { "Start", "Middle", "Stop", 0 };


// The some draw.h stuff

void MarkerStyle::toString( BufferString& bs ) const
{
    FileMultiString fms;
    fms = eString(Type,type);
    fms += size;
    color.fill( bs.buf() );
    fms += bs;
    bs = fms;
}


void MarkerStyle::fromString( const char* s )
{
    FileMultiString fms( s );
    type = eEnum(Type,fms[0]);
    size = atoi(fms[1]);
    FileMultiString colfms( fms.from(2) );
    color.use( colfms );
}


void LineStyle::toString( BufferString& bs ) const
{
    FileMultiString fms;
    fms = eString(Type,type);
    fms += width;
    color.fill( bs.buf() );
    fms += bs;
    bs = fms;
}


void LineStyle::fromString( const char* s )
{
    FileMultiString fms( s );
    type = eEnum(Type,fms[0]);
    width = atoi(fms[1]);
    FileMultiString colfms( fms.from(2) );
    color.use( colfms );
}

// And the ColorTable stuff ...

const char* ColorVal::sKey = "Value-Color";
const char* ColorTable::sKeyName = "Color table name";
const char* ColorTable::sKeyMarkColor = "Marker color";
const char* ColorTable::sKeyUdfColor = "Undef color";

DefineEnumNames(ColorTable,Type,0,"Color table name")
{
	"Red-White-Blue",
	"Grey scales",
	"Blue-Green-Magenta",
	"White-Yellow-Red",
	"Blue-Cyan-WYR",
	"Blue-White-Blue",
	"Black/White",
	"UserDefined",
	0
};


void ColorTable::calcList( int nritems )
{
    collist.erase();
    const int sz = cvs.size();
    if ( sz == 0 || nritems < 1 ) return;

    ColorVal cv0( cvs[0] );
    ColorVal cv1( cvs[sz-1] );
    float dist = cv1.value - cv0.value;
    uselist = false;
    if ( nritems == 1 )
    {
	collist += color( cv0.value + dist / 2 );
	uselist = true;
	return;
    }

    float step = dist / (nritems-1);
    for ( int idx=0; idx<nritems; idx++ )
	collist += color( cv0.value + idx * step );

    uselist = true;
}


Color ColorTable::color( float v ) const
{
    if ( mIsUndefined(v) ) return undefcolor;
    const int sz = cvs.size();
    if ( sz == 0 ) return undefcolor;

    ColorVal cv( cvs[0] );
    if ( sz == 1 || mIS_ZERO(v-cv.value) ) return cv.color;
    bool isrev = cvs[0].value > cvs[1].value;
    if ( (isrev && v>cv.value) || (!isrev && v<cv.value) ) return undefcolor;

    ColorVal cv2 = cvs[sz-1];
    if ( (isrev && v<cv2.value) || (!isrev && v>cv2.value) ) return undefcolor;

    if ( uselist )
    {
	const int csz = collist.size();
	if ( csz < 1 ) return undefcolor;
	if ( csz == 1 ) return collist[0];

	float fcidx = ((v-cv.value) * (csz-1)) / (cv2.value-cv.value);
	return collist[ ((int)(fcidx+.5)) ];
    }


#define mColRGBVal(c) ((cv2.value-v)*cv.color.c()+(v-cv.value)*cv2.color.c())

    for ( int idx=1; idx<cvs.size(); idx++ )
    {
	cv2 = cvs[idx];
	if ( (isrev && v >= cv2.value) || (!isrev && v <= cv2.value) )
	{
	    if ( mIS_ZERO(v-cv2.value) ) return cv2.color;

	    float dist = cv2.value - cv.value;
	    return Color( Color::getUChar( mColRGBVal(r) / dist ),
			  Color::getUChar( mColRGBVal(g) / dist ),
			  Color::getUChar( mColRGBVal(b) / dist ),
			  Color::getUChar( mColRGBVal(t) / dist ) );
	}
	cv = cv2;
    }

    return undefcolor;
}


void ColorTable::scaleTo( const Interval<float>& intv )
{
    const int sz = cvs.size();
    if ( !sz ) return;
    if ( sz < 2 ) { cvs[0].value = (intv.start+intv.stop)*.5; return; }

    const float oldwidth = cvs[sz-1].value - cvs[0].value;
    const float newwidth = intv.stop - intv.start;
    const float oldstart = cvs[0].value;
    cvs[0].value = intv.start;
    cvs[sz-1].value = intv.stop;
    if ( !oldwidth )
    {
	for ( int idx=1; idx<sz-1; idx++ )
	    cvs[idx].value = intv.start + idx * newwidth / (sz-1);
    }
    else
    {
	const float fac = newwidth / oldwidth;
	for ( int idx=1; idx<sz-1; idx++ )
	    cvs[idx].value = intv.start + (cvs[idx].value - oldstart) * fac;
    }
}


Interval<float> ColorTable::getInterval() const
{
    Interval<float> ret( mUndefValue, mUndefValue );
    if ( cvs.size() > 0 )
	ret = Interval<float>( cvs[0].value, cvs[cvs.size()-1].value );

    return ret;
}


static float getfromPar( const IOPar& iopar, Color& col, const char* key,
			 bool withval=false )
{
    const char* res = iopar[key];
    float val = withval ? mUndefValue : 0;
    if ( res && *res )
    {
	if ( !withval )
	    col.use( res );
	else
	{
	    const FileMultiString fms( res );
	    if ( fms.size() > 1 && col.use( fms.from(1) ) )
		val = atof(fms[0]);
	}
    }
    return val;
}


void ColorTable::usePar( const IOPar& iopar )
{
    setName( "" );
    const char* nm = iopar[sNameKey];
    if ( nm && *nm )
    {
	Type t = eEnum(ColorTable::Type,nm);
	if ( t != (Type)0 || !strcmp(nm,TypeNames[0]) )
	    ColorTable::get( eEnum(ColorTable::Type,nm), *this,
			     Interval<float>(0,1) );
	else
	    type_ = UserDefined;
    }

    getfromPar( iopar, markcolor, sKeyMarkColor );
    getfromPar( iopar, undefcolor, sKeyUdfColor );

    TypeSet<ColorVal> newcvs;
    int idx = -1;
    while ( 1 )
    {
	idx++;
	BufferString key( ColorVal::sKey );
	key += "."; key += idx;
	Color col;
	float val = getfromPar( iopar, col, key, true );
	if ( mIsUndefined(val) )
	{
	    if ( idx ) break;
	    continue;
	}

	newcvs += ColorVal( col, val );
    }

    if ( newcvs.size() > 1 )
    {
	setName( nm );
	cvs = newcvs;
    }
}


void ColorTable::getNames( UserIDSet& names )
{
    names.deepErase();
    names.setName( "Color table" );

    int idx = 0;
    while ( idx < (int)UserDefined )
	names.add( TypeNames[idx++] );
    names.setCurrent(0);

    IOPar* iopar = Settings::common().subselect( names.name() );
    if ( !iopar || !iopar->size() ) { delete iopar; return; }

    idx = 0;
    while ( 1 )
    {
	idx++;
	BufferString key; key += idx;
	IOPar* ctiopar = iopar->subselect( key );
	if ( !ctiopar || !ctiopar->size() ) { delete ctiopar; return; }
	
	const char* res = (*ctiopar)[sNameKey];
	if ( res && *res )
	    names.add( res );
    }
    names.setCurrent(0);
}


void ColorTable::get( const char* nm, ColorTable& ct )
{
    if ( !nm || !*nm ) return;

    int idx = 0;
    while ( idx < (int)UserDefined )
    {
	if ( !strcmp(TypeNames[idx],nm) )
	    get( (Type)idx, ct, Interval<float>(0,1) );
	idx++;
    }

    IOPar* iopar = Settings::common().subselect( "Color table" );
    if ( !iopar || !iopar->size() ) { delete iopar; return; }

    idx = 0;
    while ( 1 )
    {
	idx++;
	BufferString key; key += idx;
	IOPar* ctiopar = iopar->subselect( key );
	if ( !ctiopar || !ctiopar->size() ) { delete ctiopar; return; }
	
	const char* res = (*ctiopar)[sNameKey];
	if ( !strcmp(res,nm) )
	{
	    ct.type_ = UserDefined;
	    ct.usePar( *ctiopar );
	}
    }
}


void ColorTable::fillPar( IOPar& iopar ) const
{
    iopar.set( sKeyName, name() );
    iopar.set( sKeyMarkColor, markcolor.r(), markcolor.g(), markcolor.b() );
    iopar.set( sKeyUdfColor, undefcolor.r(), undefcolor.g(), undefcolor.b() );
    if ( type_ == UserDefined )
    {
	for ( int idx=0; idx<cvs.size(); idx++ )
	{
	    FileMultiString fms;
	    fms += cvs[idx].color.r();
	    fms += cvs[idx].color.g();
	    fms += cvs[idx].color.b();
	    fms += cvs[idx].value;
	    iopar.set( ColorVal::sKey, fms );
	}
    }
}


void ColorTable::get( ColorTable::Type typ, ColorTable& ct,
			const Interval<float>& intv )
{
    ct.type_ = typ;
    switch ( typ )
    {
    case ColorTable::RWB:	getRWB( ct, intv );		break;
    case ColorTable::Greys:	getGreys( ct, intv );		break;
    case ColorTable::BCGYRM:	getBCGYRM( ct, intv );		break;
    case ColorTable::WYR:	getWYR( ct, intv );		break;
    case ColorTable::BCWYR:	getBCWYR( ct, intv );		break;
    case ColorTable::BWB:	getBWB( ct, intv );		break;
    case ColorTable::BlackWhite: getBlackWhite( ct, intv );	break;
    }
}


void ColorTable::set( ColorTable::Type t )
{
    type_ = t;
    setName( ColorTable::TypeNames[t] );
}


void ColorTable::getRWB( ColorTable& ct, const Interval<float>& intv )
{
    ct.set( ColorTable::RWB );
    ct.cvs.erase();
    if ( intv.start > intv.stop )
    {
	ct.cvs += ColorVal( Color(  0,  0,255), intv.start );
	ct.cvs += ColorVal( Color(255,255,255), (intv.start+intv.stop)/2 );
	ct.cvs += ColorVal( Color(255,  0,  0), intv.stop );
    }
    else
    {
	ct.cvs += ColorVal( Color(255,  0,  0), intv.stop );
	ct.cvs += ColorVal( Color(255,255,255), (intv.start+intv.stop)/2 );
	ct.cvs += ColorVal( Color(  0,  0,255), intv.start );
    }
    ct.undefcolor = Color( 200, 200, 200 );
    ct.markcolor = Color( 0, 200, 0 );
}


void ColorTable::getWYR( ColorTable& ct, const Interval<float>& intv )
{
    ct.set( ColorTable::WYR );
    ct.cvs.erase();
    if ( intv.start > intv.stop )
    {
	ct.cvs += ColorVal( Color(255,  0,  0), intv.stop );
	ct.cvs += ColorVal( Color(255,255,  0), (intv.start+intv.stop)/2 );
	ct.cvs += ColorVal( Color(255,255,255), intv.start );
    }
    else
    {
	ct.cvs += ColorVal( Color(255,255,255), intv.start );
	ct.cvs += ColorVal( Color(255,255,  0), (intv.start+intv.stop)/2 );
	ct.cvs += ColorVal( Color(255,  0,  0), intv.stop );
    }
    ct.undefcolor = Color( 200, 200, 200 );
    ct.markcolor = Color( 0, 200, 0 );
}


void ColorTable::getBCWYR( ColorTable& ct, const Interval<float>& intv )
{
    ct.set( ColorTable::BCWYR );
    ct.cvs.erase();
    float step = intv.width() / 4.;
    if ( intv.start > intv.stop )
    {
	ct.cvs += ColorVal( Color(255,  0,  0), intv.stop );
	ct.cvs += ColorVal( Color(255,255,  0), intv.start+3*step );
	ct.cvs += ColorVal( Color(255,255,255), intv.start+2*step );
	ct.cvs += ColorVal( Color(  0,255,255), intv.start+  step );
	ct.cvs += ColorVal( Color(  0,  0,255), intv.start );
    }
    else
    {
	ct.cvs += ColorVal( Color(  0,  0,255), intv.start );
	ct.cvs += ColorVal( Color(  0,255,255), intv.start+  step );
	ct.cvs += ColorVal( Color(255,255,255), intv.start+2*step );
	ct.cvs += ColorVal( Color(255,255,  0), intv.start+3*step );
	ct.cvs += ColorVal( Color(255,  0,  0), intv.stop );
    }
    ct.undefcolor = Color( 200, 200, 200 );
    ct.markcolor = Color( 0, 0, 0 );
}


void ColorTable::getGreys( ColorTable& ct, const Interval<float>& intv )
{
    ct.set( ColorTable::Greys );
    ct.cvs.erase();
    if ( intv.start > intv.stop )
    {
	ct.cvs += ColorVal( Color(255,255,255), intv.stop );
	ct.cvs += ColorVal( Color(  0,  0,  0), intv.start );
    }
    else
    {
	ct.cvs += ColorVal( Color(  0,  0,  0), intv.start );
	ct.cvs += ColorVal( Color(255,255,255), intv.stop );
    }
    ct.undefcolor = Color( 255, 255, 200 );
    ct.markcolor = Color( 200, 0, 0 );
}


void ColorTable::getBWB( ColorTable& ct, const Interval<float>& intv )
{
    ct.set( ColorTable::BWB );
    ct.cvs.erase();
    ct.cvs += ColorVal( Color(  0,  0,255), intv.start );
    ct.cvs += ColorVal( Color(255,255,255), (intv.start+intv.stop)*.5 );
    ct.cvs += ColorVal( Color(  0,  0,255), intv.stop );
    ct.undefcolor = Color( 200, 200, 200 );
    ct.markcolor = Color( 0, 200, 0 );
}


void ColorTable::getBlackWhite( ColorTable& ct, const Interval<float>& intv )
{
    ct.set( ColorTable::BlackWhite );
    ct.cvs.erase();
    Interval<float> iv = intv;
    iv.sort();
    float half = (iv.start + iv.stop) * .5;
    ct.cvs += ColorVal( Color(0,0,0), iv.start );
    ct.cvs += ColorVal( Color(0,0,0), half - mEPSILON );
    ct.cvs += ColorVal( Color(255,255,255), half + mEPSILON );
    ct.cvs += ColorVal( Color(255,255,255), iv.stop );
    ct.markcolor = Color( 0, 0, 0 );
    ct.undefcolor = Color( 255, 255, 255 );
}


void ColorTable::getBCGYRM( ColorTable& ct, const Interval<float>& intv )
{
    ct.set( ColorTable::BCGYRM );
    ct.cvs.erase();
    float step = intv.width() / 5.;
    if ( intv.start > intv.stop )
    {
	ct.cvs += ColorVal( Color(255,  0,255), intv.stop );
	ct.cvs += ColorVal( Color(255,  0,  0), intv.start+4   *step );
	ct.cvs += ColorVal( Color(255,255,  0), intv.start+2.75*step );
	ct.cvs += ColorVal( Color(  0,255,  0), intv.start+2   *step );
	ct.cvs += ColorVal( Color(  0,255,255), intv.start+1.25*step );
	ct.cvs += ColorVal( Color(  0,  0,255), intv.start );
    }
    else
    {
	ct.cvs += ColorVal( Color(  0,  0,255), intv.start );
	ct.cvs += ColorVal( Color(  0,255,255), intv.start+1.25*step );
	ct.cvs += ColorVal( Color(  0,255,  0), intv.start+2*step );
	ct.cvs += ColorVal( Color(255,255,  0), intv.start+2.75*step );
	ct.cvs += ColorVal( Color(255,  0,  0), intv.start+4*step );
	ct.cvs += ColorVal( Color(255,  0,255), intv.stop );
    }
    ct.undefcolor = Color( 200, 200, 200 );
    ct.markcolor = Color( 0, 0, 0 );
}
