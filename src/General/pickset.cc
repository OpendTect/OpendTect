/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "pickset.h"
#include "picksetmgr.h"

#include "datapointset.h"
#include "filepath.h"
#include "ioman.h"
#include "ioobj.h"
#include "iopar.h"
#include "od_iostream.h"
#include "oddirs.h"
#include "polygon.h"
#include "posimpexppars.h"
#include "settings.h"
#include "survinfo.h"
#include "tabledef.h"
#include "unitofmeasure.h"

#include <ctype.h>
#include <iostream>

static const char*		sKeyStartIdx()	{ return "Start index"; }
static const char*		sKeyNrPts()	{ return "Nr points"; }
static OD::Color		defcolor()	{ return OD::Color::Red(); }
static int			defPixSz()	{ return 3; }
static MarkerStyle3D::Type	defMarkerStyle(){ return MarkerStyle3D::Sphere;}
static OD::LineStyle		defLineStyle()
{
    return OD::LineStyle( OD::LineStyle::Solid, defPixSz(), defcolor() );
}

static MarkerStyle2D		defMarkerStyle2D()
{
    return MarkerStyle2D( MarkerStyle2D::Circle, defPixSz(), defcolor() );
}

static MarkerStyle3D		defMarkerStyle3D()
{
    return MarkerStyle3D( defMarkerStyle(), defPixSz(), defcolor() );
}


int Pick::Set::getSizeThreshold()
{
    int thresholdval = 1000;
    Settings::common().get( sKeyThresholdSize(), thresholdval );
    return thresholdval;
}

