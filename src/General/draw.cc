/*+
 * COPYRIGHT: (C) de Groot-Bril Earth Sciences B.V.
 * AUTHOR   : A.H. Bril
 * DATE     : 18-4-1996
-*/

static const char* rcsID = "$Id: draw.cc,v 1.30 2002-08-02 13:06:40 nanne Exp $";

/*! \brief Several implementations for UI-related things.

The main chunk is color table related.
*/

#include "draw.h"
#include "colortab.h"
#include "separstr.h"
#include "iopar.h"
#include "settings.h"
#include "uidset.h"
#include "ascstream.h"
#include "ptrman.h"
#include <fstream>

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
bool ColorTable::tabparsinited = false;
ObjectSet<IOPar> ColorTable::tabpars;


ColorTable& ColorTable::operator=(const ColorTable& n )
{
    setName( n.name() );

    cvs = n.cvs;
    undefcolor = n.undefcolor;
    markcolor = n.markcolor;

    collist = n.collist;
    uselist = n.uselist;

    return *this;
}


bool ColorTable::operator==( const ColorTable& ct ) const
{
    bool res = false;
    if ( ct.name() != name() ||
	 ct.getInterval() != getInterval() ||
	 ct.undefcolor != undefcolor ||
	 ct.markcolor != markcolor )		return false;
	 
    if ( ct.cvs.size() != cvs.size() ) return false;
    else
    {
	for ( int idx=0; idx<cvs.size(); idx++ )
	{
	    if ( cvs[idx].value != ct.cvs[idx].value ||
		 cvs[idx].color != ct.cvs[idx].color )		return false;
	}
    }

    return true;
}


ColorTable* ColorTable::clone() const
{
    IOPar iopar;
    fillPar( iopar );

    ColorTable* res = new ColorTable;
    res->usePar( iopar );
    return res;
}


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


