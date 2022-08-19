/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "coltabsequence.h"
#include "coltabindex.h"

#include "ascstream.h"
#include "bendpointfinder.h"
#include "bufstringset.h"
#include "file.h"
#include "iopar.h"
#include "keystrs.h"
#include "oddirs.h"
#include "ptrman.h"
#include "genc.h"
#include "separstr.h"
#include "settings.h"
#include "od_ostream.h"

const char* ColTab::Sequence::sKeyValCol()	{ return "Value-Color"; }
const char* ColTab::Sequence::sKeyMarkColor()	{ return "Marker color"; }
const char* ColTab::Sequence::sKeyUdfColor()	{ return "Undef color"; }
const char* ColTab::Sequence::sKeyTransparency(){ return "Transparency"; }
const char* ColTab::Sequence::sKeyCtbl()	{ return "Color table"; }
const char* ColTab::Sequence::sKeyNrSegments()	{ return "Nr segments"; }
const char* ColTab::Sequence::sKeyRainbow()	{ return "Rainbow"; }

static const char* sKeyCtabSettsKey = "coltabs";


class BendPointFinderColors : public BendPointFinderBase
{
public:
		BendPointFinderColors(const ColTab::Sequence&);
protected:
    float	getMaxSqDistToLine(int& idx,int start,int stop) const override;
    double	sqDistTo(int ipt, int segstart, int segend) const;

    const ColTab::Sequence&	seq_;
};


class BendPointFinderTrans : public BendPointFinderBase
{
public:
		BendPointFinderTrans(const ColTab::Sequence&);
protected:
    float	getMaxSqDistToLine(int& idx,int start, int stop) const override;
    double	sqDistTo(int ipt, int segstart, int segend) const;

    const ColTab::Sequence&	seq_;
};


#define mInitStdMembs(uc,mc) \
      undefcolor_(uc) \
    , markcolor_(mc) \


ColTab::Sequence::Sequence()
    : mInitStdMembs(OD::Color::LightGrey(),OD::Color::DgbColor())
    , nrsegments_( 0 )
    , colorChanged(this)
    , transparencyChanged(this)
    , toBeRemoved(this)
    , type_(User)
{
}


ColTab::Sequence::Sequence( const char* nm )
    : NamedCallBacker(nm)
    , mInitStdMembs(OD::Color::LightGrey(),OD::Color::DgbColor())
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
    : NamedCallBacker(ctab.name())
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
{
    return !(*this==ctab);
}


bool ColTab::Sequence::canChangePosition( int idx ) const
{
    return validIdx(idx) && idx!=0 && idx!=(size()-1);
}


OD::Color ColTab::Sequence::color( float x ) const
{
    const int sz = size();
    if ( sz == 0 || x <= -mDefEps || x >= 1+mDefEps )
	return undefcolor_;

    const bool snaptomarkerbelow = nrsegments_==-1;
    if ( nrsegments_>0 )
	x = snapToSegmentCenter( x );

    const unsigned char t = OD::Color::getUChar( transparencyAt(x) );

    float x0 = x_[0];
    if ( sz == 1 || x < x0 + mDefEps )
	return OD::Color( r_[0], g_[0], b_[0], t );
    float x1 = x_[ sz-1 ];
    if ( x > x1 - mDefEps )
	return OD::Color( r_[sz-1], g_[sz-1], b_[sz-1], t );

    for ( int idx=1; idx<sz; idx++ )
    {
	x1 = x_[idx];
	if ( x < x1 + mDefEps )
	{
	    if ( mIsEqual(x,x1,mDefEps) )
		return OD::Color( r_[idx], g_[idx], b_[idx], t );
	    if ( snaptomarkerbelow )
		return OD::Color( r_[idx-1], g_[idx-1], b_[idx-1], t );

	    x0 = x_[idx-1];
	    const float frac = (x-x0) / (x1-x0);

	    #define mColVal(c)\
		OD::Color::getUChar( frac*c[idx] + (1-frac)*c[idx-1] )

	    return OD::Color( mColVal(r_), mColVal(g_), mColVal(b_), t );
	}
    }

    pErrMsg( "Should not reach" );
    return undefcolor_;
}