namespace Pick
{

// Pick::SetMgr
static ObjectSet<SetMgr>& setMgrs()
{
    mDefineStaticLocalObject( PtrMan<ObjectSet<SetMgr> >, mgrs, = 0 );
    if ( !mgrs )
    {
	mgrs = new ObjectSet<SetMgr>;
	mgrs->add( new SetMgr(nullptr) );
    }

    return *mgrs;
}


SetMgr& SetMgr::getMgr( const char* nm )
{
    ObjectSet<SetMgr>& mgrs = setMgrs();
    const StringView nmstr = nm;
    if ( nmstr.isEmpty() )
	return *mgrs[0];

    for ( int idx=1; idx<mgrs.size(); idx++ )
    {
	if ( mgrs[idx]->name() == nm )
	    return *mgrs[idx];
    }


    SetMgr* newmgr =  new SetMgr( nm );
    mgrs += newmgr;
    return *newmgr;
}


SetMgr::SetMgr( const char* nm )
    : NamedCallBacker(nm)
    , locationChanged(this)
    , setToBeRemoved(this)
    , setAdded(this)
    , setChanged(this)
    , setDispChanged(this)
    , bulkLocationChanged(this)
    , undo_( *new Undo() )
{
    mAttachCB( IOM().entryRemoved, SetMgr::objRm );
    mAttachCB( IOM().entriesRemoved, SetMgr::objectsRm );
    mAttachCB( IOM().surveyToBeChanged, SetMgr::survChg );
    mAttachCB( IOM().applicationClosing, SetMgr::survChg );
}


SetMgr::~SetMgr()
{
    detachAllNotifiers();
    undo_.removeAll();
    delete &undo_;
}


void SetMgr::add( const MultiID& ky, Set* st )
{
// st is never null here
    pss_ += st;
    ids_ += ky;
    changed_ += false;
    setAdded.trigger( st );
}


void SetMgr::set( const MultiID& ky, Set* newset )
{
    RefMan<Set> oldset = find( ky );
    if ( !oldset )
    {
	if ( newset )
	    add( ky, newset );
    }
    else if ( newset != oldset.ptr() )
    {
	const int idx = pss_.indexOf( oldset.ptr() );
	//Must be removed from list before trigger, otherwise
	//other users may remove it in calls invoked by the cb.
	pss_.removeSingle( idx );
	setToBeRemoved.trigger( oldset.ptr() );
	ids_.removeSingle( idx );
	changed_.removeSingle( idx );
	if ( newset )
	    add( ky, newset );
    }
}


const Undo& SetMgr::undo() const	{ return undo_; }
Undo& SetMgr::undo()			{ return undo_; }


const MultiID& SetMgr::id( int idx ) const
{
    return ids_.validIdx(idx) ? ids_[idx] : MultiID::udf();
}


void SetMgr::setID( int idx, const MultiID& mid )
{
    ids_[idx] = mid;
}


int SetMgr::indexOf( const Set& st ) const
{
    return pss_.indexOf( &st );
}


int SetMgr::indexOf( const MultiID& ky ) const
{
    for ( int idx=0; idx<size(); idx++ )
    {
	if ( ky == ids_[idx] )
	    return idx;
    }
    return -1;
}


int SetMgr::indexOf( const char* nm ) const
{
    for ( int idx=0; idx<size(); idx++ )
    {
	if ( pss_[idx]->name() == nm )
	    return idx;
    }

    return -1;
}


RefMan<Set> SetMgr::find( const MultiID& ky )
{
    const int idx = indexOf( ky );
    return pss_.validIdx(idx) ? pss_[idx] : nullptr;
}


ConstRefMan<Set> SetMgr::find( const MultiID& ky ) const
{
    const int idx = indexOf( ky );
    return pss_.validIdx(idx) ? pss_[idx] : nullptr;
}


MultiID* SetMgr::find( const Set& st ) const
{
    const int idx = indexOf( st );
    return idx < 0 ? const_cast<MultiID*>(&MultiID::udf()) :
				    const_cast<MultiID*>(&ids_[idx]);
}


RefMan<Set> SetMgr::find( const char* nm )
{
    const int idx = indexOf( nm );
    return pss_.validIdx(idx) ? pss_[idx] : nullptr;
}


ConstRefMan<Set> SetMgr::find( const char* nm ) const
{
    const int idx = indexOf( nm );
    return pss_.validIdx(idx) ? pss_[idx] : nullptr;
}


bool SetMgr::isChanged( int idx ) const
{
    return changed_.validIdx(idx) ? (bool) changed_[idx] : false;
}


void SetMgr::setUnChanged( int idx, bool yn )
{
    if ( changed_.validIdx(idx) )
	changed_[idx] = !yn;
}


void SetMgr::reportChange( CallBacker* sender, const ChangeData& cd )
{
    const int setidx = pss_.indexOf( cd.set_ );
    if ( setidx >= 0 )
    {
	changed_[setidx] = true;
	mDynamicCastGet(Pick::SetMgr*,picksetmgr,sender);
	Notifier<Pick::SetMgr> notif( picksetmgr ? picksetmgr->locationChanged
						 : nullptr );
	NotifyStopper ns( notif, this );
	locationChanged.trigger( const_cast<ChangeData*>( &cd ) );
    }
}


void SetMgr::reportChange( CallBacker* sender, const Set& s )
{
    const int setidx = pss_.indexOf( &s );
    if ( setidx >= 0 )
    {
	changed_[setidx] = true;
	mDynamicCastGet(Pick::SetMgr*,picksetmgr,sender);
	Notifier<Pick::SetMgr> notif( picksetmgr ? picksetmgr->setChanged
						 : nullptr );
	NotifyStopper ns( notif, this );
	setChanged.trigger( const_cast<Set*>(&s) );
    }
}


void SetMgr::reportDispChange( CallBacker* sender, const Set& s )
{
    const int setidx = pss_.indexOf( &s );
    if ( setidx >= 0 )
    {
	mDynamicCastGet(Pick::SetMgr*,picksetmgr,sender);
	Notifier<Pick::SetMgr> notif( picksetmgr ? picksetmgr->setDispChanged
						 : nullptr );
	NotifyStopper ns( notif, this );
	setDispChanged.trigger( const_cast<Set*>(&s) );
    }
}


void SetMgr::reportBulkChange( CallBacker* sender, const BulkChangeData& cd )
{
    const int setidx = pss_.indexOf( cd.set_ );
    if ( setidx >= 0 )
    {
	changed_[setidx] = true;
	mDynamicCastGet(Pick::SetMgr*,picksetmgr,sender);
	Notifier<Pick::SetMgr> notif( picksetmgr ?
				picksetmgr->bulkLocationChanged : nullptr );
	NotifyStopper ns( notif, this );
	bulkLocationChanged.trigger( const_cast<BulkChangeData*>(&cd) );
    }
}


void SetMgr::removeCBs( CallBacker* cb )
{
    locationChanged.removeWith( cb );
    bulkLocationChanged.removeWith( cb );
    setToBeRemoved.removeWith( cb );
    setAdded.removeWith( cb );
    setChanged.removeWith( cb );
    setDispChanged.removeWith( cb );
}


void SetMgr::removeAll()
{
    for ( int idx=pss_.size()-1; idx>=0; idx-- )
    {
	Set* pset = pss_.removeSingle( idx );
	setToBeRemoved.trigger( pset );
    }
}


void SetMgr::survChg( CallBacker* )
{
    removeAll();
    locationChanged.cbs_.erase();
    bulkLocationChanged.cbs_.erase();
    setToBeRemoved.cbs_.erase();
    setAdded.cbs_.erase();
    setChanged.cbs_.erase();
    setDispChanged.cbs_.erase();
    ids_.erase();
    changed_.erase();
}


void SetMgr::objRm( CallBacker* cb )
{
    if ( !cb || !cb->isCapsule() )
	return;

    mCBCapsuleUnpack( const MultiID&, ky, cb );
    if ( indexOf(ky) >= 0 )
	set( ky, nullptr );
}


void SetMgr::objectsRm( CallBacker* cb )
{
    if ( !cb || !cb->isCapsule() )
	return;

    mCBCapsuleUnpack( const TypeSet<MultiID>&, keys, cb );
    if ( keys.isEmpty() )
	return;

    for ( const auto& ky : keys )
    {
	if ( indexOf(ky) >= 0 )
	    set( ky, nullptr );
    }
}


BufferString SetMgr::getDispFileName( const MultiID& mid )
{
    PtrMan<IOObj> ioobj = IOM().get( mid );
    if ( !ioobj)
	return BufferString::empty();

    FilePath fp( ioobj->fullUserExpr(true) );
    fp.setExtension( "disp" );
    if ( fp.nrLevels() <= 1 )
    {
	const IOObjContext::StdDirData* dirdata =
			IOObjContext::getStdDirData( IOObjContext::Loc );
	BufferString dirpath;
	if ( dirdata )
	    dirpath = dirdata->dirnm_;

	const FilePath locfp( GetDataDir(), dirpath );
	fp.setPath( locfp.fullPath() );
    }

    return fp.fullPath();
}


static const char* sKeyDispPars()	{ return "Display Parameters"; }


bool SetMgr::readDisplayPars( const MultiID& mid, IOPar& par ) const
{
    const BufferString fnm = getDispFileName( mid );
    return par.read( fnm, sKeyDispPars() );
}


bool SetMgr::writeDisplayPars( const MultiID& mid, const IOPar& par ) const
{
    IOPar curpar;
    readDisplayPars( mid, curpar );
    curpar.merge( par );
    const BufferString fnm = getDispFileName( mid );
    return curpar.write( fnm, sKeyDispPars() );
}


void SetMgr::dumpMgrInfo( StringPairSet& infoset )
{
    const ObjectSet<SetMgr> mgrs = setMgrs();
    infoset.add( "Nr of Managers", mgrs.size() );
    for ( int idx=0; idx<mgrs.size(); idx++ )
    {
	const SetMgr* setmgr = mgrs[idx];
	infoset.add( IOPar::compKey(toString(idx+1),"Manager Name"),
		     setmgr->name() );
	const int nrsets = setmgr->size();
	infoset.add( IOPar::compKey(toString(idx+1),"Nr sets"), nrsets );

	for ( int ids=0; ids<nrsets; ids++ )
	{
	    ConstRefMan<Set> ps = setmgr->get( ids );
	    BufferString setkey = IOPar::compKey( toString(idx+1), ids );
	    infoset.add( IOPar::compKey(setkey,"PointSet ID"),
			 setmgr->id(ids).toString() );
	    infoset.add( IOPar::compKey(setkey,"PointSet Name"), ps->name() );
	    infoset.add( IOPar::compKey(setkey,"PointSet Size"), ps->size() );
	    infoset.add( IOPar::compKey(setkey,"PointSet References"),
		         ps->nrRefs()-1 );
	}
    }
}


class PickSetKnotUndoEvent: public UndoEvent
{
public:
    enum  UnDoType	    { Insert, PolygonClose, Remove, Move };
    PickSetKnotUndoEvent( UnDoType type, const MultiID& mid, int sidx,
			  const Pick::Location& pos )
    : type_(type)
    , newpos_(Pick::Location(Coord3::udf()))
    , pos_(Pick::Location(Coord3::udf()))
    , mid_(mid)
    , index_(sidx)
    {
	init( type, mid, sidx, pos );
    }


