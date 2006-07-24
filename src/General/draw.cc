/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : A.H. Bril
 * DATE     : 18-4-1996
-*/

static const char* rcsID = "$Id: draw.cc,v 1.55 2006-07-24 16:27:46 cvskris Exp $";

/*! \brief Several implementations for UI-related things.

The main chunk is color table related.
*/

#include "draw.h"
#include "colortab.h"

#include "basictask.h"
#include "separstr.h"
#include "iopar.h"
#include "settings.h"
#include "ascstream.h"
#include "ptrman.h"
#include "oddirs.h"
#include "interpol1d.h"
#include "bufstringset.h"
#include "strmprov.h"
#include "filegen.h"

#include <iostream>

// First some implementations for a couple of header files ...

DefineEnumNames(MarkerStyle2D,Type,2,"Marker type")
    { "None", "Square", "Circle", "Cross", 0 };
DefineEnumNames(MarkerStyle3D,Type,0,"Marker type")
    { "Cube", "Cone", "Cylinder", "Sphere", "Arrow", "Cross", 0 };
DefineEnumNames(LineStyle,Type,0,"Line style")
    { "None", "Solid", "Dashed", "Dotted", "Dash-Dotted", "Dash-Dot-Dotted",0 };
DefineEnumNames(Alignment,Pos,3,"Alignment position")
    { "Start", "Middle", "Stop", 0 };


// Then some draw.h stuff

#define mToStringImpl( clss, par ) \
void clss::toString( BufferString& bs ) const \
{ \
    FileMultiString fms; \
    fms = eString(Type,type); \
    fms += par; \
    color.fill( bs.buf() ); \
    fms += bs; \
    bs = fms; \
}


#define mFromStringImpl( clss, par ) \
void clss::fromString( const char* s ) \
{ \
    FileMultiString fms( s ); \
    type = eEnum(Type,fms[0]); \
    par = atoi(fms[1]); \
    FileMultiString colfms( fms.from(2) ); \
    color.use( colfms ); \
}


mToStringImpl( MarkerStyle2D, size )
mToStringImpl( MarkerStyle3D, size )
mToStringImpl( LineStyle, width )

mFromStringImpl( MarkerStyle2D, size )
mFromStringImpl( MarkerStyle3D, size )
mFromStringImpl( LineStyle, width )


// And the ColorTable stuff ...

const char* ColorVal::sKey = "Value-Color";
const char* ColorTable::sKeyName = "Color table name";
const char* ColorTable::sKeyMarkColor = "Marker color";
const char* ColorTable::sKeyUdfColor = "Undef color";
const char* ColorTable::sKeyTransparency = "Transparency";
static const char* sKeyCtbl = "Color table";

static ObjectSet<IOPar>& stdTabPars()
{
    static ObjectSet<IOPar>* parset = 0;
    if ( !parset )
    {
	parset = new ObjectSet<IOPar>;
	ColorTable::getStdTabPars( *parset );
    }
    return *parset;
}


static IOPar* readStdTabs()
{
    StreamData sd = StreamProvider( GetDataFileName("ColTabs") ).makeIStream();
    IOPar* iop = 0;
    if ( sd.usable() )
    {
	ascistream astrm( *sd.istrm );
	iop = new IOPar( astrm );
	sd.close();
    }
    return iop;
}


bool ColorTable::putStdTabPars( const ObjectSet<IOPar>& parset )
{
    BufferString fname = GetDataFileName("ColTabs");
    if ( File_exists(fname) && !File_isWritable(fname)
	&& !File_makeWritable(fname,NO,YES) )
    {
	ErrMsg( "Cannot make standard color tables file writable" );
	return false;
    }

    StreamData sd = StreamProvider( GetDataFileName("ColTabs") ).makeOStream();
    if ( !sd.usable() || !sd.ostrm->good() )
    {
	ErrMsg( "Cannot open standard color tables file for write" );
	return false;
    }

    IOPar iop( "Standard color tables" );
    for ( int idx=0; idx<parset.size(); idx++ )
    {
	BufferString nrstr; nrstr += idx+1;
	iop.mergeComp( *parset[idx], nrstr.buf() );
    }
    ascostream astrm( *sd.ostrm );
    astrm.putHeader( "Color table definitions" );
    iop.putTo( astrm );
    return true;
}