Color ColorTable::color( float v, bool use_undefcol ) const
{
    if ( mIsUndefined(v) ) return undefcolor;
    const int sz = cvs.size();
    if ( sz == 0 ) return undefcolor;

    ColorVal cv( cvs[0] );
    if ( sz == 1 || mIS_ZERO(v-cv.value) ) return cv.color;
    bool isrev = cvs[0].value > cvs[1].value;
    if ( (isrev && v>cv.value) || (!isrev && v<cv.value) ) 
	if ( use_undefcol )	
	    return undefcolor;
	else
	    v = cv.value;

    ColorVal cv2 = cvs[sz-1];
    if ( (isrev && v<cv2.value) || (!isrev && v>cv2.value) )
	if ( use_undefcol )	
	    return undefcolor;
	else
	    v = cv2.value;

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


int ColorTable::colorIdx( float v, int undefid ) const
{
    const int sz = cvs.size();
    if ( !sz ) return undefid;

    const int csz = collist.size();
    if ( csz < 1 ) return undefid;
    if ( csz == 1 ) return 0;


    float startval = cvs[0].value;
    float stopval = cvs[sz-1].value;

    if ( mIsUndefined( v ) ) return undefid;

    bool isrev = startval > stopval;
    if ( isrev )
    {
	if ( v>startval ) return 0;
	if ( v<stopval ) return csz-1;
    }
    else
    {
	if ( v<startval ) return 0;
	if ( v>stopval ) return csz-1;
    }

    float fcidx = ((v-startval) * (csz-1)) / (stopval-startval);
    return ((int)(fcidx+.5));
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
    const char* res = iopar.find( sNameKey );
    if ( res ) setName( res );
    getfromPar( iopar, markcolor, sKeyMarkColor );
    getfromPar( iopar, undefcolor, sKeyUdfColor );

    cvs.erase();
    for ( int idx=0; ; idx++ )
    {
	BufferString key( ColorVal::sKey );
	key += "."; key += idx;
	Color col;
	float val = getfromPar( iopar, col, key, true );
	if ( mIsUndefined(val) )
	{
	    if ( idx ) break;
	    continue;
	}

	cvs += ColorVal( col, val );
    }

    if ( uselist )
	calcList( collist.size() );
}


void ColorTable::initTabs()
{
    ifstream strm( GetDataFileName("ColTabs") );
    if ( !strm ) return;
    ascistream astrm( strm );
    IOPar iopar( astrm, true );
    add( iopar, 0, &tabpars );
    if ( tabpars.size() )
	tabparsinited = true;
}


void ColorTable::getNames( UserIDSet& names, bool usrct_only )
{
    names.deepErase();
    names.setName( "Color table" );

    PtrMan<IOPar> iopar = Settings::common().subselect( names.name() );
    if ( iopar && iopar->size() )
	add( *iopar, &names, 0 );

    if ( usrct_only )
    {
	names.setCurrent(0);
	return;
    }

    if ( !tabparsinited ) initTabs();
    for ( int idx=0; idx<tabpars.size(); idx++ )
    {
	const char* nm = tabpars[idx]->find( sNameKey );
	if ( nm && *nm && !names[nm] )
	    names.add( nm );
    }

    names.setCurrent(0);
}


void ColorTable::add( const IOPar& iopar, UserIDSet* names,
			ObjectSet<IOPar>* pars )
{
    for ( int idx=0; ; idx++ )
    {
	IOPar* ctiopar = iopar.subselect( idx );
	if ( !ctiopar || !ctiopar->size() )
	{
	    if ( !idx ) continue;
	    delete ctiopar;
	    break;
	}

	const char* res = ctiopar->find( sNameKey );
	if ( res && *res )
	{
	    if ( names )
		{ names->add( res ); delete ctiopar; }
	    else
		*pars += ctiopar;
	}
    }
}


void ColorTable::get( const char* nm, ColorTable& ct )
{
    BufferString ctname = "Rainbow";
    if ( !nm || !*nm )
	mSettUse(get,"dTect.Color table","Name",ctname);
    else
	ctname = nm;

    PtrMan<IOPar> iopar = Settings::common().subselect( "Color table" );
    if ( iopar && iopar->size() )
    {
	for ( int idx=0; ; idx++ )
	{
	    PtrMan<IOPar> ctiopar = iopar->subselect( idx );
	    if ( !ctiopar || !ctiopar->size() )
	    {
		if ( !idx ) continue;
		break;
	    }
	    
	    const char* res = (*ctiopar)[sNameKey];
	    if ( !strcmp(res,ctname) )
	    {
		ct.usePar( *ctiopar );
		return;
	    }
	}
    }

    if ( !tabparsinited ) initTabs();
    for ( int idx=0; idx<tabpars.size(); idx++ )
    {
	const IOPar& iop = *tabpars[idx];
	if ( !strcmp(ctname,iop[sNameKey]) )
	{
	    ct.usePar( iop );
    	    return;
	}
    }

    //NEXT version remove
    if ( !strcmp(ctname,"Blue-Green-Magenta") )
	get( "Rainbow", ct );
    else if ( !strcmp(ctname,"White-Yellow-Red") )
	get( "SunRise", ct );
    else if ( !strcmp(ctname,"Blue-Cyan-WYR") )
	get( "Pastel", ct );
    else if ( !strcmp(ctname,"Blue-White-Blue") )
	get( "Blue Spirit", ct );

    if ( !ct.name() || !(*ct.name()) )
	get( "Red-White-Black", ct );
}


void ColorTable::fillPar( IOPar& iopar ) const
{
    iopar.set( sNameKey, name() );
    FileMultiString fms;
    fms += markcolor.r(); fms += markcolor.g(); fms += markcolor.b();
    iopar.set( sKeyMarkColor, fms );
    fms = "";
    fms += undefcolor.r(); fms += undefcolor.g(); fms += undefcolor.b();
    fms += undefcolor.t();
    iopar.set( sKeyUdfColor, fms );

    for ( int idx=0; idx<cvs.size(); idx++ )
    {
	fms = "";
	fms += cvs[idx].value;
	fms += cvs[idx].color.r();
	fms += cvs[idx].color.g();
	fms += cvs[idx].color.b();
	fms += cvs[idx].color.t();
	BufferString str( ColorVal::sKey );
	str += "."; str += idx;
	iopar.set( str, fms );
    }
}
