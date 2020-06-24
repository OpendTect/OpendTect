/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert etc.
 * DATE     : 1996 / Sep 2007
-*/


#include "coltabseqmgr.h"

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
#include "staticstring.h"
#include "od_ostream.h"
#include "odruncontext.h"

const char* ColTab::Sequence::sKeyValCol()	{ return "Value-Color"; }
const char* ColTab::Sequence::sKeyMarkColor()	{ return "Marker color"; }
const char* ColTab::Sequence::sKeyUdfColor()	{ return "Undef color"; }
const char* ColTab::Sequence::sKeyTransparency(){ return "Transparency"; }
const char* ColTab::Sequence::sKeyCtbl()	{ return "Color table"; }
const char* ColTab::Sequence::sKeyNrSegments()	{ return "Nr segments"; }
const char* ColTab::Sequence::sKeyDisabled()	{ return "Disabled"; }
const char* ColTab::SequenceManager::sKeyRemoved() { return "System.Hide"; }

static const char* sKeyCtabSettsKey = "coltabs";
mDefineInstanceCreatedNotifierAccess(ColTab::Sequence);

mDefineEnumUtils(ColTab::Sequence,Status,"Color Sequence Status")
{ "System", "Edited", "Added", 0 };

template<>
void EnumDefImpl<ColTab::Sequence::Status>::init()
{
    uistrings_ += mEnumTr("System",0);
    uistrings_ += mEnumTr("Edited",0);
    uistrings_ += mEnumTr("Added",0);
}

static const char* sKeyDefNameSeis =
			"dTect.Disp.Default Color Table.OD Seismic 1";
static const char* sKeyDefNameAttrib
			    = "dTect.Disp.Default Color Table.Attributes";
static const char* sKeyDefNameOld = "dTect.Color table.Name";


const char* ColTab::Sequence::sDefaultName( bool for_seismics )
{
    const char* envval = GetEnvVar( for_seismics ? "OD_DEFAULT_SEIS_COLOR_TABLE"
						 : "OD_DEFAULT_COLOR_TABLE" );
    if ( envval )
	return envval;

    BufferString defcoltabnm;
    Settings::common().get( for_seismics ? sKeyDefNameSeis
					 : sKeyDefNameAttrib, defcoltabnm );
    if ( defcoltabnm.isEmpty() )
    {
	Settings::common().get( sKeyDefNameOld, defcoltabnm );
	if ( defcoltabnm.isEmpty() )
	    return for_seismics ? "OD Seismic 1" : "Pastel";
    }

    mDeclStaticString( ret );
    ret = defcoltabnm;
    return ret;
}


void ColTab::setDefSeqName( bool for_seismics, const char* nm )
{
    Settings::common().removeWithKey( sKeyDefNameOld );
    Settings::common().update(
		for_seismics ? sKeyDefNameSeis : sKeyDefNameAttrib, nm );
    Settings::common().write();
}


const char* ColTab::defSeqName( bool for_seismics )
{
    return ColTab::Sequence::sDefaultName( for_seismics );
}


ColTab::Sequence::Sequence()
{
    mTriggerInstanceCreatedNotifier();
}


ColTab::Sequence::Sequence( const char* nm )
{
    if ( nm && *nm )
	*this = *SeqMGR().getAny( nm );
    mTriggerInstanceCreatedNotifier();
}


ColTab::Sequence::Sequence( const ColTab::Sequence& oth )
    : SharedObject(oth)
{
    copyClassData( oth );
    mTriggerInstanceCreatedNotifier();
}


ColTab::Sequence::~Sequence()
{
    sendDelNotif();
}


mImplMonitorableAssignment( ColTab::Sequence, SharedObject )


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
    disabled_ = oth.disabled_;
}


