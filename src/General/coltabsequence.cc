/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : Bert etc.
 * DATE     : 1996 / Sep 2007
-*/

static const char* rcsID = "$Id: coltabsequence.cc,v 1.1 2007-09-07 11:21:01 cvsbert Exp $";

#include "coltabsequence.h"
#include "separstr.h"
#include "iopar.h"
#include "settings.h"
#include "ascstream.h"
#include "ptrman.h"
#include "oddirs.h"
#include "bufstringset.h"
#include "strmprov.h"
#include "filegen.h"
#include "keystrs.h"

const char* ColTab::Sequence::sKeyValCol = "Value-Color";
const char* ColTab::Sequence::sKeyMarkColor = "Marker color";
const char* ColTab::Sequence::sKeyUdfColor = "Undef color";
const char* ColTab::Sequence::sKeyTransparency = "Transparency";
const char* ColTab::Sequence::sKeyCtbl = "Color table";
static const char* sKeyCtabSettsKey = "coltabs";


ColTab::Sequence::Sequence()
    : undefcolor_(Color::LightGrey)
    , markcolor_(Color::DgbColor)
{
}


ColTab::Sequence::Sequence( const char* nm )
    : NamedObject(nm)
    , undefcolor_(Color::LightGrey)
    , markcolor_(Color::DgbColor)
{
    get(nm,*this);
}


ColTab::Sequence::Sequence( const ColTab::Sequence& ctab )
    : NamedObject(ctab)
    , r_(ctab.r_)
    , g_(ctab.r_)
    , b_(ctab.r_)
    , x_(ctab.x_)
    , tr_(ctab.tr_)
    , undefcolor_(ctab.undefcolor_)
    , markcolor_(ctab.markcolor_)
{
}


ColTab::Sequence& ColTab::Sequence::operator=( const ColTab::Sequence& ctab )
{
    if ( &ctab != this )
    {
	setName( ctab.name() );
	r_ = ctab.r_;
	g_ = ctab.r_;
	b_ = ctab.r_;
	x_ = ctab.x_;
	tr_ = ctab.tr_;
	undefcolor_ = ctab.undefcolor_;
	markcolor_ = ctab.markcolor_;
    }
    return *this;
}


Color ColTab::Sequence::color( float x ) const
{
    const int sz = size();
    if ( sz == 0 || x <= -mDefEps || x >= 1+mDefEps )
	return undefcolor_;

    const unsigned char t = Color::getUChar( transparencyAt(x) );

    float x0 = x_[0];
    if ( sz == 1 || x < x0 + mDefEps )
	return Color( r_[0], g_[0], b_[0], t );
    float x1 = x_[ sz-1 ];
    if ( x > x1 - mDefEps )
	return Color( r_[sz-1], g_[sz-1], b_[sz-1], t );

    for ( int idx=1; idx<sz; idx++ )
    {
	x1 = x_[idx];
	if ( x < x1 + mDefEps )
	{
	    if ( mIsEqual(x,x1,mDefEps) )
		return Color( r_[idx], g_[idx], b_[idx], t );
	    x0 = x_[idx-1];
	    const float frac = (x-x0) / (x1-x0);
#	    define mColVal(c) Color::getUChar( frac*c[idx] + (1-frac)*c[idx-1] )
	    return Color( mColVal(r_), mColVal(g_), mColVal(b_), t );
	}
    }

    pErrMsg( "Should not reach" );
    return undefcolor_;
}


float ColTab::Sequence::transparencyAt( float x ) const
{
    const int sz = tr_.size();
    if ( sz == 0 || x <= -mDefEps || x >= 1+mDefEps )	return 0;

    float x0 = tr_[0].x; float y0 = tr_[0].y;
    if ( sz == 1 || x < x0+mDefEps )			return y0;
    float x1 = tr_[sz-1].x; float y1 = tr_[sz-1].y;
    if ( x > x1 - mDefEps )				return y1;

    for ( int idx=1; idx<sz; idx++ )
    {
	x1 = tr_[idx].x; y1 = tr_[idx].y;
	if ( x < x1 + mDefEps )
	{
	    if ( mIsEqual(x,x1,mDefEps) )
		return y1;
	    x0 = tr_[0].x; y0 = tr_[0].y;
	    const float frac = (x-x0) / (x1-x0);
	    return frac * y1 + (1-frac) * y0;
	}
    }

    pErrMsg( "Should not reach" );
    return 0;
}


bool ColTab::Sequence::hasTransparency() const
{
    if ( tr_.isEmpty() )
	return false;

    for ( int idx=0; idx<tr_.size(); idx++ )
	if ( tr_[idx].y > 0.1 ) return true;

    return false;
}


