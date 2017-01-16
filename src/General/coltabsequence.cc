/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert etc.
 * DATE     : 1996 / Sep 2007
-*/


#include "coltabsequence.h"
#include "coltabindex.h"

#include "ascstream.h"
#include "bufstringset.h"
#include "file.h"
#include "iopar.h"
#include "keystrs.h"
#include "oddirs.h"
#include "ptrman.h"
#include "genc.h"
#include "separstr.h"
#include "settings.h"
#include "envvars.h"
#include "perthreadrepos.h"
#include "od_ostream.h"

const char* ColTab::Sequence::sKeyValCol()	{ return "Value-Color"; }
const char* ColTab::Sequence::sKeyMarkColor()	{ return "Marker color"; }
const char* ColTab::Sequence::sKeyUdfColor()	{ return "Undef color"; }
const char* ColTab::Sequence::sKeyTransparency(){ return "Transparency"; }
const char* ColTab::Sequence::sKeyCtbl()	{ return "Color table"; }
const char* ColTab::Sequence::sKeyNrSegments()	{ return "Nr segments"; }

static const char* sKeyCtabSettsKey = "coltabs";
mDefineInstanceCreatedNotifierAccess(ColTab::Sequence);


static const char* sKeyDefName = "dTect.Disp.Default Color table";
static const char* sKeyDefNameOld = "dTect.Color table.Name";

const char* ColTab::Sequence::sDefaultName( bool for_seismics )
{
    const char* envval = GetEnvVar( for_seismics ? "OD_DEFAULT_SEIS_COLOR_TABLE"
						 : "OD_DEFAULT_COLOR_TABLE" );
    if ( envval )
	return envval;

    BufferString defcoltabnm;
    if ( !Settings::common().get(sKeyDefNameOld,defcoltabnm) )
	  Settings::common().get( sKeyDefName, defcoltabnm);
    if ( defcoltabnm.isEmpty() )
	return for_seismics ? "Seismics" : "Viridis";

    mDeclStaticString( ret );
    ret = defcoltabnm;
    return ret;
}


const char* ColTab::defSeqName()
{
    return ColTab::Sequence::sDefaultName( true );
}


#define mInitMembs() \
      NamedMonitorable(0) \
    , undefcolor_(Color::LightGrey()) \
    , markcolor_(Color::DgbColor()) \
    , nrsegments_( 0 ) \
    , type_(User)


ColTab::Sequence::Sequence()
    : mInitMembs()
{
    mTriggerInstanceCreatedNotifier();
}


ColTab::Sequence::Sequence( const char* nm )
    : mInitMembs()
{
    bool gotrequested = false;
    if ( nm && *nm )
	gotrequested = ColTab::SM().get( nm , *this );
    if ( !gotrequested )
	ColTab::SM().get( defSeqName(), *this );
    mTriggerInstanceCreatedNotifier();
}


ColTab::Sequence::Sequence( const ColTab::Sequence& oth )
    : mInitMembs()
{
    copyAll( oth );
    mTriggerInstanceCreatedNotifier();
}


ColTab::Sequence::~Sequence()
{
    sendDelNotif();
}


mImplMonitorableAssignment( ColTab::Sequence, NamedMonitorable )


void ColTab::Sequence::copyClassData( const Sequence& oth )
{
    r_ = oth.r_;
    g_ = oth.g_;
    b_ = oth.b_;
    x_ = oth.x_;
    tr_ = oth.tr_;
    nrsegments_ = oth.nrsegments_;
    undefcolor_ = oth.undefcolor_;
    markcolor_ = oth.markcolor_;
    type_ = oth.type_;
}



bool ColTab::Sequence::operator==( const ColTab::Sequence& oth ) const
{
    if ( !NamedMonitorable::operator==(oth) )
	return false;

    mLock4Read();
    MonitorLock ml( oth );

    const size_type sz = gtSize();
    if ( sz != oth.gtSize()
      || oth.tr_.size() != tr_.size()
      || oth.undefcolor_ != undefcolor_
      || oth.markcolor_ != markcolor_
      || oth.nrsegments_ != nrsegments_ )
	return false;

   for ( IdxType idx=0; idx<sz; idx++ )
   {
      if ( !mIsEqual(oth.x_[idx],x_[idx],0.001) || oth.r_[idx] != r_[idx] ||
	   oth.g_[idx] != g_[idx] || oth.b_[idx] != b_[idx] )
	  return false;
   }

   for ( IdxType idx=0; idx<tr_.size(); idx++ )
   {
       if ( oth.tr_[idx] != tr_[idx] )
	   return false;
   }

   return true;
}


