/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert etc.
 * DATE     : 1996 / Sep 2007
-*/

static const char* rcsID = "$Id: coltabsequence.cc,v 1.38 2012/02/20 10:05:30 cvskris Exp $";

#include "coltabsequence.h"
#include "coltabindex.h"

#include "ascstream.h"
#include "bufstringset.h"
#include "file.h"
#include "iopar.h"
#include "keystrs.h"
#include "oddirs.h"
#include "ptrman.h"
#include "separstr.h"
#include "settings.h"
#include "strmprov.h"
#include <iostream>

const char* ColTab::Sequence::sKeyValCol()	{ return "Value-Color"; }
const char* ColTab::Sequence::sKeyMarkColor()	{ return "Marker color"; }
const char* ColTab::Sequence::sKeyUdfColor()	{ return "Undef color"; }
const char* ColTab::Sequence::sKeyTransparency(){ return "Transparency"; }
const char* ColTab::Sequence::sKeyCtbl()	{ return "Color table"; }
const char* ColTab::Sequence::sKeyNrSegments()	{ return "Nr segments"; }

static const char* sKeyCtabSettsKey = "coltabs";

#define mInitStdMembs(uc,mc) \
      undefcolor_(uc) \
    , markcolor_(mc) \


ColTab::Sequence::Sequence()
    : mInitStdMembs(Color::LightGrey(),Color::DgbColor())
    , nrsegments_( 0 )
    , colorChanged(this)
    , transparencyChanged(this)
    , toBeRemoved(this)
    , type_(User)
{
}


ColTab::Sequence::Sequence( const char* nm )
    : NamedObject(nm)
    , mInitStdMembs(Color::LightGrey(),Color::DgbColor())
    , nrsegments_( 0 )
    , colorChanged(this)
    , transparencyChanged(this)
    , toBeRemoved(this)
    , type_(System)
{
    bool res = false;
    if ( nm && *nm )
	res = ColTab::SM().get( nm , *this );
    if ( !res )
	ColTab::SM().get( defSeqName(), *this );
}


ColTab::Sequence::Sequence( const ColTab::Sequence& ctab )
    : NamedObject(ctab)
    , r_(ctab.r_)
    , g_(ctab.g_)
    , b_(ctab.b_)
    , x_(ctab.x_)
    , tr_(ctab.tr_)
    , type_(ctab.type_)
    , colorChanged( ctab.colorChanged )
    , transparencyChanged( ctab.transparencyChanged )
    , toBeRemoved( ctab.toBeRemoved )
    , nrsegments_( ctab.nrsegments_ )
    , mInitStdMembs(ctab.undefcolor_,ctab.markcolor_)
{
}


ColTab::Sequence::~Sequence()
{
    toBeRemoved.trigger( this );
}


ColTab::Sequence& ColTab::Sequence::operator=( const ColTab::Sequence& ctab )
{
    if ( &ctab != this )
    {
	setName( ctab.name() );
	r_ = ctab.r_;
	g_ = ctab.g_;
	b_ = ctab.b_;
	x_ = ctab.x_;
	tr_ = ctab.tr_;
	nrsegments_ = ctab.nrsegments_;
	undefcolor_ = ctab.undefcolor_;
	markcolor_ = ctab.markcolor_;
	type_ = ctab.type_;
	triggerAll();
    }
    return *this;
}


bool ColTab::Sequence::operator==( const ColTab::Sequence& ctab ) const
{
   if ( ctab.name() != name() ||
	ctab.size() != size() ||
	ctab.tr_.size() != tr_.size() ||
	ctab.undefcolor_ != undefcolor_ ||
	ctab.nrsegments_ != nrsegments_ ||
	ctab.markcolor_ != markcolor_ )          return false;

   for ( int idx=0; idx<size(); idx++ )
   {
      if ( !mIsEqual(ctab.x_[idx],x_[idx],0.001) || ctab.r_[idx] != r_[idx] ||
	   ctab.g_[idx] != g_[idx] || ctab.b_[idx] != b_[idx] )
	  return false;
   }

   for ( int idx=0; idx<tr_.size(); idx++ )
   {
       if ( ctab.tr_[idx] != tr_[idx] )
	   return false;
   }

   return true;
}