void ColTab::Sequence::setColor( float px, unsigned char pr, unsigned char pg,
				 unsigned char pb )
{
    if ( px > 1 ) px = 1; if ( px < 0 ) px = 0;
    const int sz = size();

    for ( int idx=0; idx<sz; idx++ )
    {
	const float x = x_[idx];
	if ( mIsEqual(x,px,mDefEps) )
	    { changeColor( idx, pr, pg, pb ); return; }
	else if ( px < x )
	{
	    x_.insert(idx,px);
	    r_.insert(idx,pr); g_.insert(idx,pg); b_.insert(idx,pb);
	    return;
	}
    }

    r_ += pr; g_ += pg; b_ += pb; x_ += px;
}


void ColTab::Sequence::changeColor( int idx, unsigned char pr,
				    unsigned char pg, unsigned char pb )
{
    if ( idx >= 0 && idx < size() )
	{ r_[idx] = pr; g_[idx] = pg; b_[idx] = pb; }
}


void ColTab::Sequence::changePos( int idx, float x )
{
    const int sz = size();
    if ( idx < 0 || idx >= sz ) return;

    if ( x > 1 ) x = 1; if ( x < 0 ) x = 0;

    if ( (idx > 0 && x_[idx-1] >= x) )
	x_[idx] = x_[idx-1] + 1.01*mDefEps;
    if ( (idx < sz-1 && x_[idx+1] <= x) )
	x_[idx] = x_[idx-1] - 1.01*mDefEps;
    else
	x_[idx] = x;
}


void ColTab::Sequence::setTransparency( Geom::Point2D<float> pt )
{
    if ( pt.x < 0 ) pt.x = 0; if ( pt.x > 1 ) pt.x = 1;
    if ( pt.y < 0 ) pt.y = 0; if ( pt.y > 255 ) pt.y = 255;

    for ( int idx=0; idx<tr_.size(); idx++ )
    {
	const float x = tr_[idx].x;
	if ( mIsEqual(x,pt.x,mDefEps) )
	    { tr_[idx] = pt; return; }
	else if ( pt.x < x )
	    { tr_.insert( idx, pt ); return; }
    }

    tr_ += pt;
}


static float getfromPar( const IOPar& iopar, Color& col, const char* key,
			 bool withx=false )
{
    const char* res = iopar.find( key );
    float px = withx ? mUdf(float) : 0;
    if ( res && *res )
    {
	if ( !withx )
	    col.use( res );
	else
	{
	    const FileMultiString fms( res );
	    if ( fms.size() > 1 && col.use( fms.from(1) ) )
		px = atof(fms[0]);
	}
    }
    return px;
}


void ColTab::Sequence::fillPar( IOPar& iopar ) const
{
    iopar.set( sKey::Name, name() );
    FileMultiString fms;
    fms += markcolor_.r(); fms += markcolor_.g(); fms += markcolor_.b();
    iopar.set( sKeyMarkColor, fms );
    fms = "";
    fms += undefcolor_.r(); fms += undefcolor_.g(); fms += undefcolor_.b();
    fms += undefcolor_.t();
    iopar.set( sKeyUdfColor, fms );

    for ( int idx=0; idx<x_.size(); idx++ )
    {
	fms = "";
	fms += x_[idx]; fms += r_[idx]; fms += g_[idx]; fms += b_[idx];
	fms += transparencyAt( x_[idx] );
	BufferString str( sKeyValCol );
	str += "."; str += idx;
	iopar.set( str, fms );
    }

    for ( int idx=0; idx<tr_.size(); idx++ )
    {
	BufferString key( sKeyTransparency );
	key += "."; key += idx;
	iopar.set( key, tr_[idx].x, tr_[idx].y );
    }
}


void ColTab::Sequence::usePar( const IOPar& iopar )
{
    const char* res = iopar.find( sKey::Name );
    if ( res ) setName( res );
    getfromPar( iopar, markcolor_, sKeyMarkColor );
    getfromPar( iopar, undefcolor_, sKeyUdfColor );

    x_.erase(); r_.erase(); g_.erase(); b_.erase(); tr_.erase();
    for ( int idx=0; ; idx++ )
    {
	BufferString key( sKeyValCol );
	key += "."; key += idx;
	Color col;
	float px = getfromPar( iopar, col, key, true );
	if ( mIsUdf(px) )
	{
	    if ( idx )
		break;
	    continue;
	}

	x_ += px;
	r_ += col.r(); g_ += col.g(); b_ += col.b();
    }

    for ( int idx=0; ; idx++ )
    {
	BufferString key( sKeyTransparency );
	key += "."; key += idx;
	Geom::Point2D<float> pt;
	if ( !iopar.get( key, pt.x, pt.y ) ) break;
	tr_ += pt;
    }
}