ColTab::Sequence::size_type ColTab::Sequence::size() const
{
    mLock4Read();
    return x_.size();
}


Color ColTab::Sequence::color( PosType x ) const
{
    mLock4Read();
    const size_type sz = gtSize();
    if ( sz == 0 || x <= -mDefEps || x >= 1+mDefEps )
	return undefcolor_;

    const bool snaptomarkerbelow = nrsegments_ == -1;
    if ( nrsegments_ > 0 )
	x = snapToSegmentCenter( x );

    ValueType t = Color::getUChar( gtTransparencyAt(x) );

    PosType x0 = x_[0];
    if ( sz == 1 || x < x0 + mDefEps )
	return Color( r_[0], g_[0], b_[0], t );
    PosType x1 = x_[ sz-1 ];
    if ( x > x1 - mDefEps )
	return Color( r_[sz-1], g_[sz-1], b_[sz-1], t );

    for ( IdxType idx=1; idx<sz; idx++ )
    {
	x1 = x_[idx];
	if ( x < x1 + mDefEps )
	{
	    if ( mIsEqual(x,x1,mDefEps) )
		return Color( r_[idx], g_[idx], b_[idx], t );
	    if ( snaptomarkerbelow )
		return Color( r_[idx-1], g_[idx-1], b_[idx-1], t );

	    x0 = x_[idx-1];
	    const PosType frac = (x-x0) / (x1-x0);
#	    define mColVal(c) Color::getUChar( frac*c[idx] + (1-frac)*c[idx-1] )
	    return Color( mColVal(r_), mColVal(g_), mColVal(b_), t );
	}
    }

    pErrMsg( "Should not reach" );
    return undefcolor_;
}


ColTab::Sequence::PosType ColTab::Sequence::position( IdxType idx ) const
{
    mLock4Read();
    return x_[idx];
}


ColTab::Sequence::ValueType ColTab::Sequence::gtTransparencyAt(
						PosType xpos ) const
{
    const size_type sz = tr_.size();
    if ( sz == 0 || xpos <= -mDefEps || xpos >= 1+mDefEps )
	return 0;

    PosType x0 = tr_[0].first; ValueType y0 = tr_[0].second;
    if ( sz == 1 || xpos < x0+mDefEps )
	return y0;
    PosType x1 = tr_[sz-1].first; ValueType y1 = tr_[sz-1].second;
    if ( xpos > x1 - mDefEps )
	return y1;

    for ( IdxType idx=1; idx<sz; idx++ )
    {
	x1 = tr_[idx].first; y1 = tr_[idx].second;
	if ( xpos < x1 + mDefEps )
	{
	    if ( mIsEqual(xpos,x1,mDefEps) )
		return y1;
	    x0 = tr_[idx-1].first; y0 = tr_[idx-1].second;
	    const float frac = (xpos-x0) / (x1-x0);
	    const float val = frac * y1 + (1-frac) * y0;
	    return mRounded( ValueType, val );
	}
    }

    pErrMsg( "Should not reach" );
    return 0;
}


bool ColTab::Sequence::hasTransparency() const
{
    mLock4Read();

    if ( undefcolor_.t() || markcolor_.t() )
	return true;

    if ( tr_.isEmpty() )
	return false;

    for ( IdxType idx=0; idx<tr_.size(); idx++ )
	if ( tr_[idx].second > 0 ) return true;

    return false;
}