bool ColTab::Sequence::operator!=( const ColTab::Sequence& ctab ) const
{ return !(*this==ctab); }


Color ColTab::Sequence::color( float x ) const
{
    const int sz = size();
    if ( sz == 0 || x <= -mDefEps || x >= 1+mDefEps )
	return undefcolor_;

    const bool snaptomarkerbelow = nrsegments_==-1;
    if ( nrsegments_>0 )
	x = snapToSegmentCenter( x );

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
	    if ( snaptomarkerbelow )
		return Color( r_[idx-1], g_[idx-1], b_[idx-1], t );

	    x0 = x_[idx-1];
	    const float frac = (x-x0) / (x1-x0);
#	    define mColVal(c) Color::getUChar( frac*c[idx] + (1-frac)*c[idx-1] )
	    return Color( mColVal(r_), mColVal(g_), mColVal(b_), t );
	}
    }

    pErrMsg( "Should not reach" );
    return undefcolor_;
}


float ColTab::Sequence::transparencyAt( float xpos ) const
{
    const int sz = tr_.size();
    if ( sz == 0 || xpos <= -mDefEps || xpos >= 1+mDefEps )	return 0;

    float x0 = tr_[0].x; float y0 = tr_[0].y;
    if ( sz == 1 || xpos < x0+mDefEps )			return y0;
    float x1 = tr_[sz-1].x; float y1 = tr_[sz-1].y;
    if ( xpos > x1 - mDefEps )				return y1;

    for ( int idx=1; idx<sz; idx++ )
    {
	x1 = tr_[idx].x; y1 = tr_[idx].y;
	if ( xpos < x1 + mDefEps )
	{
	    if ( mIsEqual(xpos,x1,mDefEps) )
		return y1;
	    x0 = tr_[idx-1].x; y0 = tr_[idx-1].y;
	    const float frac = (xpos-x0) / (x1-x0);
	    return frac * y1 + (1-frac) * y0;
	}
    }

    pErrMsg( "Should not reach" );
    return 0;
}


bool ColTab::Sequence::hasTransparency() const
{
    if ( undefcolor_.t() || markcolor_.t() )
	return true;

    if ( tr_.isEmpty() )
	return false;

    for ( int idx=0; idx<tr_.size(); idx++ )
	if ( tr_[idx].y > 0.1 ) return true;

    return false;
}


int ColTab::Sequence::setColor( float px, unsigned char pr, unsigned char pg,
				 unsigned char pb )
{
    if ( px > 1 ) px = 1; if ( px < 0 ) px = 0;
    const int sz = size();

    int chgdidx = -1;
    bool done = false;
    for ( int idx=0; idx<sz; idx++ )
    {
	const float x = x_[idx];
	if ( mIsEqual(x,px,mDefEps) )
	{ changeColor( idx, pr, pg, pb ); done = true; chgdidx = idx; break; }
	else if ( px < x )
	{
	    x_.insert(idx,px);
	    r_.insert(idx,pr); g_.insert(idx,pg); b_.insert(idx,pb);
	    chgdidx = idx;
	    done = true; break;
	}
    }

    if ( !done )
	{ r_ += pr; g_ += pg; b_ += pb; x_ += px; chgdidx = x_.size()-1; }

    colorChanged.trigger();
    return chgdidx;
}


void ColTab::Sequence::changeColor( int idx, unsigned char pr,
				    unsigned char pg, unsigned char pb )
{
    if ( idx >= 0 && idx < size() )
    {
	r_[idx] = pr; g_[idx] = pg; b_[idx] = pb;
	colorChanged.trigger();
    }
}


#define mEps 0.00001

void ColTab::Sequence::changePos( int idx, float x )
{
    const int sz = size();
    if ( idx<0 || idx>=sz ) return;

    if ( x > 1 ) x = 1;
    if ( x < 0 ) x = 0;

    x_[idx] = x;
    colorChanged.trigger();
}


void ColTab::Sequence::removeColor( int idx )
{
    if ( idx>0 && idx<size()-1 )
    {
	x_.remove( idx ); r_.remove( idx ); g_.remove( idx ); b_.remove( idx );
	colorChanged.trigger();
    }
}