float ColTab::Sequence::transparencyAt( float xpos ) const
{
    const int sz = tr_.size();
    if ( sz == 0 || xpos <= -mDefEps || xpos >= 1+mDefEps )
	return 0;

    float x0 = tr_[0].x;
    float y0 = tr_[0].y;
    if ( sz == 1 || xpos < x0+mDefEps )
	return y0;

    float x1 = tr_[sz-1].x;
    float y1 = tr_[sz-1].y;
    if ( xpos > x1 - mDefEps )
	return y1;

    for ( int idx=1; idx<sz; idx++ )
    {
	x1 = tr_[idx].x;
	y1 = tr_[idx].y;
	if ( xpos < x1 + mDefEps )
	{
	    if ( mIsEqual(xpos,x1,mDefEps) )
		return y1;

	    x0 = tr_[idx-1].x;
	    y0 = tr_[idx-1].y;
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
	if ( tr_[idx].y > 0.1f ) return true;

    return false;
}


int ColTab::Sequence::setColor( float pos, const OD::Color& col )
{
    return setColor( pos, col.r(), col.g(), col.b() );
}


int ColTab::Sequence::setColor( float px, unsigned char pr, unsigned char pg,
				unsigned char pb )
{
    if ( px > 1 ) px = 1;
    if ( px < 0 ) px = 0;
    const int sz = size();

    int chgdidx = -1;
    bool done = false;
    for ( int idx=0; idx<sz; idx++ )
    {
	const float x = x_[idx];
	if ( mIsEqual(x,px,mDefEps) )
	{
	    changeColor( idx, pr, pg, pb );
	    done = true;
	    chgdidx = idx;
	    break;
	}
	else if ( px < x )
	{
	    x_.insert( idx, px );
	    r_.insert( idx, pr );
	    g_.insert( idx, pg );
	    b_.insert( idx, pb );
	    chgdidx = idx;
	    done = true;
	    break;
	}
    }

    if ( !done )
    {
	r_ += pr;
	g_ += pg;
	b_ += pb;
	x_ += px;
	chgdidx = x_.size()-1;
    }

    colorChanged.trigger();
    return chgdidx;
}


void ColTab::Sequence::changeColor( int idx, const OD::Color& col )
{
    changeColor( idx, col.r(), col.g(), col.b() );
}


void ColTab::Sequence::changeColor( int idx, unsigned char pr,
				    unsigned char pg, unsigned char pb )
{
    if ( !validIdx(idx) )
	return;

    r_[idx] = pr; g_[idx] = pg; b_[idx] = pb;
    colorChanged.trigger();
}


#define mEps 0.00001

void ColTab::Sequence::changePos( int idx, float x )
{
    if ( !canChangePosition(idx) )
	return;

    if ( x > 1 ) x = 1;
    if ( x < 0 ) x = 0;

    x_[idx] = x;
    colorChanged.trigger();
}


void ColTab::Sequence::removeColor( int idx )
{
    if ( !canChangePosition(idx) )
	return;

    x_.removeSingle( idx );
    r_.removeSingle( idx );
    g_.removeSingle( idx );
    b_.removeSingle( idx );
    colorChanged.trigger();
}


void ColTab::Sequence::removeAllColors()
{
    x_.erase();
    r_.erase();
    g_.erase();
    b_.erase();
}


void ColTab::Sequence::setTransparency( Geom::Point2D<float> pt )
{
    if ( pt.x < 0 ) pt.x = 0;
    if ( pt.x > 1 ) pt.x = 1;
    if ( pt.y < 0 ) pt.y = 0;
    if ( pt.y > 255 ) pt.y = 255;

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
    tr_.removeSingle( idx );
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
	Swap( r_[first], r_[last] );
	Swap( g_[first], g_[last] );
	Swap( b_[first], b_[last] );
	Swap( x_[first], x_[last] );

	first++;
	last--;
    }

    for ( int idx=0; idx<size(); idx++ )
	x_[idx] = 1.0f - x_[idx];
}


void ColTab::Sequence::flipTransparency()
{
    TypeSet< Geom::Point2D<float> > oldtr( tr_ );

    const int sz = oldtr.size();

    for ( int idx=0; idx<sz; idx++ )
    {
	tr_[idx].x = 1 - oldtr[sz-idx-1].x;
	tr_[idx].y = oldtr[sz-idx-1].y;
    }
}


static float getfromPar( const IOPar& iopar, OD::Color& col, const char* key,
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
	    *px = fms.getFValue( 0 );
    }

    return true;
}