ColTab::Sequence::IdxType ColTab::Sequence::setColor( PosType px,
				 ValueType pr, ValueType pg, ValueType pb )
{
    mLock4Write();

    if ( px > 1 ) px = 1; if ( px < 0 ) px = 0;
    const size_type sz = gtSize();

    IdxType chgdidx = -1;
    bool done = false;
    for ( IdxType idx=0; idx<sz; idx++ )
    {
	const PosType x = x_[idx];
	if ( mIsEqual(x,px,mDefEps) )
	{
	    chgColor( idx, pr, pg, pb );
	    done = true; chgdidx = idx;
	    break;
	}
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

    mSendChgNotif( cColorChange(), chgdidx );
    return chgdidx;
}


void ColTab::Sequence::changeColor( IdxType idx,
				 ValueType pr, ValueType pg, ValueType pb )
{
    mLock4Write();
    if ( chgColor(idx,pr,pg,pb) )
	mSendChgNotif( cColorChange(), idx );
}


bool ColTab::Sequence::chgColor( IdxType idx,
				 ValueType pr, ValueType pg, ValueType pb )
{
    if ( !x_.validIdx(idx) )
	return false;

    r_[idx] = pr; g_[idx] = pg; b_[idx] = pb;
    return true;
}


#define mEps 0.00001

void ColTab::Sequence::changePos( IdxType idx, PosType x )
{
    if ( x > 1 ) x = 1;
    if ( x < 0 ) x = 0;

    mLock4Write();
    const size_type sz = gtSize();
    if ( idx<0 || idx>=sz )
	return;

    x_[idx] = x;
    mSendChgNotif( cColorChange(), idx );
}


void ColTab::Sequence::removeColor( IdxType idx )
{
    mLock4Write();
    if ( rmColor(idx) )
	mSendChgNotif( cColorChange(), idx );
}


bool ColTab::Sequence::rmColor( IdxType idx )
{
    if ( !x_.validIdx(idx) )
	return false;

    x_.removeSingle( idx );
    r_.removeSingle( idx );
    g_.removeSingle( idx );
    b_.removeSingle( idx );
    return true;
}


void ColTab::Sequence::removeAllColors()
{
    if ( !isEmpty() )
    {
	mLock4Write();
	x_.erase(); r_.erase(); g_.erase(); b_.erase();
	mSendEntireObjChgNotif();
    }
}


ColTab::Sequence::size_type ColTab::Sequence::transparencySize() const
{
    mLock4Read();
    return tr_.size();
}


ColTab::Sequence::TranspPtType ColTab::Sequence::transparency(
						IdxType idx ) const
{
    mLock4Read();
    return tr_.validIdx(idx) ? tr_[idx] : TranspPtType( 0.f, 0 );
}


ColTab::Sequence::ValueType ColTab::Sequence::transparencyAt( PosType px ) const
{
    mLock4Read();
    return gtTransparencyAt( px );
}


void ColTab::Sequence::setTransparency( TranspPtType pt )
{
    if ( pt.first < 0 ) pt.first = 0.f;
    if ( pt.first > 1 ) pt.first = 1.f;
    if ( pt.second < 0 ) pt.second = 0.f;
    if ( pt.second > 255 ) pt.second = 255.f;

    mLock4Write();
    bool done = false; IdxType chgidx = -1;
    for ( IdxType idx=0; idx<tr_.size(); idx++ )
    {
	const PosType x = tr_[idx].first;
	if ( mIsEqual(x,pt.first,mDefEps) )
	    { chgidx = idx; tr_[idx] = pt; done = true; break; }
	else if ( pt.first < x )
	    { chgidx = idx; tr_.insert( idx, pt ); done = true; break; }
    }

    if ( !done )
    {
	tr_ += pt;
	chgidx = tr_.size()-1;
    }

    mSendChgNotif( cTransparencyChange(), chgidx );
}


void ColTab::Sequence::removeTransparencies()
{
    mLock4Write();
    tr_.erase();
    mSendEntireObjChgNotif();
}


void ColTab::Sequence::removeTransparencyAt( IdxType idx )
{
    mLock4Write();
    if ( tr_.validIdx(idx) )
    {
	tr_.removeSingle( idx );
	mSendChgNotif( cTransparencyChange(), idx );
    }
}


void ColTab::Sequence::changeTransparency( IdxType idx, TranspPtType pt )
{
    mLock4Write();
    if ( !tr_.validIdx(idx) )
	return;

    tr_[idx] = pt;
    mSendChgNotif( cTransparencyChange(), idx );
}


void ColTab::Sequence::flipColor()
{
    mLock4Write();
    if ( size() == 0 ) return;

    IdxType first = 0;
    IdxType last = size() - 1;
    for ( ; first!=last && first<last; )
    {
	Swap( r_[first], r_[last] );
	Swap( g_[first], g_[last] );
	Swap( b_[first], b_[last] );
	Swap( x_[first], x_[last] );

	first++;
	last--;
    }

    for ( IdxType idx=0; idx<size(); idx++ )
	x_[idx] = 1.0f - x_[idx];
    mSendEntireObjChgNotif();
}


void ColTab::Sequence::flipTransparency()
{
    mLock4Write();
    const TypeSet<TranspPtType> oldtr( tr_ );

    const size_type sz = oldtr.size();

    for ( IdxType idx=0; idx<sz; idx++ )
    {
	tr_[idx].first = 1 - oldtr[sz-idx-1].first;
	tr_[idx].second = oldtr[sz-idx-1].second;
    }
    mSendEntireObjChgNotif();
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
	    *px = fms.getFValue( 0 );
    }

    return true;
}