void ColTab::Sequence::removeAllColors()
{
    x_.erase(); r_.erase(); g_.erase(); b_.erase();
}


void ColTab::Sequence::setTransparency( Geom::Point2D<float> pt )
{
    if ( pt.x < 0 ) pt.x = 0; if ( pt.x > 1 ) pt.x = 1;
    if ( pt.y < 0 ) pt.y = 0; if ( pt.y > 255 ) pt.y = 255;

    bool done = false;
    for ( int idx=0; idx<tr_.size(); idx++ )
    {
	const float x = tr_[idx].x;
	if ( mIsEqual(x,pt.x,mDefEps) )
	    { tr_[idx] = pt; done = true; break; }
	else if ( pt.x < x )
	    { tr_.insert( idx, pt ); done = true; break; }
    }

    if ( !done )
	tr_ += pt;

    transparencyChanged.trigger();
}


void ColTab::Sequence::removeTransparencies()
{
    tr_.erase();
    transparencyChanged.trigger();
}


void ColTab::Sequence::removeTransparencyAt( int idx )
{
    tr_.remove( idx );
    transparencyChanged.trigger();
}


void ColTab::Sequence::changeTransparency( int idx, Geom::Point2D<float> pt )
{
    if ( !tr_.validIdx(idx) ) return;

    tr_[idx] = pt;
    transparencyChanged.trigger();
}


void ColTab::Sequence::flipColor()
{
    if ( size() == 0 ) return;

    int first = 0;
    int last = size() - 1;
    for ( ; first!=last && first<last; )
    {
	unsigned char uctmp;
	float ftmp;
	mSWAP( r_[first], r_[last], uctmp );
	mSWAP( g_[first], g_[last], uctmp );
	mSWAP( b_[first], b_[last], uctmp );
	mSWAP( x_[first], x_[last], ftmp );

	first++;
	last--;
    }

    for ( int idx=0; idx<size(); idx++ )
	x_[idx] = 1.0-x_[idx];
}


void ColTab::Sequence::flipTransparency()
{
    for ( int idx=0; idx<tr_.size()-1; idx++ )
	tr_[idx].x = 1 - tr_[idx].x;
}


static float getfromPar( const IOPar& iopar, Color& col, const char* key,
			 float* px=0 )
{
    const char* res = iopar.find( key );
    if ( px ) *px = mUdf(float);
    if ( !res || !*res )
	return false;

    if ( !px )
	col.use( res );
    else
    {
	const FileMultiString fms( res );
	if ( fms.size() > 1 && col.use( fms.from(1) ) )
	    *px = toFloat(fms[0]);
    }

    return true;
}


void ColTab::Sequence::fillPar( IOPar& iopar ) const
{
    iopar.set( sKey::Name, name() );
    FileMultiString fms;
    fms += (int)markcolor_.r(); fms += (int)markcolor_.g();
    fms += (int)markcolor_.b(); fms += (int)markcolor_.t();
    iopar.set( sKeyMarkColor(), fms );
    fms.setEmpty();
    fms += (int)undefcolor_.r(); fms += (int)undefcolor_.g();
    fms += (int)undefcolor_.b(); fms += (int)undefcolor_.t();
    iopar.set( sKeyUdfColor(), fms );
    iopar.set( sKeyNrSegments(), nrsegments_ );

    for ( int idx=0; idx<x_.size(); idx++ )
    {
	fms = "";
	fms += x_[idx];
	fms += (int)r_[idx]; fms += (int)g_[idx]; fms += (int)b_[idx];
	fms += transparencyAt( x_[idx] );
	BufferString str( sKeyValCol() );
	str += "."; str += idx;
	iopar.set( str, fms );
    }

    for ( int idx=0; idx<tr_.size(); idx++ )
    {
	BufferString key( sKeyTransparency() );
	key += "."; key += idx;
	iopar.set( key, tr_[idx].x, tr_[idx].y );
    }
}