static ObjectSet<IOPar>& stdTabPars()
{
    static ObjectSet<IOPar>* parset = 0;
    if ( !parset )
    {
	parset = new ObjectSet<IOPar>;
	ColTab::Sequence::getStdTabPars( *parset );
    }
    return *parset;
}


void ColTab::Sequence::getNames( NamedBufferStringSet& names,
				 ColTab::Sequence::Src opt )
{
    names.deepErase();
    names.setName( sKeyCtbl );

    if ( opt != ColTab::Sequence::Sys )
    {
	Settings& setts( Settings::fetch(sKeyCtabSettsKey) );
	if ( setts.size() )
	    add( setts, &names, 0 );
	if ( opt == ColTab::Sequence::UsrDef ) return;
    }

    ObjectSet<IOPar>& tabpars = stdTabPars();
    for ( int idx=0; idx<tabpars.size(); idx++ )
    {
	const char* nm = tabpars[idx]->find( sKey::Name );
	if ( nm && *nm )
	    names.addIfNew( nm );
    }
}


bool ColTab::Sequence::get( const char* nm, ColTab::Sequence& ct,
			    ColTab::Sequence::Src opt )
{
    BufferString ctname = "Seismics";
    if ( nm && *nm )
	ctname = nm;
    else
    {
	BufferString key( IOPar::compKey( "dTect", sKeyCtbl ) );
	mSettUse(get,key.buf(),"Name",ctname);
    }

    if ( opt != ColTab::Sequence::Sys )
    {
	Settings& setts( Settings::fetch(sKeyCtabSettsKey) );
	if ( setts.size() )
	{
	    for ( int idx=0; ; idx++ )
	    {
		PtrMan<IOPar> ctiopar = setts.subselect( idx );
		if ( !ctiopar || !ctiopar->size() )
		{
		    if ( !idx ) continue;
		    break;
		}
		
		if ( ctname == ctiopar->find( sKey::Name ) )
		{
		    ct.usePar( *ctiopar );
		    return true;
		}
	    }
	}
    }

    if ( opt == ColTab::Sequence::UsrDef )
	return false;

    ObjectSet<IOPar>& tabpars = stdTabPars();
    for ( int idx=0; idx<tabpars.size(); idx++ )
    {
	const IOPar& iop = *tabpars[idx];
	if ( ctname == iop.find(sKey::Name) )
	{
	    ct.usePar( iop );
    	    return true;
	}
    }

    return false;
}


void ColTab::Sequence::add( const ColTab::Sequence& ctab )
{
    NamedBufferStringSet names;
    getNames( names, UsrDef );
    const int newidx = names.size();
    IOPar par; ctab.fillPar( par );
    Settings& setts( Settings::fetch(sKeyCtabSettsKey) );
    setts.mergeComp( par, getStringFromInt(newidx) );
    setts.write();
}


static IOPar* readStdTabs()
{
    IOPar* iop = 0;
    BufferString fnm = mGetSetupFileName("ColTabs");
    if ( File_exists(fnm) )
    {
	StreamData sd = StreamProvider( fnm ).makeIStream();
	if ( sd.usable() )
	{
	    ascistream astrm( *sd.istrm );
	    iop = new IOPar( astrm );
	    sd.close();
	}
    }
    return iop;
}


void ColTab::Sequence::getStdTabPars( ObjectSet<IOPar>& parset )
{
    IOPar* iopar = readStdTabs();
    if ( iopar )
	add( *iopar, 0, &parset );
    delete iopar;
}


bool ColTab::Sequence::putStdTabPars( const ObjectSet<IOPar>& parset )
{
    BufferString fname = GetSetupDataFileName( ODSetupLoc_ApplSetupOnly,
	    					"ColTabs" );
    if ( fname.isEmpty() )
	fname = GetSetupDataFileName(ODSetupLoc_SWDirOnly,"ColTabs");

    if ( File_exists(fname) && !File_isWritable(fname)
	&& !File_makeWritable(fname,NO,YES) )
    {
	ErrMsg( "Cannot make standard color tables file writable" );
	return false;
    }

    StreamData sd = StreamProvider( fname ).makeOStream();
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


void ColTab::Sequence::add( const IOPar& iopar, BufferStringSet* names,
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

	const char* res = ctiopar->find( sKey::Name );
	if ( res && *res )
	{
	    if ( names )
		{ names->add( res ); delete ctiopar; }
	    if ( pars )
		*pars += ctiopar;
	}
    }
}