void ColTab::Sequence::fillPar( IOPar& iopar ) const
{
    mLock4Read();

    putNameInPar( iopar );
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
	fms += gtTransparencyAt( x_[idx] );
	BufferString str( sKeyValCol() );
	str += "."; str += idx;
	iopar.set( str, fms );
    }

    for ( IdxType idx=0; idx<tr_.size(); idx++ )
    {
	BufferString key( sKeyTransparency() );
	key += "."; key += idx;
	iopar.set( key, tr_[idx].first, (float)tr_[idx].second );
    }
}


bool ColTab::Sequence::usePar( const IOPar& iopar )
{
    FixedString res = iopar.find( sKey::Name() );
    if ( !res )
	return false;

    mLock4Write();
    if ( !getfromPar( iopar, markcolor_, sKeyMarkColor(), 0 ) ||
	 !getfromPar( iopar, undefcolor_, sKeyUdfColor(), 0 ) )
	return false;

    name_ = res;

    nrsegments_ = 0;
    const bool hadnrsegments = iopar.get( sKeyNrSegments(), nrsegments_ );

    x_.erase(); r_.erase(); g_.erase(); b_.erase(); tr_.erase();
    for ( IdxType idx=0; ; idx++ )
    {
	BufferString key( sKeyValCol() );
	key += "."; key += idx;
	Color col;
	PosType px;
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

    for ( IdxType idx=0; ; idx++ )
    {
	BufferString key( sKeyTransparency() );
	key += "."; key += idx;
	TranspPtType pt; float floatval;
	if ( !iopar.get( key, pt.first, floatval ) )
	    break;

	pt.second = mRounded( ValueType, floatval );
	tr_ += pt;
    }

    //OD 3.2 and before
    if ( !hadnrsegments && gtSize() > 3 )
    {
	bool found = false;
	for ( IdxType idx=1; idx<gtSize()-2; idx+=2 )
	{
	    if ( x_[idx+1]-x_[idx] <= 1.01*mEps )
	    {
		rmColor( idx );
		found = true;
	    }
	}

	if ( found )
	    nrsegments_ = -1;
    }

    mSendEntireObjChgNotif();
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


ColTab::SeqMgr::~SeqMgr()
{
    deepErase( seqs_ );
    deepErase( removedseqs_ );
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
    for ( IdxType idx=0; ; idx++ )
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

	IdxType existidx = indexOf( newseq->name() );
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
	    delete oldseq;
	}
    }
}


ColTab::SeqMgr::IdxType ColTab::SeqMgr::indexOf( const char* nm ) const
{
    for ( IdxType idx=0; idx<seqs_.size(); idx++ )
	if ( seqs_[idx]->name() == nm )
	    return idx;
    for ( IdxType idx=0; idx<seqs_.size(); idx++ )
	if ( seqs_[idx]->name().isEqual(nm,CaseInsensitive) )
	    return idx;
    return -1;
}


bool ColTab::SeqMgr::get( const char* nm, Sequence& seq )
{
    IdxType idx = indexOf( nm );
    if ( idx < 0 ) return false;
    seq = *get( idx );
    return true;
}