bool ColTab::Sequence::usePar( const IOPar& iopar )
{
    ColTab::Sequence backup = *this;
    FixedString res = iopar.find( sKey::Name );
    if ( !res )
	return false;

    setName( res );

    if ( !getfromPar( iopar, markcolor_, sKeyMarkColor(), 0 ) ||
	 !getfromPar( iopar, undefcolor_, sKeyUdfColor(), 0 ) )
    {
	*this = backup;
	return false;
    }

    nrsegments_ = 0;
    const bool hadnrsegments = iopar.get( sKeyNrSegments(), nrsegments_ );

    x_.erase(); r_.erase(); g_.erase(); b_.erase(); tr_.erase();
    for ( int idx=0; ; idx++ )
    {
	BufferString key( sKeyValCol() );
	key += "."; key += idx;
	Color col;
	float px;
	getfromPar( iopar, col, key, &px );
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
	BufferString key( sKeyTransparency() );
	key += "."; key += idx;
	Geom::Point2D<float> pt;
	if ( !iopar.get( key, pt.x, pt.y ) ) break;
	tr_ += pt;
    }

    //OD 3.2 and before
    if ( !hadnrsegments && size() > 3 )
    {
	bool found = false;
	for ( int idx=1; idx<size()-2; idx+=2 )
	{
	    if ( position(idx+1)-position(idx) <= 1.01*mEps )
	    {
		removeColor( idx );
		found = true;
	    }
	}

	if ( found )
	{
	    nrsegments_ = -1;
	}
    }

    triggerAll();
    return true;
}


ColTab::SeqMgr& ColTab::SM()
{
    static ColTab::SeqMgr* theinst = 0;
    if ( !theinst )
	theinst = new ColTab::SeqMgr;
    return *theinst;
}


ColTab::SeqMgr::SeqMgr()
    : seqAdded(this)
    , seqRemoved(this)
{
    readColTabs();
}


void ColTab::SeqMgr::refresh()
{
    deepErase( seqs_ );
    readColTabs();
}


void ColTab::SeqMgr::readColTabs()
{
    IOPar* iop = 0;
    BufferString fnm = mGetSetupFileName("ColTabs");
    if ( File::exists(fnm) )
    {
	StreamData sd = StreamProvider( fnm ).makeIStream();
	if ( sd.usable() )
	{
	    ascistream astrm( *sd.istrm );
	    iop = new IOPar( astrm );
	    sd.close();
	}
    }
    if ( iop ) addFromPar( *iop, true );
    delete iop;
    if ( InSysAdmMode() ) return;

    Settings& setts( Settings::fetch(sKeyCtabSettsKey) );
    addFromPar( setts, false );
}


void ColTab::SeqMgr::addFromPar( const IOPar& iop, bool fromsys )
{
    for ( int idx=0; ; idx++ )
    {
	PtrMan<IOPar> ctiopar = iop.subselect( idx );
	if ( !ctiopar || !ctiopar->size() )
	{
	    if ( !idx ) continue;
	    return;
	}
	ColTab::Sequence* newseq = new ColTab::Sequence;
	if ( !newseq->usePar( *ctiopar ) )
	    { delete newseq; continue; }

	if ( newseq->size() < 1 && newseq->transparencySize() < 1 )
	    { delete newseq; continue; }

	int existidx = indexOf( newseq->name() );
	if ( existidx < 0 )
	{
	    newseq->setType( fromsys ? Sequence::System : Sequence::User );
	    add( newseq );
	}
	else
	{
	    newseq->setType( Sequence::Edited );
	    Sequence* oldseq = seqs_[existidx];
	    seqs_.replace( existidx, newseq );
	    oldseq->toBeRemoved.trigger( oldseq );
	    delete oldseq;
	}
    }
}


int ColTab::SeqMgr::indexOf( const char* nm ) const
{
    for ( int idx=0; idx<seqs_.size(); idx++ )
	if ( seqs_[idx]->name() == nm )
	    return idx;
    for ( int idx=0; idx<seqs_.size(); idx++ )
	if ( seqs_[idx]->name().isEqual(nm,true) )
	    return idx;
    return -1;
}


bool ColTab::SeqMgr::get( const char* nm, Sequence& seq )
{
    int idx = indexOf( nm );
    if ( idx < 0 ) return false;
    seq = *get( idx );
    return true;
}