Monitorable::ChangeType ColTab::Sequence::compareClassData(
					const Sequence& oth ) const
{
    const size_type sz = gtSize();
    if ( sz != oth.gtSize()
      || oth.tr_.size() != tr_.size()
      || oth.nrsegments_ != nrsegments_
      || oth.undefcolor_ != undefcolor_
      || oth.markcolor_ != markcolor_ )
	return cEntireObjectChange();

    for ( idx_type idx=0; idx<sz; idx++ )
    {
	if ( !mIsEqual(oth.x_[idx],x_[idx],0.001)
	  || oth.r_[idx] != r_[idx] || oth.g_[idx] != g_[idx]
	  || oth.b_[idx] != b_[idx] )
	return cEntireObjectChange();
    }

    for ( idx_type idx=0; idx<tr_.size(); idx++ )
    {
	if ( oth.tr_[idx] != tr_[idx] )
	    return cEntireObjectChange();
    }

    return cNoChange();
}


ColTab::Sequence::size_type ColTab::Sequence::size() const
{
    mLock4Read();
    return gtSize();
}


ColTab::Sequence::Status ColTab::Sequence::status() const
{
    return SeqMGR().statusOf( *this );
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

    CompType t = Color::ratio2Comp( gtTransparencyAt(x) );

    PosType x0 = x_[0];
    if ( sz == 1 || x < x0 + mDefEps )
	return Color( r_[0], g_[0], b_[0], t );
    PosType x1 = x_[ sz-1 ];
    if ( x > x1 - mDefEps )
	return Color( r_[sz-1], g_[sz-1], b_[sz-1], t );

    for ( idx_type idx=1; idx<sz; idx++ )
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
#	    define mColVal(c) \
		Color::ratio2Comp( frac*c[idx] + (1-frac)*c[idx-1] )
	    return Color( mColVal(r_), mColVal(g_), mColVal(b_), t );
	}
    }

    pErrMsg( "Should not reach" );
    return undefcolor_;
}


ColTab::PosType ColTab::Sequence::position( idx_type idx ) const
{
    mLock4Read();
    return x_[idx];
}