const ColTab::Sequence& ColTab::SeqMgr::getAny( const char* nm ) const
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
	return *cs;
    }

    IdxType idx = 0;
    if ( nm && *nm )
    {
	idx = indexOf( nm );
	if ( idx >= 0 )
	    return *get( idx );
    }

    idx = indexOf( Sequence::sDefaultName() );
    if ( idx < 0 )
	idx = 0;

    return *get( idx );
}


void ColTab::SeqMgr::getSequenceNames( BufferStringSet& nms )
{
    nms.erase();
    for ( IdxType idx=0; idx<size(); idx++ )
	nms.add( SM().get(idx)->name() );
}


void ColTab::SeqMgr::set( const ColTab::Sequence& seq )
{
    IdxType idx = indexOf( seq.name() );
    if ( idx < 0 )
	add( new ColTab::Sequence(seq) );
    else
	*seqs_[idx] = seq;
}


void ColTab::SeqMgr::remove( IdxType idx )
{
    if ( idx < 0 || idx > size() )
	return;
    removedseqs_ += seqs_.removeSingle( idx );
    seqRemoved.trigger();
}


bool ColTab::SeqMgr::write( bool sys, bool applsetup )
{
    if ( !sys )
    {
	Settings& setts( Settings::fetch(sKeyCtabSettsKey) );
	setts.setEmpty();
	IdxType newidx = 1;
	for ( IdxType idx=0; idx<seqs_.size(); idx++ )
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
    IdxType newidx = 1;
    for ( IdxType idx=0; idx<seqs_.size(); idx++ )
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


ColTab::Sequence::PosType ColTab::Sequence::snapToSegmentCenter(
							PosType x ) const
{
    if ( nrsegments_<1 )
	return x;

    if ( mIsUdf(x) )
	return x;

    if ( nrsegments_==1 )
	return 0.5;

    const float segmentsize = 1.0f / (nrsegments_ - 1);

    IdxType segment = (IdxType) ( x/segmentsize + 0.5 );
    if ( segment<0 ) segment = 0;
    if ( segment>=nrsegments_ )
	segment = nrsegments_-1;
    return segment*segmentsize;
}

ColTab::Sequence RGBBlend::getRedColTab()
{
    mDefineStaticLocalObject( ColTab::Sequence, redctab, );
    redctab.setType(ColTab::Sequence::User);
    redctab.setColor( 0, 0, 0, 0 );
    redctab.setColor( 1, 255, 0, 0 );
    redctab.setName( "Red" );
    return redctab;
}

ColTab::Sequence RGBBlend::getGreenColTab()
{
    mDefineStaticLocalObject( ColTab::Sequence, greenctab, );
    greenctab.setType(ColTab::Sequence::User);
    greenctab.setColor( 0, 0, 0, 0 );
    greenctab.setColor( 1, 0, 0, 255 );
    greenctab.setName( "Blue" );
    return greenctab;
}


ColTab::Sequence RGBBlend::getBlueColTab()
{
    mDefineStaticLocalObject( ColTab::Sequence, bluectab, );
    bluectab.setType(ColTab::Sequence::User);
    bluectab.setColor( 0, 0, 0, 0 );
    bluectab.setColor( 1, 0, 255, 0 );
    bluectab.setName( "Green" );
    return bluectab;
}


ColTab::Sequence RGBBlend::getTransparencyColTab()
{
    mDefineStaticLocalObject( ColTab::Sequence, transpctab, );
    transpctab.setType(ColTab::Sequence::User);
    transpctab.setColor( 0.f, 0, 0, 0 );
    transpctab.setColor( 1.f, 255, 255, 255 );
    transpctab.setTransparency( ColTab::Sequence::TranspPtType(0.f,0.f) );
    transpctab.setTransparency( ColTab::Sequence::TranspPtType(1.f,255.f) );
    transpctab.setName( "Transparency" );
    return transpctab;
}


ColTab::Sequence RGBBlend::getColTab( int nr )
{
    switch ( nr )
    {
	case 0: return getRedColTab();
	case 1: return getGreenColTab();
	case 2: return getBlueColTab();
	case 3: return getTransparencyColTab();
    }

    return ColTab::Sequence();
}