const ColTab::Sequence* ColTab::SeqMgr::getAny( const char* nm ) const
{
    if ( seqs_.isEmpty() )
    {
	ColTab::Sequence* cs = new ColTab::Sequence;
	cs->setColor( 0, 0, 0, 0 );
	cs->setColor( 1, 255, 255, 255 );
	cs->setName( "Grey scales" );
	cs->setType( ColTab::Sequence::User );
	cs->setMarkColor( Color::DgbColor() );
	cs->setUndefColor( Color(255,255,0) );
	((ColTab::SeqMgr*)this)->seqs_ += cs;
	return cs;
    }
    int idx = indexOf( nm );
    if ( idx >= 0 ) return get( idx );
    idx = indexOf( "Rainbow" );
    if ( idx >= 0 ) return get( idx );

    return get( 0 );
}


void ColTab::SeqMgr::getSequenceNames( BufferStringSet& nms )
{
    nms.erase();
    for ( int idx=0; idx<size(); idx++ )
	nms.add( SM().get(idx)->name() );
}


void ColTab::SeqMgr::set( const ColTab::Sequence& seq )
{
    int idx = indexOf( seq.name() );
    if ( idx < 0 )
	add( new ColTab::Sequence(seq) );
    else
	*seqs_[idx] = seq;
}


void ColTab::SeqMgr::remove( int idx )
{
    if ( idx < 0 || idx > size() ) return;
    ColTab::Sequence* seq = seqs_.remove( idx );
    seqRemoved.trigger();
    delete seq;
}


bool ColTab::SeqMgr::write( bool sys, bool applsetup )
{
    if ( !sys )
    {
	Settings& setts( Settings::fetch(sKeyCtabSettsKey) );
	setts.setEmpty();
	int newidx = 1;
	for ( int idx=0; idx<seqs_.size(); idx++ )
	{
	    const ColTab::Sequence& seq = *seqs_[idx];
	    if ( !seq.isSys() )
	    {
		IOPar iop; seq.fillPar( iop );
		char str[255];
		getStringFromInt(newidx,str);
		setts.mergeComp( iop, str );
		newidx++;
	    }
	}
	return setts.write( false );
    }

    const BufferString fnm( applsetup
	    ? GetSetupDataFileName(ODSetupLoc_ApplSetupOnly,"ColTabs",0)
	    : GetSetupDataFileName(ODSetupLoc_SWDirOnly,"ColTabs",0) );
    if ( File::exists(fnm) && !File::isWritable(fnm)
		&& !File::makeWritable(fnm,true,false) )
    {
	BufferString msg( "Cannot make:\n" ); msg == fnm; msg += "\nwritable.";
	ErrMsg( msg ); return false;
    }

    StreamData sd = StreamProvider( fnm ).makeOStream();
    if ( !sd.usable() || !sd.ostrm->good() )
    {
	BufferString msg( "Cannot open:\n" ); msg == fnm; msg += "\nfor write.";
	ErrMsg( msg ); return false;
    }

    ascostream astrm( *sd.ostrm );
    astrm.putHeader( "Color table definitions" );
    IOPar iopar;
    int newidx = 1;
    for ( int idx=0; idx<seqs_.size(); idx++ )
    {
	const ColTab::Sequence& seq = *seqs_[idx];
	if ( seq.isSys() )
	{
	    IOPar iop; seq.fillPar( iop );
	    char str[255];
	    getStringFromInt(newidx,str);
	    iopar.mergeComp( iop, str );
	    newidx++;
	}
    }

    iopar.putTo( astrm );
    return true;
}


float ColTab::Sequence::snapToSegmentCenter( float x ) const
{
    if ( nrsegments_<1 )
	return x;

    if ( mIsUdf(x) )
	return x;

    if ( nrsegments_==1 )
	return 0.5;

    const float segmentsize = 1.0/(nrsegments_-1);

    int segment = (int) (x/segmentsize+0.5 );
    if ( segment<0 ) segment = 0;
    if ( segment>=nrsegments_ )
	segment = nrsegments_-1;
    return segment*segmentsize;
}