ColTab::Sequence::CompType ColTab::Sequence::gtTransparencyAt(
						PosType xpos ) const
{
    const size_type sz = tr_.size();
    if ( sz == 0 || xpos <= -mDefEps || xpos >= 1+mDefEps )
	return 0;

    PosType x0 = tr_[0].first; CompType y0 = tr_[0].second;
    if ( sz == 1 || xpos < x0+mDefEps )
	return y0;
    PosType x1 = tr_[sz-1].first; CompType y1 = tr_[sz-1].second;
    if ( xpos > x1 - mDefEps )
	return y1;

    for ( idx_type idx=1; idx<sz; idx++ )
    {
	x1 = tr_[idx].first; y1 = tr_[idx].second;
	if ( xpos < x1 + mDefEps )
	{
	    if ( mIsEqual(xpos,x1,mDefEps) )
		return y1;
	    x0 = tr_[idx-1].first; y0 = tr_[idx-1].second;
	    const float frac = (xpos-x0) / (x1-x0);
	    const float val = frac * y1 + (1-frac) * y0;
	    return mRounded( CompType, val );
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

    for ( idx_type idx=0; idx<tr_.size(); idx++ )
	if ( tr_[idx].second > 0 ) return true;

    return false;
}


ColTab::Sequence::idx_type ColTab::Sequence::setColor( PosType px,
				 CompType pr, CompType pg, CompType pb )
{
    mLock4Write();

    if ( px > 1 ) px = 1; if ( px < 0 ) px = 0;
    const size_type sz = gtSize();

    idx_type chgdidx = -1;
    bool done = false;
    for ( idx_type idx=0; idx<sz; idx++ )
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
	{ r_ += pr; g_ += pg; b_ += pb; x_ += px; chgdidx = gtSize()-1; }

    mSendChgNotif( cColorChange(), chgdidx );
    return chgdidx;
}


void ColTab::Sequence::changeColor( idx_type idx,
				 CompType pr, CompType pg, CompType pb )
{
    mLock4Write();
    if ( chgColor(idx,pr,pg,pb) )
	mSendChgNotif( cColorChange(), idx );
}


bool ColTab::Sequence::chgColor( idx_type idx,
				 CompType pr, CompType pg, CompType pb )
{
    if ( !x_.validIdx(idx) )
	return false;

    r_[idx] = pr; g_[idx] = pg; b_[idx] = pb;
    return true;
}


#define mEps 0.00001

void ColTab::Sequence::changePos( idx_type idx, PosType x )
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


void ColTab::Sequence::removeColor( idx_type idx )
{
    mLock4Write();
    if ( rmColor(idx) )
	mSendChgNotif( cColorChange(), idx );
}


bool ColTab::Sequence::rmColor( idx_type idx )
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
						idx_type idx ) const
{
    mLock4Read();
    return tr_.validIdx(idx) ? tr_[idx] : TranspPtType( 0.f, 0 );
}


ColTab::Sequence::CompType ColTab::Sequence::transparencyAt( PosType px ) const
{
    mLock4Read();
    return gtTransparencyAt( px );
}


void ColTab::Sequence::setTransparency( TranspPtType pt )
{
    if ( pt.first < 0 ) pt.first = 0.f;
    if ( pt.first > 1 ) pt.first = 1.f;
    if ( pt.second < 0 ) pt.second = 0;
    if ( pt.second > 255 ) pt.second = 255;

    mLock4Write();
    bool done = false; idx_type chgidx = -1;
    for ( idx_type idx=0; idx<tr_.size(); idx++ )
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


void ColTab::Sequence::removeTransparencyAt( idx_type idx )
{
    mLock4Write();
    if ( tr_.validIdx(idx) )
    {
	tr_.removeSingle( idx );
	mSendChgNotif( cTransparencyChange(), idx );
    }
}


void ColTab::Sequence::changeTransparency( idx_type idx, TranspPtType pt )
{
    mLock4Write();
    if ( !tr_.validIdx(idx) )
	return;

    tr_[idx] = pt;
    mSendChgNotif( cTransparencyChange(), idx );
}


void ColTab::Sequence::emitStatusChg() const
{
    mLock4Write();
    mSendChgNotif( cStatusChange(), cUnspecChgID() );
}


uiString ColTab::Sequence::statusDispStr() const
{
    return ::toUiString( "%1%2" ).arg( toUiString(status()) )
	.arg( disabled() ? od_static_tr("ColTab::Sequence::statusDispStr",
				"[disabled]") : uiString::empty() );
}


ColTab::PosType ColTab::Sequence::snapToSegmentCenter( PosType x ) const
{
    mLock4Read();
    if ( mIsUdf(x) )
	return x;
    else if ( nrsegments_ < 1 )
	return x;
    else if ( nrsegments_ == 1 )
	return 0.5f;
    else if ( x < 0.f )
	{ pErrMsg("0<=x<=1"); x = 0.f; }
    else if ( x > 1.f )
	{ pErrMsg("0<=x<=1"); x = 1.f; }

    const float segmentsize = 1.0f / (nrsegments_ - 1);

    idx_type segment = (idx_type) ( x/segmentsize + 0.5f );
    if ( segment<0 )
	segment = 0;
    if ( segment>=nrsegments_ )
	segment = nrsegments_-1;

    return segment*segmentsize;
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
    iopar.setYN( sKeyDisabled(), disabled_ );

    for ( int idx=0; idx<gtSize(); idx++ )
    {
	fms.setEmpty();
	fms += x_[idx];
	fms += (int)r_[idx]; fms += (int)g_[idx]; fms += (int)b_[idx];
	fms += gtTransparencyAt( x_[idx] );
	BufferString str( sKeyValCol() );
	str += "."; str += idx;
	iopar.set( str, fms );
    }

    for ( idx_type idx=0; idx<tr_.size(); idx++ )
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
    iopar.getYN( sKeyDisabled(), disabled_ );

    nrsegments_ = 0;
    const bool hadnrsegments = iopar.get( sKeyNrSegments(), nrsegments_ );

    x_.erase(); r_.erase(); g_.erase(); b_.erase(); tr_.erase();
    for ( idx_type idx=0; ; idx++ )
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

    for ( idx_type idx=0; ; idx++ )
    {
	BufferString key( sKeyTransparency() );
	key += "."; key += idx;
	TranspPtType pt; float floatval;
	if ( !iopar.get( key, pt.first, floatval ) )
	    break;

	pt.second = mRounded( CompType, floatval );
	tr_ += pt;
    }

    //OD 3.2 and before
    if ( !hadnrsegments && gtSize() > 3 )
    {
	bool found = false;
	for ( idx_type idx=1; idx<gtSize()-2; idx+=2 )
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



static Threads::AtomicPointer<ColTab::SequenceManager> the_coltab_seqmgr_;

namespace ColTab
{

const SequenceManager& SeqMGR()
{
    if ( !the_coltab_seqmgr_ )
	the_coltab_seqmgr_.setIfEqual( 0, new SequenceManager );

    return *the_coltab_seqmgr_;
}

}


ColTab::SequenceManager::SequenceManager()
    : nameChange(this)
    , iscopy_(false)
{
    IOPar* iop = 0;
    BufferString fnm = mGetSetupFileName("ColTabs");
    od_istream strm( fnm );
    if ( strm.isOK() )
    {
	ascistream astrm( strm );
	iop = new IOPar( astrm );
    }
    if ( iop )
	{ addFromPar( *iop, true ); delete iop; }

    if ( !OD::InSysAdmRunContext() )
    {
	Settings& setts( Settings::fetch(sKeyCtabSettsKey) );
	addFromPar( setts, false );

	BufferStringSet removedseqnms;
	PtrMan<IOPar> remiop = setts.subselect( sKeyRemoved() );
	if ( remiop && !remiop->isEmpty() )
	{
	    for ( int idx=0; idx<remiop->size(); idx++ )
		removedseqnms.add( remiop->getValue(idx) );
	}

	for ( int idx=0; idx<sysseqs_.size(); idx++ )
	{
	    const Sequence* seq = sysseqs_[idx];
	    if ( removedseqnms.isPresent(seq->name()) )
		continue;

	    if ( gtIdxOf(seqs_,seq->name()) < 0
		&& !removedseqnms.isPresent(seq->name()) )
		doAdd( seq->clone(), false );
	}
    }
    lastsaveddirtycount_ = dirtyCount();
}


ColTab::SequenceManager::SequenceManager( const SequenceManager& oth )
    : Monitorable(oth)
    , iscopy_(true)
    , nameChange(this)
{
    copyClassData( oth );
    mTriggerInstanceCreatedNotifier();
}


ColTab::SequenceManager::~SequenceManager()
{
    sendDelNotif();
    deepUnRef( seqs_ );
    deepUnRef( sysseqs_ );
}


void ColTab::SequenceManager::deleteInst( SequenceManager* inst )
{
    delete inst;
}


mImplMonitorableAssignment( ColTab::SequenceManager, Monitorable )


void ColTab::SequenceManager::copyClassData( const SequenceManager& oth )
{
    deepUnRef( seqs_ );
    deepUnRef( sysseqs_ );

    for ( int othidx=0; othidx<oth.seqs_.size(); othidx++ )
	doAdd( oth.seqs_[othidx]->clone(), false );

    sysseqs_ = oth.sysseqs_;
    deepRef( sysseqs_ );
    lastsaveddirtycount_ = oth.lastsaveddirtycount_;
}


Monitorable::ChangeType ColTab::SequenceManager::compareClassData(
					const SequenceManager& oth ) const
{
    if ( seqs_.size() != oth.seqs_.size() )
	return cEntireObjectChange();
    for ( int idx=0; idx<seqs_.size(); idx++ )
	if ( *seqs_[idx] != *oth.seqs_[idx] )
	    return cEntireObjectChange();
    return cNoChange();
}


void ColTab::SequenceManager::rollbackFromCopy( const SequenceManager& oth )
{
    if ( iscopy_ || !oth.iscopy_ )
	{ pErrMsg("Knurft"); return; }

    rollbackFrom( oth );
}


void ColTab::SequenceManager::rollbackFrom( const SequenceManager& oth )
{
    // We cannot lock.
    // There will be callbacks and the mgr cannot be in a locked state
    // Not really a problem as rollback is a 'are you sure?' user thing.
    // We will try to make as few changes as possible.

    // 1. remove ones that are not in oth
    for ( int myidx=0; myidx<seqs_.size(); myidx++ )
    {
	Sequence* myseq = seqs_[myidx];
	if ( oth.idxOf(myseq->name()) < 0 )
	{
	    seqs_.removeSingle( myidx );
	    myseq->unRef();
	    myidx--;
	}
    }

    // 2. add or copy the rest
    for ( int othidx=0; othidx<oth.seqs_.size(); othidx++ )
    {
	const Sequence& othseq = *oth.seqs_[othidx];
	const int myidx = idxOf( othseq.name() );
	if ( myidx < 0 )
	    doAdd( othseq.clone(), false );
	else if ( *seqs_[myidx] != othseq )
	    *seqs_[myidx] = othseq;
    }

    // 3. update to latest sys sequences
    deepUnRef( sysseqs_ );
    sysseqs_ = oth.sysseqs_;
    deepRef( sysseqs_ );

    // 4. copy the rest
    lastsaveddirtycount_ = oth.lastsaveddirtycount_;
    Monitorable::operator =( oth );
}


bool ColTab::SequenceManager::isPresent( const char* nm ) const
{
    mLock4Read();
    return idxOf( nm ) < 0;
}


ColTab::Sequence::Status ColTab::SequenceManager::statusOf(
						const Sequence& seq ) const
{
    // can't lock because Sequence will call this function
    // should be OK as the sysseqs_ should be constant during normal use

    const idx_type sysidx = gtIdxOf( sysseqs_, seq.name() );

    return sysidx < 0			? Sequence::Added
	 : (seq == *sysseqs_[sysidx]	? Sequence::System
					: Sequence::Edited);
}


ColTab::SequenceManager::ConstRef ColTab::SequenceManager::getByName(
						const char* nm ) const
{
    mLock4Read();
    const idx_type idx = idxOf( nm );
    return ConstRef( idx < 0 ? 0 : seqs_[idx] );
}


ColTab::SequenceManager::ConstRef ColTab::SequenceManager::getSystemSeq(
						const char* nm ) const
{
    mLock4Read();
    const idx_type idx = gtIdxOf( sysseqs_, nm );
    return ConstRef( idx < 0 ? 0 : sysseqs_[idx] );
}


RefMan<ColTab::Sequence> ColTab::SequenceManager::get4Edit(
						const char* nm ) const
{
    mLock4Read();
    const idx_type idx = idxOf( nm );
    ColTab::Sequence* ret = 0;
    if ( idx >= 0 )
	ret = const_cast<Sequence*>( seqs_[idx] );
    return ret;
}


ColTab::SequenceManager::size_type ColTab::SequenceManager::size() const
{
    mLock4Read();
    return seqs_.size();
}


ColTab::SequenceManager::idx_type ColTab::SequenceManager::indexOf(
						const char* nm ) const
{
    mLock4Read();
    return idxOf( nm );
}


ColTab::SequenceManager::idx_type ColTab::SequenceManager::indexOf(
						const Sequence& seq ) const
{
    mLock4Read();
    for ( idx_type idx=0; idx<seqs_.size(); idx++ )
	if ( seqs_[idx] == &seq )
	    return idx;
    return -1;
}


ColTab::SequenceManager::ConstRef ColTab::SequenceManager::getByIdx(
						idx_type idx ) const
{
    mLock4Read();
    return ConstRef( idx < 0 ? 0 : seqs_[idx] );
}


ColTab::SequenceManager::ConstRef ColTab::SequenceManager::getAny(
					const char* nm, bool forseis ) const
{
    mLock4Read();
    return gtAny( nm, forseis );
}


ColTab::SequenceManager::ConstRef ColTab::SequenceManager::getDefault(
							bool forseis ) const
{
    mLock4Read();
    return gtAny( 0, forseis );
}


const ColTab::Sequence* ColTab::SequenceManager::gtAny( const char* nm,
							bool forseis ) const
{
    if ( seqs_.isEmpty() )
    {
	ColTab::Sequence* cs = new ColTab::Sequence;
	cs->setColor( 0.f, 0, 0, 0 );
	cs->setColor( 1.f, 255, 255, 255 );
	cs->setMarkColor( Color::DgbColor() );
	cs->setUndefColor( Color(255,255,0) );
	cs->setName( "[Fallback]" );
	return cs;
    }

    idx_type idx = 0;
    if ( nm && *nm )
    {
	idx = idxOf( nm );
	if ( idx >= 0 )
	    return seqs_[idx];
    }

    idx = idxOf( Sequence::sDefaultName(forseis) );
    if ( idx < 0 )
	idx = 0;

    return seqs_[idx];
}


ColTab::SequenceManager::ConstRef ColTab::SequenceManager::getFromPar(
				const IOPar& iop, const char* subky ) const
{
    mLock4Read();

    ConstRef ret; BufferString seqname;
    if ( iop.get(IOPar::compKey(subky,sKey::Name()),seqname) )
    {
	idx_type idx = idxOf( seqname );
	if ( idx >= 0 )
	    ret = seqs_[idx];
    }
    if ( !ret )
    {
	ret = gtAny( 0, true );
	ColTab::Sequence* specseq = ret->clone();
	if ( !subky || !*subky )
	    specseq->usePar( iop );
	else
	{
	    PtrMan<IOPar> subpar = iop.subselect( subky );
	    specseq->usePar( *subpar );
	}
	ret = specseq;
    }

    return ret;
}


void ColTab::SequenceManager::getSequenceNames( BufferStringSet& nms ) const
{
    mLock4Read();
    for ( int idx=0; idx<seqs_.size(); idx++ )
	nms.add( seqs_[idx]->name() );
}


#define mSeqSet(issys) (issys ? sysseqs_ : seqs_)


void ColTab::SequenceManager::addFromPar( const IOPar& iop, bool issys )
{
    TypeSet<int> idxs; iop.collectIDs( idxs );
    for ( int idx=0; idx<idxs.size(); idx++ )
    {
	PtrMan<IOPar> ctiopar = iop.subselect( idxs[idx] );
	if ( !ctiopar || ctiopar->isEmpty() )
	    continue;

	ColTab::Sequence* newseq = new ColTab::Sequence;
	if ( !newseq->usePar(*ctiopar) || newseq->size() < 1 )
	    { newseq->unRef(); continue; }

	idx_type existidx = gtIdxOf( mSeqSet(issys), newseq->name() );
	if ( existidx < 0 )
	    doAdd( newseq, issys );
	else
	{
	    *mSeqSet(issys)[existidx] = *newseq;
	    newseq->unRef();
	}
    }
}


ColTab::SequenceManager::idx_type ColTab::SequenceManager::idxOf(
						const char* nm ) const
{
    return gtIdxOf( seqs_, nm );
}


ColTab::SequenceManager::idx_type ColTab::SequenceManager::gtIdxOf(
			    const ObjectSet<Sequence>& seqs, const char* nm )
{
    for ( idx_type idx=0; idx<seqs.size(); idx++ )
	if ( seqs[idx]->name().isEqual(nm,CaseInsensitive) )
	    return idx;
    return -1;
}


void ColTab::SequenceManager::doAdd( Sequence* seq, bool issys )
{
    seq->ref();
    mSeqSet(issys) += seq;
    if ( !issys && !iscopy_ )
	mAttachCB( seq->objectChanged(), SequenceManager::seqChgCB );
}


void ColTab::SequenceManager::seqChgCB( CallBacker* cb )
{
    mGetMonitoredChgData( cb, chgdata );
    if ( chgdata.isNoChange() )
	return;

    if ( chgdata.changeType() == NamedMonitorable::cNameChange() )
	nameChange.trigger( chgdata );

    touch();
}


void ColTab::SequenceManager::add( Sequence* seq )
{
    if ( !seq )
	return;

    mLock4Read();
    if ( idxOf(seq->name()) >= 0 )
	return;
    if ( !mLock2Write() && idxOf(seq->name()) >= 0 )
	return;

    doAdd( seq, false );
    mSendChgNotif( cSeqAdd(), seqs_.size()-1 );
}


void ColTab::SequenceManager::removeByName( const char* nm )
{
    mLock4Read();

    int idx = idxOf( nm );
    if ( !seqs_.validIdx(idx) )
	return;

    mSendChgNotif( cSeqRemove(), idx );
    mReLock();

    idx = idxOf( nm );
    if ( !mLock2Write() && !seqs_.validIdx(idx) )
	return;

    Sequence* rmseq = seqs_.removeSingle( idx );
    rmseq->unRef();
}


bool ColTab::SequenceManager::needsSave() const
{
    return dirtyCount() != lastsaveddirtycount_;
}


uiRetVal ColTab::SequenceManager::write( bool sys, bool applsetup ) const
{
    uiRetVal uirv;
    mLock4Read();

    if ( !sys )
    {
	Settings& setts( Settings::fetch(sKeyCtabSettsKey) );
	setts.setEmpty();
	idx_type newidx = 1;
	for ( idx_type idx=0; idx<sysseqs_.size(); idx++ )
	{
	    const ColTab::Sequence& seq = *sysseqs_[idx];
	    if ( gtIdxOf(seqs_,seq.name()) < 0 )
	    {
		setts.set( IOPar::compKey(sKeyRemoved(),newidx), seq.name() );
		newidx++;
	    }
	}
	newidx = 1;
	for ( idx_type idx=0; idx<seqs_.size(); idx++ )
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
	if ( !setts.write(false) )
	    uirv.set( tr("Cannot write user defined color tables") );
	return uirv;
    }

    const BufferString fnm( applsetup
	    ? GetSetupDataFileName(ODSetupLoc_ApplSetupOnly,"ColTabs",0)
	    : GetSetupDataFileName(ODSetupLoc_SWDirOnly,"ColTabs",0) );
    if ( File::exists(fnm) && !File::isWritable(fnm)
		&& !File::makeWritable(fnm,true,false) )
    {
	uirv.set( tr("Cannot make:\n%1\nnwritable").arg(fnm) );
	return uirv;
    }

    od_ostream strm( fnm );
    if ( !strm.isOK() )
    {
	uirv.set( tr("Cannot open:\n%1\nnfor write").arg(fnm) );
	strm.addErrMsgTo( uirv );
	return uirv;
    }

    ascostream astrm( strm );
    astrm.putHeader( "Color table definitions" );
    IOPar iopar;
    idx_type newidx = 1;
    for ( idx_type idx=0; idx<seqs_.size(); idx++ )
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
    if ( !strm.isOK() )
	uirv.set( tr("Error during write of color table specifics") );
    else
	lastsaveddirtycount_ = dirtyCount();

    return uirv;
}


const ColTab::Sequence& ColTab::SequenceManager::getRGBBlendColSeq( int nr )
{
    typedef ColTab::Sequence* pColTab_Sequence;
    mDefineStaticLocalObject( pColTab_Sequence*, ctabs, = 0);
    if ( !ctabs )
    {
	ctabs = new pColTab_Sequence[4];
	for ( int idx=0; idx<4; idx++ )
	{
	    ColTab::Sequence* seq = new ColTab::Sequence;
	    seq->setColor( 0.f, 0, 0, 0 );
	    ctabs[idx] = seq;
	}
#	define mSetEndCol(nr,r,g,b) ctabs[nr]->setColor( 1.f, r, g, b )
	mSetEndCol( 0, 255, 0, 0 );
	mSetEndCol( 1, 0, 255, 0 );
	mSetEndCol( 2, 0, 0, 255 );
	mSetEndCol( 3, 255, 255, 255 );
#	define mSetName(nr,nm) ctabs[nr]->setName( nm )
	mSetName( 0, "Red" );
	mSetName( 1, "Green" );
	mSetName( 2, "Blue" );
	mSetName( 3, "Transparency" );
	ctabs[3]->setTransparency( ColTab::Sequence::TranspPtType(0.f,0) );
	ctabs[3]->setTransparency( ColTab::Sequence::TranspPtType(1.f,255) );
    }

    if ( nr < 0 )
	{ pFreeFnErrMsg("RGB col tab nr too low"); nr = 0; }
    if ( nr > 3 )
	{ pFreeFnErrMsg("RGB col tab nr too high"); nr = 3; }

    return *(ctabs[nr]);
}