void ColorTable::getStdTabPars( ObjectSet<IOPar>& parset )
{
    IOPar* iopar = readStdTabs();
    if ( iopar )
	add( *iopar, 0, &parset );
    delete iopar;
}


ColorTable& ColorTable::operator=(const ColorTable& n )
{
    setName( n.name() );

    cvs_ = n.cvs_;
    undefcolor_ = n.undefcolor_;
    markcolor_ = n.markcolor_;

    collist_ = n.collist_;
    usetranslist_ = n.usetranslist_;
    translist_ = n.translist_;

    return *this;
}


bool ColorTable::operator==( const ColorTable& ct ) const
{
    bool res = false;
    if ( ct.name() != name() ||
	 ct.getInterval() != getInterval() ||
	 ct.undefcolor_ != undefcolor_ ||
	 ct.markcolor_ != markcolor_ )		return false;
	 
    if ( ct.cvs_.size() != cvs_.size() ) return false;
    else
    {
	for ( int idx=0; idx<cvs_.size(); idx++ )
	{
	    if ( cvs_[idx].value != ct.cvs_[idx].value ||
		 cvs_[idx].color != ct.cvs_[idx].color )
		return false;
	}
    }

    if ( ct.translist_.size() != translist_.size() ) return false;
    else
    {
	for ( int idx=0; idx<translist_.size(); idx++ )
	    if ( translist_[idx] != ct.translist_[idx] )
		 return false;
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


class ColorTableIndexer : public ParallelTask
{
public:
    			ColorTableIndexer( int nt, float x0, float dx,
			       		   const ColorTable& ct, Color* res )
			    : nrtimes( nt )
			    , x0_( x0 )
			    , dx_( dx )
			    , ct_( ct )
			    , res_( res )
			{}

    bool		doWork( int start, int stop )
    			{
			    for ( int idx=start; idx<=stop; idx++ )
				res_[idx] = ct_.color( x0_+dx_*idx );
			    return true;
			}

    int			nrTimes() const { return nrtimes; }

protected:

    int			nrtimes;
    float		x0_;
    float		dx_;
    const ColorTable&	ct_;
    Color*		res_;

};


void ColorTable::calcList( int nritems )
{
    usetranslist_ = false;	//Prevent list from usage while computing
    const int sz = cvs_.size();
    if ( !sz || nritems<1 )
    {
	collist_.erase();
	return;
    }

    const ColorVal cv0( cvs_[0] );
    const ColorVal cv1( cvs_[sz-1] );
    const float dist = cv1.value - cv0.value;
    collist_.setSize( nritems );
    if ( nritems == 1 )
	collist_[0] = color( cv0.value + dist / 2 );
    else
    {
	ColorTableIndexer comp( nritems, cv0.value, dist/(nritems-1),
				*this, collist_.arr() );
	comp.execute();
    }

    usetranslist_ = true;
}


Color ColorTable::color( float v, bool use_undefcol ) const
{
    if ( mIsUdf(v) ) return undefcolor_;
    const int sz = cvs_.size();
    if ( sz == 0 ) return undefcolor_;

    ColorVal cv( cvs_[0] );
    if ( sz == 1 || mIsEqual(v,cv.value,mDefEps) )
	return Color( cv.color.r(), cv.color.g(), cv.color.b(),
		(int)(getTransparency( v ) + .5) );

    bool isrev = cvs_[0].value > cvs_[1].value;
    if ( (isrev && v>cv.value) || (!isrev && v<cv.value) ) 
	if ( use_undefcol )	
	    return undefcolor_;
	else
	    v = cv.value;

    ColorVal cv2 = cvs_[sz-1];
    if ( (isrev && v<cv2.value) || (!isrev && v>cv2.value) )
	if ( use_undefcol )	
	    return undefcolor_;
	else
	    v = cv2.value;

    if ( usetranslist_ )
    {
	const int csz = collist_.size();
	if ( csz < 1 ) return undefcolor_;
	if ( csz == 1 ) return collist_[0];

	float fcidx = ((v-cv.value) * (csz-1)) / (cv2.value-cv.value);
	return collist_[ ((int)(fcidx+.5)) ];
    }


#define mColRGBVal(c) ((cv2.value-v)*cv.color.c()+(v-cv.value)*cv2.color.c())

    for ( int idx=1; idx<cvs_.size(); idx++ )
    {
	cv2 = cvs_[idx];
	if ( (isrev && v >= cv2.value) || (!isrev && v <= cv2.value) )
	{
	    if ( mIsEqual(v,cv2.value,mDefEps) )
		return Color( cv2.color.r(), cv2.color.g(), cv2.color.b(),
			      (int)(getTransparency( v ) + .5) );

	    float dist = cv2.value - cv.value;
	    return Color( Color::getUChar( mColRGBVal(r) / dist ),
			  Color::getUChar( mColRGBVal(g) / dist ),
			  Color::getUChar( mColRGBVal(b) / dist ),
			  (int)(getTransparency( v ) + .5) );
	}
	cv = cv2;
    }

    return undefcolor_;
}


float ColorTable::getTransparency( float val ) const
{
    const int sz = cvs_.size();
    float valnorm = (val - cvs_[0].value) / (cvs_[sz-1].value - cvs_[0].value);
    for ( int idx=1; idx<translist_.size(); idx ++)
    {
	float x0 = translist_[idx-1].x();
	float y0 = translist_[idx-1].y();
	float x1 = translist_[idx].x();
	float y1 = translist_[idx].y();
	if ( valnorm >= x0 && valnorm <= x1 )
	    return Interpolate::linear1D( x0, y0, x1, y1, valnorm );
    }

    return 0;
}


int ColorTable::colorIdx( float v, int undefid ) const
{
    const int sz = cvs_.size();
    if ( !sz ) return undefid;

    const int csz = collist_.size();
    if ( csz < 1 ) return undefid;
    if ( csz == 1 ) return 0;


    float startval = cvs_[0].value;
    float stopval = cvs_[sz-1].value;

    if ( mIsUdf( v ) ) return undefid;

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
    const int sz = cvs_.size();
    if ( !sz ) return;
    if ( sz < 2 ) { cvs_[0].value = (intv.start+intv.stop)*.5; return; }

    const float oldwidth = cvs_[sz-1].value - cvs_[0].value;
    const float newwidth = intv.stop - intv.start;
    const float oldstart = cvs_[0].value;
    cvs_[0].value = intv.start;
    cvs_[sz-1].value = intv.stop;
    if ( !oldwidth )
    {
	for ( int idx=1; idx<sz-1; idx++ )
	    cvs_[idx].value = intv.start + idx * newwidth / (sz-1);
    }
    else
    {
	const float fac = newwidth / oldwidth;
	for ( int idx=1; idx<sz-1; idx++ )
	    cvs_[idx].value = intv.start + (cvs_[idx].value - oldstart) * fac;
    }
}


Interval<float> ColorTable::getInterval() const
{
    Interval<float> ret( mUdf(float), mUdf(float) );
    if ( cvs_.size() > 0 )
	ret = Interval<float>( cvs_[0].value, cvs_[cvs_.size()-1].value );

    return ret;
}


bool ColorTable::hasTransparency() const
{
    for ( int idx=cvs_.size()-1; idx>=0; idx-- )
    {
	if ( cvs_[idx].color.t() )
	    return true;
    }

    return false;
}


void ColorTable::setTransparencyFromColVals()
{
    translist_.erase();
    if ( !cvs_.size() )
	return;

    const Interval<float> range( cvs_[0].value, cvs_[cvs_.size()-1].value );

    if ( cvs_.size()==1 || mIsZero( range.width(), mDefEps ) )
    {
	translist_ += Geom::Point2D<float>(0,cvs_[0].color.t() );
	translist_ += Geom::Point2D<float>(1,cvs_[cvs_.size()-1].color.t() );
    }
    else
    {
	for ( int idx=0; idx<cvs_.size(); idx++ )
    	{
	    const float relpos = (cvs_[idx].value-range.start) / range.width();
	    translist_ += Geom::Point2D<float>(relpos,cvs_[idx].color.t() );
	}
    }
}


static float getfromPar( const IOPar& iopar, Color& col, const char* key,
			 bool withval=false )
{
    const char* res = iopar[key];
    float val = withval ? mUdf(float) : 0;
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


void ColorTable::fillPar( IOPar& iopar ) const
{
    iopar.set( sNameKey, name() );
    FileMultiString fms;
    fms += markcolor_.r(); fms += markcolor_.g(); fms += markcolor_.b();
    iopar.set( sKeyMarkColor, fms );
    fms = "";
    fms += undefcolor_.r(); fms += undefcolor_.g(); fms += undefcolor_.b();
    fms += undefcolor_.t();
    iopar.set( sKeyUdfColor, fms );

    for ( int idx=0; idx<cvs_.size(); idx++ )
    {
	fms = "";
	fms += cvs_[idx].value;
	fms += cvs_[idx].color.r();
	fms += cvs_[idx].color.g();
	fms += cvs_[idx].color.b();
	fms += cvs_[idx].color.t();
	BufferString str( ColorVal::sKey );
	str += "."; str += idx;
	iopar.set( str, fms );
    }

    for ( int idx=0; idx<translist_.size(); idx++ )
    {
	BufferString key( sKeyTransparency );
	key += "."; key += idx;
	iopar.set( key, translist_[idx].x(), translist_[idx].y() );
    }
}


void ColorTable::usePar( const IOPar& iopar )
{
    const char* res = iopar.find( sNameKey );
    if ( res ) setName( res );
    getfromPar( iopar, markcolor_, sKeyMarkColor );
    getfromPar( iopar, undefcolor_, sKeyUdfColor );

    cvs_.erase();
    translist_.erase();
    for ( int idx=0; ; idx++ )
    {
	BufferString key( ColorVal::sKey );
	key += "."; key += idx;
	Color col;
	float val = getfromPar( iopar, col, key, true );
	if ( mIsUdf(val) )
	{
	    if ( idx ) break;
	    continue;
	}

	cvs_ += ColorVal( col, val );
    }

    for ( int idx=0; ; idx++ )
    {
	BufferString key( sKeyTransparency );
	key += "."; key += idx;
	float val;
	float alpha;
	if ( !iopar.get( key, val, alpha ) ) break;
	translist_ += Geom::Point2D<float>(val,alpha);
    }

    if ( !translist_.size() )
    {
	for ( int idx=0; idx<cvs_.size(); idx++ )
	{
	    translist_ +=
		Geom::Point2D<float>(cvs_[idx].value,cvs_[idx].color.t());
	}
    }

    if ( usetranslist_ )
	calcList( collist_.size() );
}


void ColorTable::getNames( NamedBufferStringSet& names, ColorTable::Src opt )
{
    names.deepErase();
    names.setName( sKeyCtbl );

    if ( opt != ColorTable::Sys )
    {
	PtrMan<IOPar> iopar = Settings::common().subselect( names.name() );
	if ( iopar && iopar->size() )
	    add( *iopar, &names, 0 );
	if ( opt == ColorTable::UsrDef ) return;
    }

    ObjectSet<IOPar>& tabpars = stdTabPars();
    for ( int idx=0; idx<tabpars.size(); idx++ )
    {
	const char* nm = tabpars[idx]->find( sNameKey );
	if ( nm && *nm )
	    names.addIfNew( nm );
    }
}


bool ColorTable::get( const char* nm, ColorTable& ct, ColorTable::Src opt )
{
    BufferString ctname = "Seismics";
    if ( nm && *nm )
	ctname = nm;
    else
    {
	BufferString key( IOPar::compKey( "dTect", sKeyCtbl ) );
	mSettUse(get,key.buf(),"Name",ctname);
    }

    if ( opt != ColorTable::Sys )
    {
	PtrMan<IOPar> iopar = Settings::common().subselect( sKeyCtbl );
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
		
		if ( ctname == ctiopar->find( sNameKey ) )
		{
		    ct.usePar( *ctiopar );
		    return true;
		}
	    }
	}
    }

    if ( opt == ColorTable::UsrDef )
	return false;

    ObjectSet<IOPar>& tabpars = stdTabPars();
    for ( int idx=0; idx<tabpars.size(); idx++ )
    {
	const IOPar& iop = *tabpars[idx];
	if ( ctname == iop.find(sNameKey) )
	{
	    ct.usePar( iop );
    	    return true;
	}
    }

    return false;
}


void ColorTable::add( const IOPar& iopar, BufferStringSet* names,
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
	    if ( pars )
		*pars += ctiopar;
	}
    }
}