    const char* getStandardDesc() const override
    {
	if ( type_ == Insert )
	    return "Insert Knot";
	else if ( type_ == Remove )
	    return "Remove";
	else if ( type_ == Move )
	    return "move";

	return "";
    }


    bool unDo() override
    {
	Pick::SetMgr& mgr = Pick::Mgr();
	const int mididx = mgr.indexOf( mid_ );
	const bool haspickset = mididx >=0;
	RefMan<Pick::Set> set = haspickset ? mgr.get(mididx) : nullptr;
	if ( !set )
	    return false;

	const bool isclosed
	  = set->disp3d().polyDisp()->connect_==Pick::Set::Connection::Close;
	if ( isclosed && index_ == set->size()-1 && type_ != Move )
	    type_ = PolygonClose;

	Pick::SetMgr::ChangeData::Ev ev = type_ == Move
	    ? Pick::SetMgr::ChangeData::Changed
	    : ( type_ == Remove ? Pick::SetMgr::ChangeData::Added
				: Pick::SetMgr::ChangeData::ToBeRemoved );

	Pick::SetMgr::ChangeData cd( ev, set.ptr(), index_ );

	if ( type_ == Move )
	{
	    if ( set && set->size()>index_  && pos_.pos().isDefined() )
		set->set( index_, pos_ );
	}
	else if ( type_ == Remove )
	{
	    if ( pos_.pos().isDefined() )
		set->insert( index_, pos_ );
	}
	else if ( type_ == Insert  )
	{
	    set->remove( index_ );
	}
	else if ( type_ == PolygonClose && set->isPolygon() &&
					   set->disp3d().polyDisp() )
	{
	    set->disp3d().polyDisp()->connect_ = Pick::Set::Connection::Open;
	    set->remove(index_);
	}

	mgr.reportChange( 0, cd );

	return true;
    }


    bool reDo() override
    {
	Pick::SetMgr& mgr = Pick::Mgr();
	const int mididx = mgr.indexOf( mid_ );
	const bool haspickset = mididx >=0;
	RefMan<Pick::Set> set = haspickset ? mgr.get(mididx) : nullptr;
	if ( !set ) return false;

	Pick::SetMgr::ChangeData::Ev ev = type_== Move ?
	    Pick::SetMgr::ChangeData::Changed :
	    ( type_ == Remove
	    ? Pick::SetMgr::ChangeData::ToBeRemoved
	    : Pick::SetMgr::ChangeData::Added );

	Pick::SetMgr::ChangeData cd( ev, set.ptr(), index_ );

	if ( type_ == Move )
	{
	    if ( set && set->size()>index_ && newpos_.pos().isDefined() )
		set->set( index_, newpos_ );
	}
	else if ( type_ == Remove )
	{
	    set->remove( index_ );
	}
	else if ( type_ == Insert )
	{
	    if ( pos_.pos().isDefined() )
		set->insert( index_,pos_ );
	}
	else if ( type_ == PolygonClose && set->isPolygon() &&
					   set->disp3d().polyDisp() )
	{
	    if ( pos_.pos().isDefined() )
	    {
		set->disp3d().polyDisp()->connect_
						= Pick::Set::Connection::Close;
		set->insert( index_, pos_ );
	    }
	}

	mgr.reportChange( 0, cd );

	return true;
    }

protected:

    void init( UnDoType type, const MultiID& mid, int sidx,
						const Pick::Location& pos )
    {

	Pick::SetMgr& mgr = Pick::Mgr();
	const int mididx = mgr.indexOf( mid );
	const bool haspickset = mididx >=0;
	RefMan<Pick::Set> set = haspickset ? mgr.get(mididx) : nullptr;

	if ( type == Insert || type == PolygonClose )
	{
	    if ( set && set->size()>index_ )
		pos_ = set->get( index_ );
	}
	else if ( type == Remove )
	{
	    pos_ = pos;
	}
	else if ( type == Move )
	{
	    if ( set && set->size()>index_ )
		pos_ = set->get( index_ );
	    newpos_ = pos;
	}

    }


    Pick::Location  pos_;
    Pick::Location  newpos_;
    MultiID	    mid_;
    int		    index_;
    UnDoType	    type_;
};


class PickSetBulkUndoEvent: public UndoEvent
{
public:
    enum  UnDoType	    { Insert, Remove };
    PickSetBulkUndoEvent( UnDoType type, const MultiID& mid,
			  const TypeSet<int>& idxs,
			  const TypeSet<Pick::Location>& pos )
    { init( type, mid, idxs, pos ); }

    void init( UnDoType type, const MultiID& mid, const TypeSet<int>& idxs,
		const TypeSet<Pick::Location>& pos )
    {
	type_ = type;
	pos_ = pos;
	indexes_ = idxs;
	mid_ = mid;
    }


    const char* getStandardDesc() const override
    {
	if ( type_ == Insert )
	    return "Insert Bulk";
	else
	    return "Remove Bulk";
    }


    bool unDo() override
    {
	Pick::SetMgr& mgr = Pick::Mgr();
	const int mididx = mgr.indexOf( mid_ );
	const bool haspickset = mididx >=0;
	RefMan<Pick::Set> set = haspickset ? mgr.get(mididx) : nullptr;
	if ( !set ) return false;

	Pick::SetMgr::BulkChangeData::Ev ev = type_ == Remove
				? Pick::SetMgr::BulkChangeData::Added
				: Pick::SetMgr::BulkChangeData::ToBeRemoved;

	Pick::SetMgr::BulkChangeData cd( ev, set.ptr(), indexes_ );

	if ( type_ == Remove )
	{
	    for ( int idx=0; idx<indexes_.size(); idx++ )
	    {
		if ( pos_.validIdx(idx) && pos_[idx].pos().isDefined() )
		    set->insert( indexes_[idx], pos_[idx] );
	    }
	}
	else
	{
	    for ( int idx=indexes_.size()-1; idx>=0; idx-- )
		set->remove( indexes_[idx] );
	}

	mgr.reportBulkChange( 0, cd );
	return true;
    }