void ColTab::Sequence::fillPar( IOPar& iopar ) const
{
    iopar.set( sKey::Name(), name() );
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
    StringView seqnm = iopar.find( sKey::Name() );
    if ( !seqnm )
	return false;

    setName( seqnm );

    getfromPar( iopar, markcolor_, sKeyMarkColor(), 0 );
    getfromPar( iopar, undefcolor_, sKeyUdfColor(), 0 );

    nrsegments_ = 0;
    const bool hadnrsegments = iopar.get( sKeyNrSegments(), nrsegments_ );

    x_.erase(); r_.erase(); g_.erase(); b_.erase(); tr_.erase();
    for ( int idx=0; ; idx++ )
    {
	BufferString key( sKeyValCol() );
	key += "."; key += idx;
	OD::Color col;
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

    if ( isEmpty() && !SM().get(seqnm,*this) )
	return false;

    triggerAll();
    return true;
}


ColTab::SeqMgr& ColTab::SM()
{
    mDefineStaticLocalObject( PtrMan<ColTab::SeqMgr>, theinst,
                              = new ColTab::SeqMgr );
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
    od_istream strm( mGetSetupFileName("ColTabs") );
    if ( strm.isOK() )
    {
	ascistream astrm( strm );
	iop = new IOPar( astrm );
    }
    if ( iop )
	{ addFromPar( *iop, true ); delete iop; }
    if ( InSysAdmMode() )
	return;

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
	if ( seqs_[idx]->name().isEqual(nm,CaseInsensitive) )
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
	cs->setMarkColor( OD::Color::DgbColor() );
	cs->setUndefColor( OD::Color(255,255,0) );
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
    ColTab::Sequence* seq = seqs_.removeSingle( idx );
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
		BufferString str; str.set( newidx );
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
	BufferString msg( "Cannot make:\n" ); msg += fnm; msg += "\nwritable.";
	ErrMsg( msg ); return false;
    }

    od_ostream strm( fnm );
    if ( !strm.isOK() )
    {
	BufferString msg( "Cannot open:\n", fnm, "\nfor write" );
	strm.addErrMsgTo( msg );
	ErrMsg( msg ); return false;
    }

    ascostream astrm( strm );
    astrm.putHeader( "Color table definitions" );
    IOPar iopar;
    int newidx = 1;
    for ( int idx=0; idx<seqs_.size(); idx++ )
    {
	const ColTab::Sequence& seq = *seqs_[idx];
	if ( seq.isSys() )
	{
	    IOPar iop; seq.fillPar( iop );
	    BufferString str; str.set( newidx );
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

    const float segmentsize = 1.0f / (nrsegments_ - 1);

    int segment = (int) ( x/segmentsize + 0.5 );
    if ( segment<0 ) segment = 0;
    if ( segment>=nrsegments_ )
	segment = nrsegments_-1;
    return segment*segmentsize;
}


void ColTab::Sequence::simplify()
{
    BendPointFinderColors cfinder( *this );
    cfinder.executeParallel( false );
    const TypeSet<int>& bpc = cfinder.bendPoints();
    if ( bpc.size() )
    {
	for ( int idx=size()-1; idx>=0; idx-- )
	{
	    if ( !bpc.isPresent(idx) )
		removeColor( idx );
	}
    }

    BendPointFinderTrans tfinder( *this );
    tfinder.executeParallel( false );
    const TypeSet<int>& bpt = tfinder.bendPoints();
    if ( bpt.size() )
    {
	for ( int idx=transparencySize()-1; idx>=0; idx-- )
	{
	    if ( !bpt.isPresent(idx) )
		removeTransparencyAt( idx );
	}
    }
}


BendPointFinderColors::BendPointFinderColors( const ColTab::Sequence& seq )
    : BendPointFinderBase(seq.size(), 1.f/255.f)
    , seq_(seq)
{}


float BendPointFinderColors::getMaxSqDistToLine( int& idx, int start,
						 int stop ) const
{
    if ( stop-start==2 )
    {
	idx = start+1;
	float pos = ( seq_.position(start) + seq_.position(stop) ) / 2.f;
	float r = ( seq_.r(start) + seq_.r(stop) ) / 2.f;
	float g = ( seq_.g(start) + seq_.g(stop) ) / 2.f;
	float b = ( seq_.b(start) + seq_.b(stop) ) / 2.f;
	pos  = ( seq_.position(idx) - pos );
	r = ( seq_.r(idx) - r ) / 255.f;
	g = ( seq_.g(idx) - g ) / 255.f;
	b = ( seq_.b(idx) - b ) / 255.f;
	return pos*pos + r*r + g*g + b*b;
    }

    double dsqmax = 0.0;
    for ( int ipt=start+1; ipt<stop; ipt++ )
    {
	const double dsq = sqDistTo( ipt, start, stop );
	if ( dsq>dsqmax )
	{
	    dsqmax = dsq;
	    idx = ipt;
	}
    }
    return float( dsqmax );
}


double BendPointFinderColors::sqDistTo( int ipt, int segstrt, int segend ) const
{
    const double pos_s = seq_.position(segend) - seq_.position(segstrt);
    const double r_s = ( seq_.r(segend) - seq_.r(segstrt) ) / 255.;
    const double g_s = ( seq_.g(segend) - seq_.g(segstrt) ) / 255.;
    const double b_s = ( seq_.b(segend) - seq_.b(segstrt) ) / 255.;

    double pos_p = seq_.position(ipt) - seq_.position(segstrt);
    double r_p = ( seq_.r(ipt) - seq_.r(segstrt) ) / 255.;
    double g_p = ( seq_.g(ipt) - seq_.g(segstrt) ) / 255.;
    double b_p = ( seq_.b(ipt) - seq_.b(segstrt) ) / 255.;

    const double t = (pos_p*pos_s + r_p*r_s + g_p*g_s + b_p*b_s)/
				    (pos_s*pos_s + r_s*r_s + g_s*g_s + b_s*b_s);
    pos_p -= t*pos_s;
    r_p -= t*r_s;
    g_p -= t*g_s;
    b_p -= t*b_s;
    return pos_p*pos_p + r_p*r_p + g_p*g_p + b_p*b_p;
}


BendPointFinderTrans::BendPointFinderTrans( const ColTab::Sequence& seq )
    : BendPointFinderBase(seq.transparencySize(), 1.f/255.f)
    , seq_(seq)
{}


float BendPointFinderTrans::getMaxSqDistToLine( int& idx, int start,
						 int stop ) const
{
    if ( stop-start==2 )
    {
	idx = start+1;
	const Geom::PointF trans = ( seq_.transparency(start) +
					       seq_.transparency(stop) ) / 2.f;
	const Geom::PointF trans_idx = seq_.transparency( idx );
	const float pos = trans_idx.x - trans.x;
	const float tmp = ( trans_idx.y - trans.y ) / 255.f;
	return pos*pos + tmp*tmp;
    }

    double dsqmax = 0.0;
    for ( int ipt=start+1; ipt<stop; ipt++ )
    {
	const double dsq = sqDistTo( ipt, start, stop );
	if ( dsq>dsqmax )
	{
	    dsqmax = dsq;
	    idx = ipt;
	}
    }
    return float( dsqmax );
}


double BendPointFinderTrans::sqDistTo( int ipt, int segstrt, int segend ) const
{
    const Geom::PointF tr_s = seq_.transparency(segend) -
						    seq_.transparency(segstrt);
    const double pos_s = tr_s.x;
    const double trans_s = tr_s.y / 255.;

    const Geom::PointF tr_p = seq_.transparency(ipt) -
						    seq_.transparency(segstrt);
    double pos_p = tr_p.x;
    double trans_p = tr_p.y / 255.;

    const double t = (pos_p*pos_s + trans_p*trans_s)/
						(pos_s*pos_s + trans_s*trans_s);
    pos_p -= t*pos_s;
    trans_p -= t*trans_s;
    return pos_p*pos_p + trans_p*trans_p;
}