    bool reDo() override
    {
	Pick::SetMgr& mgr = Pick::Mgr();
	const int mididx = mgr.indexOf( mid_ );
	const bool haspickset = mididx >=0;
	RefMan<Pick::Set> set = haspickset ? mgr.get(mididx) : nullptr;
	if ( !set ) return false;

	Pick::SetMgr::BulkChangeData::Ev ev = type_ == Remove
				? Pick::SetMgr::BulkChangeData::ToBeRemoved
				: Pick::SetMgr::BulkChangeData::Added;

	Pick::SetMgr::BulkChangeData cd( ev, set.ptr(), indexes_ );

	if ( type_ == Remove )
	{
	    for ( int idx=indexes_.size()-1; idx>=0; idx-- )
		set->remove( indexes_[idx] );
	}
	else
	{
	    for ( int idx=0; idx<indexes_.size(); idx++ )
	    {
		if ( pos_.validIdx(idx) && pos_[idx].pos().isDefined() )
		    set->insert( indexes_[idx], pos_[idx] );
	    }
	}

	mgr.reportBulkChange( 0, cd );
	return true;
    }

protected:

    TypeSet<Pick::Location>	pos_;
    MultiID			mid_;
    TypeSet<int>		indexes_;
    UnDoType			type_;
};


// Both Pick set types

template <class PicksType>
static int findIdx( const PicksType& picks,
					  const TrcKey& tk )
{
    const int sz = picks.size();
    for ( int idx=0; idx<sz; idx++ )
	if ( picks.get(idx).trcKey() == tk )
	    return idx;
    return -1;
}

template <class PicksType>
static int getNearestLocation( const PicksType& ps,
				const Coord3& pos, bool ignorez )
{
    const int sz = ps.size();
    if ( sz < 2 )
	return sz - 1;
    if ( pos.isUdf() )
	return 0;

    int ret = 0;
    const Coord3& p0 = ps.get( ret ).pos();
    double minsqdist = p0.isUdf() ? mUdf(double)
		     : (ignorez ? pos.sqHorDistTo( p0 ) : pos.sqDistTo( p0 ));

    for ( int idx=1; idx<sz; idx++ )
    {
	const Coord3& curpos = ps.get( idx ).pos();
	if ( pos.isUdf() )
	    continue;

	const double sqdist = ignorez ? pos.sqHorDistTo( curpos )
				      : pos.sqDistTo( curpos );
	if ( sqdist == 0 )
	    return idx;
	else if ( sqdist < minsqdist )
	    { minsqdist = sqdist; ret = idx; }
    }
    return ret;
}


// Pick::Set
mDefineEnumUtils( Pick::Set, Connection, "Connection" )
{ "None", "Open", "Close", nullptr };

mStartAllowDeprecatedSection
Set::Disp::Disp( bool ispolygon )
    : ispolygon_(ispolygon)
{
    if ( ispolygon )
	polydisp_ = new PolyDisp;
}


Set::Disp::~Disp()
{
    delete polydisp_;
}


bool Set::Disp::operator==( const Disp& oth ) const
{
    if ( !ispolygon_ )
	return !oth.ispolygon_;

    if ( polydisp_ && oth.polydisp_ )
	return polydisp_->linestyle_ == oth.polydisp_->linestyle_ &&
	       polydisp_->connect_ == oth.polydisp_->connect_ &&
	       polydisp_->dofill_ == oth.polydisp_->dofill_ &&
	       polydisp_->fillcolor_ == oth.polydisp_->fillcolor_;

    return false;
}
mStopAllowDeprecatedSection


void Set::Disp::convertToPolygon()
{
    if ( ispolygon_ )
	return;

    ispolygon_ = true;
    polydisp_ = new PolyDisp;
}


void Set::Disp::convertToPointSet()
{
    if ( !ispolygon_ )
	return;

    ispolygon_ = false;
    delete polydisp_;
    polydisp_ = nullptr;
}


Set::PolyDisp* Set::Disp::polyDisp()
{
    if ( ispolygon_ && !polydisp_  )
    {
	pErrMsg( "PolyDisp object should have been valid. Please debug." );
	polydisp_ = new Set::PolyDisp;
    }

    if ( !ispolygon_ )
    {
	pErrMsg( "If it is a polygon, Pick::Set should have been "
	    "created as one" );
	if ( !polydisp_ )
	    polydisp_ = new Set::PolyDisp;
    }

    return polydisp_;
}


const Set::PolyDisp* Set::Disp::polyDisp() const
{
    return getNonConst(this)->polyDisp();
}


Set::Disp3D::Disp3D( bool ispolygon )
    : Disp( ispolygon )
{}


Set::Disp3D::~Disp3D()
{}


bool Set::Disp3D::operator=( const Disp3D& oth )
{
    markerstyle_ = oth.markerstyle_;
    ispolygon_ = oth.ispolygon_ && oth.polydisp_;
    if ( ispolygon_ && !polydisp_ )
	polydisp_ = new PolyDisp;

    if ( polydisp_ && oth.polydisp_ )
	*polydisp_ = *oth.polydisp_;

    return true;
}


bool Set::Disp3D::operator==( const Disp& oth ) const
{
    auto* oth3d = dCast( const Disp3D*, &oth );
    if ( !oth3d )
    {
	pErrMsg( "Please provide a Disp3D object." );
	return false;
    }

    bool ret = Set::Disp::operator==( *oth3d );
    return ret && markerstyle_==oth3d->markerstyle_;
}


int Set::Disp3D::type() const
{
    return markerstyle_.type_;
}


Set::Disp2D::Disp2D( bool ispolygon )
    : Disp( ispolygon )
{}


Set::Disp2D::~Disp2D()
{}


bool Set::Disp2D::operator=( const Disp2D& oth )
{
    markerstyle_ = oth.markerstyle_;
    ispolygon_ = oth.ispolygon_ && oth.polydisp_;
    if ( ispolygon_ && !polydisp_ )
	polydisp_ = new PolyDisp;

    if ( polydisp_ && oth.polydisp_ )
	*polydisp_ = *oth.polydisp_;

    return true;
}


bool Set::Disp2D::operator==( const Disp& oth ) const
{
    auto* oth2d = dCast( const Disp2D*, &oth );
    if ( !oth2d )
    {
	pErrMsg( "Please provide a Disp2D object." );
	return false;
    }

    bool ret = Set::Disp::operator==( *oth2d );
    return ret && markerstyle_==oth2d->markerstyle_;
}


int Set::Disp2D::type() const
{
    return markerstyle_.type_;
}


mStartAllowDeprecatedSection
Set::Set( const char* nm, bool ispolygon )
    : SharedObject(nm)
    , pars_(*new IOPar)
    , zdomaininfo_(&SI().zDomainInfo())
    , disp3d_( *new Disp3D(ispolygon) )
    , disp2d_( *new Disp2D(ispolygon) )
    , disp_(disp3d_)
{
    pars_.set( sKey::Type(), ispolygon ? sKey::Polygon() : sKey::PointSet() );
    setDefaultDispPars( ispolygon );
}


Set::Set( const Set& oth )
    : pars_(*new IOPar)
    , zdomaininfo_(&SI().zDomainInfo())
    , disp3d_( *new Disp3D(oth.isPolygon()) )
    , disp2d_( *new Disp2D(oth.isPolygon()) )
    , disp_(disp3d_)
{
    pars_.set( sKey::Type(), oth.isPolygon() ? sKey::Polygon()
					     : sKey::PointSet() );
    *this = oth;
}


Set::~Set()
{
    delete &pars_;
    delete &disp3d_;
    delete &disp2d_;
}
mStopAllowDeprecatedSection


Set& Set::operator=( const Set& oth )
{
    if ( &oth == this )
	return *this;

    if ( isPolygon() != oth.isPolygon() )
	pErrMsg( "Please avoid copying a pointset into a polygon and "
		 "vice-versa" );

    locations_.copy( oth.locations_ );
    setName( oth.name() );
    pars_ = oth.pars_;
    disp3d_ = oth.disp3d_;
    disp2d_ = oth.disp2d_;
    startidxs_ = oth.startidxs_;
    readonly_ = oth.readonly_;
    setZDomain( oth.zDomain() );

    return *this;
}


void Set::convertToPointSet()
{
    if ( !isPolygon() )
	return;

    pars_.set( sKey::Type(), sKey::PointSet() );
    disp2d_.convertToPointSet();
    disp3d_.convertToPointSet();
}


void Set::convertToPolygon()
{
    if ( isPolygon() )
	return;

    pars_.set( sKey::Type(), sKey::Polygon() );
    disp2d_.convertToPolygon();
    disp3d_.convertToPolygon();
    disp2d_.polyDisp()->fillcolor_ = defcolor();
    disp2d_.polyDisp()->linestyle_ = defLineStyle();
    if ( disp2d_.polyDisp()->connect_==Connection::None )
	disp2d_.polyDisp()->connect_ = Connection::Close;
}


int Set::add( const Location& loc )
{
    locations_.add( loc );
    return locations_.size()-1;
}


int Set::add( const Coord3& crd )
{
    return add( Location(crd) );
}


int Set::add( const Coord& crd,float z )
{
    return add( Location(crd,z) );
}


void Set::set( int idx, const Location& loc )
{
    if ( locations_.validIdx(idx) )
	locations_[idx] = loc;
}


void Set::setPos( int idx, const Coord& crd )
{
    locations_[idx].setPos( crd );
}


void Set::setPos( int idx, const Coord3& crd )
{
    locations_[idx].setPos( crd );
}


void Set::setZ( int idx, float z )
{
    if ( idx==-1 )
    {
	for ( auto& loc : locations_ )
	    loc.setZ( z );
	return;
    }

    locations_[idx].setZ( z );
}


const Coord3& Set::getPos( int idx ) const
{
    return locations_[idx].pos();
}


float Set::getZ( int idx ) const
{
    return locations_[idx].z();
}


void Set::setDir( int idx, const Sphere& sphere )
{
    locations_[idx].setDir( sphere );
}


void Set::setDip( int idx, float inldip, float crldip )
{
    locations_[idx].setDip( inldip, crldip );
}


void Set::setKeyedText( int idx, const char *key, const char *txt)
{
    locations_[idx].setKeyedText( key, txt );
}


void Set::insert( int idx, const Location& loc )
{
    locations_.insert( idx, loc );
}


void Set::remove( int idx )
{
    locations_.removeSingle( idx );
    for ( int setidx=0; setidx<startidxs_.size(); setidx++ )
    {
	if ( idx < startidxs_[setidx] )
	    startidxs_[setidx]--;
    }
}


void Set::setEmpty()
{
    locations_.setEmpty();
    startidxs_.setEmpty();
}


bool Set::validIdx( int idx ) const
{
    return locations_.validIdx( idx );
}


bool Set::setCapacity( int sz )
{
    return locations_.setCapacity( sz, false );
}


bool Set::setSize( int size, const Coord3& defval )
{
    return locations_.setSize( size, Pick::Location(defval) );
}


bool Set::append( const Pick::Set& oth )
{
    return locations_.append( oth.locations_ );
}


void Set::getLocations( TypeSet<Coord3>& crds, int setidx ) const
{
    int start, stop;
    getStartStopIdx( setidx, start, stop );
    for ( int idx=start; idx<=stop; idx++ )
	crds.add( get(idx).pos() );
}


OD::GeomSystem Set::geomSystem() const
{
    OD::GeomSystem gs;
    return pars_.get(sKey::SurveyID(),gs) ? gs : Pick::geomSystem( *this );
}


bool Set::is2D() const
{
    return Pick::is2D( *this );
}


bool Set::isPolygon() const
{
    BufferString typ = pars_.find( sKey::Type() );
    if ( typ.isEmpty() )
    {
	PtrMan<IOObj> obj = IOM().get( name(), "PickSet Group" );
	if ( obj )
	    obj->pars().get( sKey::Type(), typ );
    }

    return typ.isEqual( sKey::Polygon() );
}


const ZDomain::Info& Set::zDomain() const
{
    return *zdomaininfo_;
}


bool Set::zIsTime() const
{
    return zDomain().isTime();
}


bool Set::zInMeter() const
{
    return zDomain().isDepthMeter();
}


bool Set::zInFeet() const
{
    return zDomain().isDepthFeet();
}


const UnitOfMeasure* Set::zUnit() const
{
    return UnitOfMeasure::zUnit( zDomain() );
}


Set& Set::setZDomain( const ZDomain::Info& zinfo )
{
    if ( readonly_ || (!zinfo.isTime() && !zinfo.isDepth()) ||
	 &zinfo == &zDomain() )
	return *this;

    zdomaininfo_ = &zinfo;
    return *this;
}


void Set::getStartStopIdx( int setidx, int& start, int& stop ) const
{
    start = 0;
    stop = size()-1;
    if ( !startidxs_.validIdx(setidx) )
	return;

    start = startidxs_[setidx];
    stop = setidx==startidxs_.size()-1 ? size()-1 : startidxs_[setidx+1]-1;
}


void Set::addStartIdx( int locidx )
{
    startidxs_ += locidx;
}


void Set::setStartIdx( int setidx, int locidx )
{
    if ( startidxs_.validIdx(setidx) )
	startidxs_[setidx] = locidx;
}


int Set::nrSets() const
{
    return startidxs_.isEmpty() ? 1 : startidxs_.size();
}


void Set::findStartIdxs()
{
    startidxs_.erase();
    if ( isEmpty() )
	return;

    Location firstloc = locations_.first();
    for ( int idx=1; idx<locations_.size(); idx++ )
    {
	const Location& loc = locations_[idx];
	if ( firstloc.pos().coord() != loc.pos().coord() )
	    continue;

	idx++;
	if ( idx==locations_.size() )
	    break;

	if ( startidxs_.isEmpty() )
	    startidxs_ += 0;

	startidxs_ += idx;
	firstloc = locations_[idx];
    }
}


void Set::getPolygon( ODPolygon<double>& poly, int setidx ) const
{
    int start, stop;
    getStartStopIdx( setidx, start, stop );
    for ( int idx=start; idx<=stop; idx++ )
    {
	const Coord c( locations_.get(idx).pos() );
        poly.add( Geom::Point2D<double>( c.x_, c.y_ ) );
    }
}


float Set::getXYArea( int setidx ) const
{
    const bool isdisconnected = isPolygon() && disp3d().polyDisp() &&
			disp3d().polyDisp()->connect_==Set::Connection::None;
    if ( size()<3 || isdisconnected )
	return mUdf(float);

    ODPolygon<double> polygon;
    getPolygon( polygon, setidx );
    if ( polygon.isSelfIntersecting() )
	return mUdf(float);

    return float( polygon.area() );
}


int Set::find( const TrcKey& tk ) const
{
    return findIdx( *this, tk );
}


int Set::nearestLocation( const Coord& pos ) const
{
    return getNearestLocation( *this, Coord3(pos.x_,pos.y_,0.), true );
}


int Set::nearestLocation( const Coord3& pos,
						 bool ignorez ) const
{
    return getNearestLocation( *this, pos, ignorez );
}


void Set::getBoundingBox( TrcKeyZSampling& tkzs ) const
{
    if ( isEmpty() )
    {
	tkzs.init( true );
	return;
    }

    tkzs.setEmpty();
    tkzs.zsamp_.setUdf();
    for ( int idx=0; idx<size(); idx++ )
	tkzs.include( get(idx).trcKey().binID(), get(idx).z() );
}


void Set::fillPar( IOPar& par ) const
{
    par.set( sKeyStartIdx(), startidxs_ );
    zDomain().fillPar( par );
    par.set( sKeyNrPts(), size() );
    par.merge( pars_ );
}


bool Set::usePar( const IOPar& par )
{
    BufferString type;
    const bool parhastype = par.get( sKey::Type(),type) && !type.isEmpty();
    if ( parhastype )
    {
	const bool ispolygonpar = type==sKey::Polygon();
	if ( isPolygon() != ispolygonpar )
	    pErrMsg( "Please avoid using an IOPar meant for pointset in a "
		     "polygon and vice-versa" );
    }

    useDisplayPars( par );
    TypeSet<int> startidx;
    par.get( sKeyStartIdx(), startidx );
    startidxs_.setEmpty();
    if ( !startidx.isEmpty() )
	startidxs_ = startidx;

    const ZDomain::Info* zinfo = ZDomain::Info::getFrom( par );
    if ( zinfo )
	setZDomain( *zinfo );

    pars_ = par;
    pars_.removeWithKey( sKey::ZUnit() );
    pars_.removeWithKey( sKey::Color() );
    pars_.removeWithKey( sKey::Size() );
    pars_.removeWithKey( sKeyMarkerType() );
    pars_.removeWithKey( sKeyConnect() );
    pars_.removeWithKey( sKeyStartIdx() );
    pars_.removeWithKey( sKey::LineStyle() );
    pars_.removeWithKey( sKeyFillColor() );
    pars_.removeWithKey( sKeyFill() );
    pars_.removeSubSelection( "2D" );

    return true;
}


void Set::setDefaultDispPars( bool ispolygon )
{
    setDefaultDispPars2D( ispolygon );
    setDefaultDispPars3D( ispolygon );
}


void Set::setDefaultDispPars2D( bool ispolygon )
{
    disp2d_.markerstyle_ = defMarkerStyle2D();
    if ( !ispolygon )
	return;

    disp2d_.polyDisp()->fillcolor_ = defcolor();
    disp2d_.polyDisp()->linestyle_ = defLineStyle();
    if ( disp2d_.polyDisp()->connect_==Connection::None )
	disp2d_.polyDisp()->connect_ = Connection::Close;
}


void Set::setDefaultDispPars3D( bool ispolygon )
{
    disp3d_.markerstyle_ = defMarkerStyle3D();
    if ( !ispolygon )
	return;

    disp3d_.polyDisp()->fillcolor_ = defcolor();
    disp3d_.polyDisp()->linestyle_ = defLineStyle();
    if ( disp3d_.polyDisp()->connect_==Connection::None )
	disp3d_.polyDisp()->connect_ = Connection::Close;
}


void Set::fillDispPars( const Disp& disp, IOPar& par ) const
{
    BufferString colstr;
    if ( disp.color() != OD::Color::NoColor() )
    {
	disp.color().fill( colstr );
	par.set( sKey::Color(), colstr );
    }

    par.set( sKeyMarkerType(), disp.type() );
    par.set( sKey::Size(), disp.size() );

    if ( !isPolygon() )
	return;

    BufferString fillcolstr;
    if ( disp.polyDisp()->fillcolor_ != OD::Color::NoColor() )
    {
	disp.polyDisp()->fillcolor_.fill( fillcolstr );
	par.set( sKeyFillColor(), fillcolstr );
    }

    BufferString lsstr;
    disp.polyDisp()->linestyle_.toString( lsstr );
    par.set( sKey::LineStyle(), lsstr );
    par.set( sKeyConnect(), getConnectionString(disp.polyDisp()->connect_) );
    par.setYN( sKeyFill(), disp.polyDisp()->dofill_ );
}


void Set::fillDisplayPars( IOPar& par ) const
{
    fillDisplayPars3D( par );
    fillDisplayPars2D( par );
}


void Set::fillDisplayPars3D( IOPar& par ) const
{
    fillDispPars( disp3d_, par );
}


void Set::fillDisplayPars2D( IOPar& par ) const
{
    IOPar par2d;
    fillDispPars( disp2d_, par2d );
    par.mergeComp( par2d, "2D" );
}


bool Set::useDisplayPars3D( const IOPar& par, Disp3D& disp )
{
    BufferString colstr;
    const bool hascolor = par.get( sKey::Color(), colstr );
    if ( hascolor )
	disp.color().use( colstr.buf() );

    BufferString fillcolstr;
    if ( par.get(sKeyFillColor(),fillcolstr) )
	disp.polyDisp()->fillcolor_.use( fillcolstr.buf() );

    const bool hasdispsz = par.hasKey( sKey::Size() );
    int size = defPixSz();
    if ( hasdispsz )
	par.get( sKey::Size(), size );

    disp.markerstyle_.size_ = size;

    int markertype = 0;
    if ( par.hasKey(sKeyMarkerType()) )
	par.get( sKeyMarkerType(), markertype );

    disp.markerstyle_.type_ = (MarkerStyle3D::Type) markertype;

    if ( !isPolygon() )
	return true;

    BufferString lsstr;
    if ( par.get(sKey::LineStyle(),lsstr) )
	disp.polyDisp()->linestyle_.fromString( lsstr );
    else if ( hasdispsz && hascolor )
	disp.polyDisp()->linestyle_ = OD::LineStyle( OD::LineStyle::Solid,
						     disp.size(),
						     disp.color() );

    par.getYN( sKeyFill(), disp.polyDisp()->dofill_ );
    if ( par.hasKey(sKeyConnect()) )
    {
	bool doconnect = false;
	par.getYN( sKeyConnect(), doconnect );	// For Backward Compatibility
	if ( doconnect )
	    disp.polyDisp()->connect_ = Connection::Close;
	else
	{
	    if ( !parseEnumConnection(par.find(sKeyConnect()),
		 disp.polyDisp()->connect_) )
		disp.polyDisp()->connect_ = Connection::None;
	}
    }

    return true;
}


bool Set::useDisplayPars( const IOPar& par )
{
    return useDisplayPars3D( par, disp3d_ ) && useDisplayPars2D( par, disp2d_ );
}


bool Set::useDisplayPars2D( const IOPar& par, Disp2D& disp )
{
    const IOPar* par2d = par.subselect( "2D" );
    if ( !par2d || par2d->isEmpty() )
    {
	const MarkerStyle3D::Type typ3d = (MarkerStyle3D::Type) disp3d_.type();
	const MarkerStyle2D::Type typ2d = MarkerStyle3D::getMS2DType( typ3d );
	disp2d_.markerstyle_ = MarkerStyle2D( typ2d,
					      disp3d_.size(),disp3d_.color() );
	if ( isPolygon() )
	    *disp2d_.polyDisp() = *disp3d_.polyDisp();

	return true;
    }

    BufferString colstr;
    const bool hascolor = par2d->get( sKey::Color(), colstr );
    if ( hascolor )
	disp.color().use( colstr.buf() );

    BufferString fillcolstr;
    if ( par2d->get(sKeyFillColor(),fillcolstr) )
	disp.polyDisp()->fillcolor_.use( fillcolstr.buf() );

    const bool hasdispsz = par2d->hasKey( sKey::Size() );
    int size = defPixSz();
    if ( hasdispsz )
	par2d->get( sKey::Size(), size );

    disp.markerstyle_.size_ = size;

    int markertype = 0;
    if ( par2d->hasKey(sKeyMarkerType()) )
	par2d->get( sKeyMarkerType(), markertype );

    disp.markerstyle_.type_ = (MarkerStyle2D::Type) markertype;

    if ( !isPolygon() )
	return true;

    BufferString lsstr;
    if ( par2d->get(sKey::LineStyle(),lsstr) )
	disp.polyDisp()->linestyle_.fromString( lsstr );
    else if ( hasdispsz && hascolor )
	disp.polyDisp()->linestyle_ = OD::LineStyle( OD::LineStyle::Solid,
	    disp.size(),
	    disp.color() );

    par2d->getYN( sKeyFill(), disp.polyDisp()->dofill_ );
    if ( par2d->hasKey(sKeyConnect()) )
    {
	bool doconnect = false;
	par2d->getYN( sKeyConnect(), doconnect );// For Backward Compatibility
	if ( doconnect )
	    disp.polyDisp()->connect_ = Connection::Close;
	else
	{
	    if ( !parseEnumConnection(par2d->find(sKeyConnect()),
		disp.polyDisp()->connect_) )
		disp.polyDisp()->connect_ = Connection::None;
	}
    }

    return true;
}


Set::Disp3D& Set::disp3d()
{
    return disp3d_;
}


const Set::Disp3D& Set::disp3d() const
{
    return getNonConst(this)->disp3d();
}


Set::Disp2D& Set::disp2d()
{
    return disp2d_;
}


const Set::Disp2D& Set::disp2d() const
{
    return getNonConst(this)->disp2d();
}


bool Set::writeDisplayPars() const
{
    const MultiID& mid = Pick::Mgr().get( *this );
    if ( mid.isUdf() )
	return false;

    IOPar par;
    fillDisplayPars( par );
    return Pick::Mgr().writeDisplayPars( mid, par );
}


void Set::addUndoEvent( EventType type, int idx, const Pick::Location& loc )
{
    Pick::SetMgr& mgr = Pick::Mgr();
    if ( mgr.indexOf(*this) == -1 )
	return;

    const MultiID mid = mgr.get(*this);
    if ( !mid.isUdf() )
    {
	auto undotype = sCast(PickSetKnotUndoEvent::UnDoType,type);
	const Pick::Location pos = type == Insert ?
				    Pick::Location(Coord3::udf()) : loc;
	PickSetKnotUndoEvent* undo = new PickSetKnotUndoEvent(
	undotype, mid, idx, pos );
	Pick::Mgr().undo().addEvent( undo, 0 );
    }
}


void Set::insertWithUndo( int idx, const Pick::Location& loc )
{
    insert( idx, loc );
    addUndoEvent( Insert, idx, loc );
 }


void Set::appendWithUndo( const Pick::Location& loc )
{
    add( loc );
    addUndoEvent( Insert, size()-1, loc );
 }


void Set::removeSingleWithUndo( int idx )
{
    const Pick::Location pos = get( idx );
    addUndoEvent( Remove, idx, pos );
    remove( idx );
}


void Set::moveWithUndo( int idx, const Pick::Location& undoloc,
			const Pick::Location& loc )
{
    if ( size() < idx )
	return;

    set( idx, undoloc );
    addUndoEvent( Move, idx, loc );
    set( idx, loc );
}


void Set::addBulkUndoEvent( EventType type, const TypeSet<int>& indexes,
			    const TypeSet<Pick::Location>& locs )
{
    Pick::SetMgr& mgr = Pick::Mgr();
    if ( mgr.indexOf(*this) == -1 )
	return;

    const MultiID mid = mgr.get(*this);
    if ( !mid.isUdf() )
    {
	PickSetBulkUndoEvent::UnDoType undotype =
	    type == Insert ? PickSetBulkUndoEvent::Insert
			   : PickSetBulkUndoEvent::Remove;

	PickSetBulkUndoEvent* undo =
		new PickSetBulkUndoEvent( undotype, mid, indexes, locs );
	Pick::Mgr().undo().addEvent( undo, 0 );
    }
}


void Set::bulkAppendWithUndo( const TypeSet<Pick::Location>& locs,
			      const TypeSet<int>& indexes )
{
    locations_.append( locs );
    addBulkUndoEvent( Insert, indexes, locs );
}


void Set::bulkRemoveWithUndo( const TypeSet<Pick::Location>& locs,
			      const TypeSet<int>& indexes )
{
    for ( int idx=indexes.size()-1; idx>=0; idx-- )
	this->remove( indexes[idx] );

    addBulkUndoEvent( Remove, indexes, locs );
}


bool Set::isSizeLargerThanThreshold() const
{
    bool usethreshold = true;
    Settings::common().getYN( sKeyUseThreshold(), usethreshold );
    return usethreshold && size() >= getSizeThreshold();
}


const Location& Set::get( int idx ) const
{
    return locations_[idx];
}


void Set::refNotify() const
{
}


void Set::unRefNotify() const
{
}


bool Set::fillDataPointSet( DataPointSet& dps ) const
{
    if ( !is2D() && dps.is2D() )
    {
	pErrMsg("Filling a 2D DataPointSet from 3D PointSet");
	return false;
    }

    for ( int idx=0; idx<size(); idx++ )
    {
	const Location& loc = get( idx );
	DataPointSet::DataRow dr;
	if ( dps.is2D() )
	{
	    dr.pos_.trckey_ = loc.trcKey();
	    dr.pos_.z_ = loc.z();
	}
	else
	    dr.pos_.set( loc.pos() );

	dps.addRow( dr );
    }

    return true;
}

} // namespace Pick


// PickSetAscIO
Table::FormatDesc* PickSetAscIO::getDesc( bool iszreq )
{
    Table::FormatDesc* fd = new Table::FormatDesc( "PickSet" );
    createDescBody( fd, iszreq );
    return fd;
}


void PickSetAscIO::createDescBody( Table::FormatDesc* fd, bool iszreq )
{
    fd->bodyinfos_ += Table::TargetInfo::mkHorPosition( true );
    if ( iszreq )
	fd->bodyinfos_ += Table::TargetInfo::mkZPosition( true );
}


void PickSetAscIO::updateDesc( Table::FormatDesc& fd, bool iszreq )
{
    fd.bodyinfos_.erase();
    createDescBody( &fd, iszreq );
}


bool PickSetAscIO::isXY() const
{
    return formOf( false, 0 ) == 0;
}


#define mErrRet(s) { if ( !s.isEmpty() ) errmsg_ = s; return 0; }

bool PickSetAscIO::get( od_istream& strm, Pick::Set& ps,
			bool iszreq, float constz ) const
{
    while ( true )
    {
	int ret = getNextBodyVals( strm );
	if ( ret < 0 ) mErrRet(errmsg_)
	if ( ret == 0 ) break;

	Coord pos( getPos(0, 1) );
	if ( pos.isUdf() )
	    continue;

	mPIEPAdj(Coord,pos,true);
	if ( !isXY() )
	{
            BinID bid( mNINT32(pos.x_), mNINT32(pos.y_) );
	    mPIEPAdj(BinID,bid,true);
	    SI().snap( bid );
	    pos = SI().transform( bid );
	}

	float zread = constz;
	if ( iszreq )
	{
	    zread = getFValue( 2 );
	    if ( mIsUdf(zread) )
		continue;

	    mPIEPAdj(Z,zread,true);
	}

	ps.add( Pick::Location(pos,zread) );
    }

    return true;
}

/*
Pick::List& Pick::List::add( const Location& loc, bool mkcopy )
{
    if ( mkcopy )
	*this += new Location( loc );
    else
	*this += const_cast<Location*>( &loc );
    return *this;
}


Pick::List::LocID Pick::List::find( const TrcKey& tk ) const
{
    return findIdx( *this, tk );
}


Pick::List::LocID Pick::List::nearestLocation( const Coord& pos ) const
{
    return getNearestLocation( *this, Coord3(pos.x,pos.y,0.f), true );
}


Pick::List::LocID Pick::List::nearestLocation( const Coord3& pos,
						 bool ignorez ) const
{
    return getNearestLocation( *this, pos, ignorez );
}


Pick::Location& Pick::List::get( LocID idx )
{ return *(*this)[idx]; }


const Pick::Location& Pick::List::get( LocID idx ) const
{ return *(*this)[idx]; } */
