/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : Apr 2002
-*/


#include "emobject.h"

#include "color.h"
#include "emioobjinfo.h"
#include "emundo.h"
#include "emsurfacetr.h"
#include "emmanager.h"
#include "emobjectposselector.h"
#include "geomelement.h"
#include "ioman.h"
#include "ioobj.h"
#include "iopar.h"
#include "rowcolsurface.h"
#include "ptrman.h"
#include "selector.h"
#include "survinfo.h"
#include "uistrings.h"

namespace EM
{

int EMObject::sPermanentControlNode() {return PosAttrib::PermanentControlNode;}
int EMObject::sTemporaryControlNode() {return PosAttrib::TemporaryControlNode;}
int EMObject::sEdgeControlNode()	{ return PosAttrib::EdgeControlNode; }
int EMObject::sTerminationNode()	{ return PosAttrib::TerminationNode; }
int EMObject::sSeedNode()		{ return PosAttrib::SeedNode; }
int EMObject::sIntersectionNode()	{ return PosAttrib::IntersectionNode; }

const char* EMObject::posattrprefixstr()	{ return "Pos Attrib "; }
const char* EMObject::posattrsectionstr()	{ return " Section"; }
const char* EMObject::posattrposidstr()		{ return " SubID"; }
const char* EMObject::nrposattrstr()		{ return "Nr Pos Attribs"; }

static const char* sLockColor()			{ return "Lock Color"; }
static const char* sSelectionColor()		{ return "Selection Color"; }


EMObject::EMObject( EMManager& emm )
    : manager_(emm)
    , change(this)
    , id_(-1)
    , storageid_(MultiID::udf())
    , preferredcolor_(*new OD::Color(OD::Color::Green()))
    , lockcolor_(OD::Color::Blue())
    , selectioncolor_(OD::Color::Orange())
    , changed_(false)
    , fullyloaded_(false)
    , locked_(false)
    , burstalertcount_(0)
    , selremoving_(false)
    , preferredlinestyle_(*new OD::LineStyle(OD::LineStyle::Solid,3))
    , preferredmarkerstyle_(
	*new MarkerStyle3D(MarkerStyle3D::Cube,2,OD::Color::White()))
    , posattribmarkerstyle_(*new MarkerStyle3D(MarkerStyle3D::Cube,2,
			    preferredcolor_.complementaryColor()))
    , haslockednodes_(false)
    , zdomain_(&ZDomain::SI())
{
    mDefineStaticLocalObject( Threads::Atomic<int>, oid, (0) );
    id_.set( oid++ );

    removebypolyposbox_.setEmpty();

    mAttachCB(change,EMObject::posIDChangeCB);
}


EMObject::~EMObject()
{
    detachAllNotifiers();
    //TODO: replace:
    prepareForDelete();
    //And replace by sendDelNotif(), which called from ~SharedObject
    deepErase( posattribs_ );
    delete &preferredcolor_;
    delete &preferredlinestyle_;
    delete &preferredmarkerstyle_;
    delete &posattribmarkerstyle_;

    id_.set( -2 );	//To check easier if it has been deleted
}


void EMObject::prepareForDelete() const
{
    manager_.removeObject( this );
}


void EMObject::prepareForDelete()
{
    manager_.removeObject( this );
}


void EMObject::setNewName()
{
    setName("<New EM Object>");
}


void EMObject::setMultiID( const MultiID& mid )
{
    storageid_ = mid;
    PtrMan<IOObj> ioobj = IOM().get( multiID() );
    if ( ioobj )
	name_ = ioobj->name();
}


int EMObject::sectionIndex( const SectionID& sid ) const
{
    for ( int idx=0; idx<nrSections(); idx++ )
	if ( sectionID(idx) == sid )
	    return idx;

    return -1;
}


BufferString EMObject::sectionName( const SectionID& sid ) const
{
    return BufferString( toString(sid) );
}


bool EMObject::canSetSectionName() const
{ return false; }


bool EMObject::setSectionName( const SectionID&, const char*, bool )
{ return false; }


const Geometry::Element* EMObject::sectionGeometry( const SectionID& sec ) const
{ return const_cast<EMObject*>(this)->sectionGeometryInternal(sec); }


Geometry::Element* EMObject::sectionGeometry( const SectionID& sec )
{ return sectionGeometryInternal(sec); }


Geometry::Element* EMObject::sectionGeometryInternal( const SectionID& )
{ return 0; }


Coord3 EMObject::getPos( const PosID& pid ) const
{
    if ( pid.objectID() != id() )
	return  Coord3::udf();

    return getPos( pid.sectionID(), pid.subID() );
}


Coord3 EMObject::getPos( const EM::SectionID& sid,
			 const EM::SubID& subid ) const
{
    const Geometry::Element* element = sectionGeometry( sid );
    return element ? element->getPosition( subid ) : Coord3::udf();
}


#define mRetErr( msg ) { errmsg_ = msg; return false; }

bool EMObject::setPos( const PosID& pid, const Coord3& newpos, bool addtoundo )
{
    if ( pid.objectID()!=id() )
	mRetErr(uiString::emptyString());

    return setPos( pid.sectionID(), pid.subID(), newpos, addtoundo );
}


bool EMObject::setPos( const SectionID& sid, const SubID& subid,
		       const Coord3& newpos, bool addtoundo )
{
    Threads::Locker locker( setposlock_ );

    Geometry::Element* element = sectionGeometryInternal( sid );
    if ( !element ) mRetErr( uiString::emptyString() );

    Coord3 oldpos = Coord3::udf();
    if ( addtoundo )
	oldpos = element->getPosition( subid );

    if ( !element->setPosition(subid,newpos) )
	 mRetErr( element->errMsg() );

    const PosID pid (id(), sid, subid );

    if ( !newpos.isDefined() )
    {
	for ( int idx=0; idx<posattribs_.size(); idx++ )
	{
	    if ( !posattribs_[idx] )
		continue;

	    TypeSet<PosID>& nodes = posattribs_[idx]->posids_;
	    if ( nodes.isPresent(pid) )
		setPosAttrib( pid, attribs_[idx], false, addtoundo );
	}
    }

    if ( addtoundo )
    {
	UndoEvent* undo = new SetPosUndoEvent( oldpos, pid );
	EMM().undo(id()).addEvent( undo, 0 );
    }

    if ( burstalertcount_==0 )
    {
	EMObjectCallbackData cbdata;
	cbdata.event = EMObjectCallbackData::PositionChange;
	cbdata.pid0 = pid;
	change.trigger( cbdata );
    }

    const BinID bid = BinID::fromInt64(subid);
    const TrcKey tk = TrcKey(bid);
    setNodeSourceType( tk, None );
    changed_ = true;
    return true;
}


bool EMObject::isAtEdge( const PosID& ) const
{
    pErrMsg("Not implemented");
    return false;
}


const OD::LineStyle& EMObject::preferredLineStyle() const
{
    return preferredlinestyle_;
}


void EMObject::setPreferredLineStyle( const OD::LineStyle& lnst )
{
    if ( preferredlinestyle_ == lnst )
	return;
    preferredlinestyle_ = lnst;

    EMObjectCallbackData cbdata;
    cbdata.event = EMObjectCallbackData::PrefColorChange;
    change.trigger( cbdata );

    saveDisplayPars();
}


const OD::Color& EMObject::preferredColor() const
{
    return preferredcolor_;
}


void EMObject::setPreferredColor( const OD::Color& col, bool addtoundo )
{
    if ( col==preferredcolor_ )
	return;

    if ( addtoundo )
    {
	UndoEvent* undo = new SetPrefColorEvent( id(), preferredcolor_, col );
	EMM().undo(id()).addEvent( undo );
    }

    preferredcolor_ = col;
    EMObjectCallbackData cbdata;
    cbdata.event = EMObjectCallbackData::PrefColorChange;
    change.trigger( cbdata );

    saveDisplayPars();
}


void EMObject::setSelectionColor( const OD::Color& clr )
{
    selectioncolor_ = clr;
    EMObjectCallbackData cbdata;
    cbdata.event = EMObjectCallbackData::PrefColorChange;
    change.trigger(cbdata);
}


const OD::Color& EMObject::getSelectionColor() const
{
    return selectioncolor_;
}


void EMObject::setLockColor( const OD::Color& col )
{
    lockcolor_ = col;
    EMObjectCallbackData cbdata;
    cbdata.event = EMObjectCallbackData::LockColorChange;
    change.trigger( cbdata );
}


const OD::Color& EMObject::getLockColor() const
{
    return lockcolor_;
}


void EMObject::setBurstAlert( bool yn )
{
    if ( !yn && burstalertcount_==0 )
	return;

    if ( !yn )
	burstalertcount_--;

    if ( burstalertcount_==0 )
    {
	if ( yn ) burstalertcount_++;
	EMObjectCallbackData cbdata;
	cbdata.flagfor2dviewer = !yn;
	cbdata.event = EMObjectCallbackData::BurstAlert;
	change.trigger( cbdata );
    }
    else if ( yn )
	burstalertcount_++;
}


bool EMObject::hasBurstAlert() const
{ return burstalertcount_>0; }


bool EMObject::unSetPos(const PosID& pid, bool addtoundo )
{
    return setPos( pid, Coord3::udf(), addtoundo );
}


bool EMObject::unSetPos( const EM::SectionID& sid, const EM::SubID& subid,
			 bool addtoundo )
{
    return setPos( sid, subid, Coord3::udf(), addtoundo );
}


bool EMObject::enableGeometryChecks( bool )
{ return true; }


bool EMObject::isGeometryChecksEnabled() const
{ return true; }


void EMObject::changePosID( const PosID& from, const PosID& to,
			    bool addtoundo )
{
    if ( from==to )
    {
	pErrMsg("From and to are identical");
	return;
    }

    if ( from.objectID()!=id() || to.objectID()!=id() )
	return;

    const Coord3 tosprevpos = getPos( to );
    setPos( to, getPos(from), false );

    if ( addtoundo )
    {
	PosIDChangeEvent* event = new PosIDChangeEvent( from, to, tosprevpos );
	EMM().undo(id()).addEvent( event, 0 );
    }

    EMObjectCallbackData cbdata;
    cbdata.event = EMObjectCallbackData::PosIDChange;
    cbdata.pid0 = from;
    cbdata.pid1 = to;
    change.trigger( cbdata );

    /*The unset must be after the trigger, becaues otherwise the old pos
	cannot be retrieved for the cb. In addition, the posattrib status of
	the node is needed during the cbs, and that is removed when unsetting
	the pos.  */
    unSetPos( from, false );
}


bool EMObject::isDefined( const PosID& pid ) const
{
    if ( pid.objectID()!=id() )
	return  false;

    return isDefined( pid.sectionID(), pid.subID() );
}



bool EMObject::isDefined( const EM::SectionID& sid,
			  const EM::SubID& subid ) const
{
    const Geometry::Element* element = sectionGeometry( sid );
    return element && element->isDefined( subid );
}


void EMObject::addPosAttrib( int attr )
{
    if ( attribs_.indexOf(attr) < 0 )
    {
	attribs_ += attr;
	posattribs_ += new PosAttrib();
	const int idx = attribs_.indexOf( attr );
	posattribs_[idx]->type_ = (PosAttrib::Type)attr;
	posattribs_[idx]->locked_ = false;
    }
}


void EMObject::removePosAttribList( int attr, bool addtoundo )
{
    const int idx=attribs_.indexOf( attr );
    if ( idx==-1 )
	return;

    const TypeSet<PosID>& attrlist = posattribs_[idx]->posids_;

    while ( attrlist.size() )
	setPosAttrib( attrlist[0], attr, false, addtoundo );
}


void EMObject::setPosAttrib( const PosID& pid, int attr, bool yn,
			     bool addtoundo )
{
    EMObjectCallbackData cbdata;
    cbdata.event = EMObjectCallbackData::AttribChange;
    cbdata.pid0 = pid;
    cbdata.attrib = attr;

    if ( yn )
	addPosAttrib( attr );

    const int idx = attribs_.indexOf(attr);
    if ( idx == -1 )
	return;

    TypeSet<PosID>& posids = posattribs_[idx]->posids_;
    const int idy=posids.indexOf( pid );

    if ( idy==-1 && yn )
	posids += pid;
    else if ( idy!=-1 && !yn )
	posids.removeSingle( idy, false );
    else
	return;

    if ( addtoundo )
    {
	UndoEvent* event = new SetPosAttribUndoEvent( pid, attr, yn );
	EMM().undo(id()).addEvent( event, 0 );
    }

    if ( !hasBurstAlert() )
	change.trigger( cbdata );
    changed_ = true;
}


bool EMObject::isPosAttrib( const PosID& pid, int attr ) const
{
    const int idx = attribs_.indexOf( attr );
    return idx != -1 && posattribs_[idx]->posids_.isPresent( pid );
}


const char* EMObject::posAttribName( int idx ) const
{ return 0; }

int EMObject::nrPosAttribs() const
{ return attribs_.size(); }

int EMObject::posAttrib(int idx) const
{ return attribs_[idx]; }

int EMObject::addPosAttribName( const char* nm )
{ return -1; }

bool EMObject::hasPosAttrib(int attr) const
{
    const int idx=attribs_.indexOf( attr );
    return idx!=-1 ? !posattribs_[idx]->posids_.isEmpty() : false;
}


const TypeSet<PosID>* EMObject::getPosAttribList( int attr ) const
{
    const int idx=attribs_.indexOf( attr );
    return idx!=-1 ? &posattribs_[idx]->posids_ : 0;
}


const MarkerStyle3D& EMObject::getPosAttrMarkerStyle( int attr ) const
{
    return attr == sSeedNode() ? preferredMarkerStyle3D()
				: posattribmarkerstyle_;
}


void EMObject::setPosAttrMarkerStyle( int attr, const MarkerStyle3D& ms )
{
    addPosAttrib( attr );
    if ( attr == sSeedNode() )
	setPreferredMarkerStyle3D( ms );
    else
	posattribmarkerstyle_ = ms;

    EMObjectCallbackData cbdata;
    cbdata.event = EMObjectCallbackData::AttribChange;
    cbdata.attrib = attr;
    change.trigger( cbdata );
}


const MarkerStyle3D& EMObject::preferredMarkerStyle3D() const
{
    return preferredmarkerstyle_;
}


void EMObject::setPreferredMarkerStyle3D( const MarkerStyle3D& mkst )
{
    if( mkst == preferredmarkerstyle_ )
	return;

    preferredmarkerstyle_ = mkst;
    saveDisplayPars();
}


void EMObject::lockPosAttrib( int attr, bool yn )
{
    addPosAttrib( attr );
    const int idx=attribs_.indexOf( attr );
    posattribs_[idx]->locked_ = yn;
}


bool EMObject::isPosAttribLocked( int attr ) const
{
    const int idx=attribs_.indexOf( attr );
    return idx!=-1 ? posattribs_[idx]->locked_ : false;
}


void EMObject::removeSelected( const Selector<Coord3>& selector,
			       TaskRunner* tr )
{
    if ( !selector.isOK() )
	return;

    insideselremoval_ = true;
    removebypolyposbox_.setEmpty();

    for ( int idx=0; idx<nrSections(); idx++ )
    {
	ObjectSet<const Selector<Coord3> > selectors;
	selectors += &selector;
	EMObjectPosSelector posselector( *this, sectionID(idx), selectors );
	posselector.executeParallel( tr );

	const TypeSet<EM::SubID>& list = posselector.getSelected();
	removeSelected( list );
    }

    insideselremoval_ = false;
}


void EMObject::removeSelected( const TypeSet<EM::SubID>& subids )
{
    for ( int idx = 0; idx<nrSections(); idx++ )
    {
	Geometry::Element* ge = sectionGeometry(sectionID(idx));
	if ( !ge ) continue;

	mDynamicCastGet( const Geometry::RowColSurface*, surface, ge );
	if ( !surface ) continue;

	setBurstAlert( true );
	int poscount = 0;
	ge->blockCallBacks( true, false );

	for ( int sididx=0; sididx<subids.size(); sididx++ )
	{
	    unSetPos( sectionID(idx), subids[sididx], true );
	    BinID bid = BinID::fromInt64( subids[sididx] );
	    const Coord3 pos = getPos( sectionID(idx), subids[sididx] );
	    if ( removebypolyposbox_.isEmpty() )
	    {
		removebypolyposbox_.hsamp_.start_ =
		    removebypolyposbox_.hsamp_.stop_ = bid;
		removebypolyposbox_.zsamp_.start =
		    removebypolyposbox_.zsamp_.stop = (float) pos.z;
	    }
	    else
	    {
		removebypolyposbox_.hsamp_.include(bid);
		removebypolyposbox_.zsamp_.include((float) pos.z);
	    }
	    if ( ++poscount >= 10000 )
	    {
		ge->blockCallBacks(true,true);
		poscount = 0;
	    }
	}
	ge->blockCallBacks( false, true );
	setBurstAlert(false);
    }
}


void EMObject::removeListOfSubIDs( const TypeSet<EM::SubID>& subids,
				   const EM::SectionID& sectionid )
{
    for ( int sididx = 0; sididx < subids.size(); sididx++ )
    {
	if ( sididx == 0 )
	    setBurstAlert( true );
	unSetPos( sectionid, subids[sididx], true );
	if ( sididx == subids.size()-1 )
	    setBurstAlert( false );
    }
}


void EMObject::removeAllUnSeedPos()
{
    setBurstAlert( true );
    PtrMan<EM::EMObjectIterator> iterator = createIterator( -1 );
    while( true )
    {
	const EM::PosID pid = iterator->next();
	if ( !pid.isValid() )
	    break;

	if ( !isPosAttrib(pid, EM::EMObject::sSeedNode()) &&
	     !isNodeSourceType(pid,Manual) &&
	     !isNodeLocked(pid) )
	    unSetPos( pid, true );
    }
    setBurstAlert( false );
}


const TrcKeyZSampling EMObject::getRemovedPolySelectedPosBox()
{
    return removebypolyposbox_;
}


void EMObject::emptyRemovedPolySelectedPosBox()
{
    removebypolyposbox_.setEmpty();
}


bool EMObject::isEmpty() const
{
    PtrMan<EM::EMObjectIterator> iterator = createIterator( -1 );
    return !iterator || !iterator->next().isValid();
}


uiString EMObject::errMsg() const
{
    return errmsg_;
}


bool EMObject::usePar( const IOPar& par )
{
   useDisplayPars( par );

    for ( int idx=0; idx<nrPosAttribs(); idx++ )
	removePosAttribList( posAttrib(idx), false );

    int nrattribs = 0;
    par.get( nrposattrstr(), nrattribs );
    for ( int idx=0; idx<nrattribs; idx++ )
    {
	BufferString attribkey = posattrprefixstr();
	attribkey += idx;

	int attrib;
	if ( !par.get(attribkey.buf(),attrib) )
	    continue;

	TypeSet<int> sections;
	TypeSet<SubID> subids;

	BufferString sectionkey = attribkey;
	sectionkey += posattrsectionstr();

	BufferString subidkey = attribkey;
	subidkey += posattrposidstr();

	par.get( sectionkey.buf(), sections );
	par.get( subidkey.buf(), subids );

	const int minsz = mMIN( sections.size(), subids.size() );
	for ( int idy=0; idy<minsz; idy++ )
	{
	    if ( !isDefined(mCast(EM::SectionID,sections[idy]),subids[idy]) )
		continue;
	    const PosID pid = PosID( id(),
		sCast(EM::SectionID,sections[idy]), subids[idy] );
	    setPosAttrib( pid, attrib, true, false );
	}
    }

    return true;
}


void EMObject::fillPar( IOPar& par ) const
{
    saveDisplayPars();

    int keyid = 0;
    for ( int idx=0; idx<nrPosAttribs(); idx++ )
    {
	const int attrib = posAttrib( idx );
	const TypeSet<PosID>* pids = getPosAttribList( attrib );
	if ( !pids ) continue;

	BufferString attribkey = posattrprefixstr();
	attribkey += keyid++;
	par.set( attribkey.buf(), attrib );

	TypeSet<int> attrpatches;
	TypeSet<SubID> subids;
	for ( int idy=0; idy<pids->size(); idy++ )
	{
	    attrpatches += (*pids)[idy].sectionID();
	    subids += (*pids)[idy].subID();
	}

	BufferString patchkey = attribkey;
	patchkey += posattrsectionstr();
	BufferString subidkey = attribkey;
	subidkey += posattrposidstr();

	par.set( patchkey.buf(), attrpatches );
	par.set( subidkey.buf(), subids );
    }

    par.set( nrposattrstr(), keyid );
}


bool EMObject::useDisplayPar( const IOPar& par )
{
    OD::Color col;
    if ( par.get(sKey::Color(),col) )
    {
	col.setTransparency( 0 );
	preferredcolor_ = col;
    }

    if ( par.get(sLockColor(),col) )
	lockcolor_ = col;

    if ( par.get(sSelectionColor(),col) )
	selectioncolor_ = col;

    BufferString lnststr;
    if ( par.get(sKey::LineStyle(),lnststr) )
    {
	OD::LineStyle lnst;
	lnst.fromString( lnststr );
	preferredlinestyle_ = lnst;
    }

    BufferString mkststr;
    if ( par.get(sKey::MarkerStyle(),mkststr) )
    {
	MarkerStyle3D mkst;
	mkst.fromString( mkststr );
	preferredmarkerstyle_ = mkst;
    }

    const BufferString posattribmskey =
		IOPar::compKey( posattrprefixstr(), sKey::MarkerStyle() );
    if ( par.get(posattribmskey,mkststr) )
    {
	MarkerStyle3D mkst;
	mkst.fromString( mkststr );
	posattribmarkerstyle_ = mkst;
    }

    return true;
}


void EMObject::fillDisplayPar( IOPar& par ) const
{
    par.set( sKey::Color(), preferredColor() );
    par.set( sLockColor(), getLockColor() );
    par.set( sSelectionColor(), getSelectionColor() );

    BufferString lnststr;
    preferredlinestyle_.toString( lnststr );
    par.set( sKey::LineStyle(), lnststr );

    BufferString mkststr;
    preferredmarkerstyle_.toString( mkststr );
    par.set( sKey::MarkerStyle(), mkststr );

    const BufferString posattribmskey =
		IOPar::compKey( posattrprefixstr(), sKey::MarkerStyle() );
    posattribmarkerstyle_.toString( mkststr );
    par.set( posattribmskey, mkststr );
}


void EMObject::useDisplayPars( const IOPar& par )
{
    IOPar displaypar;
    if ( !EMM().readDisplayPars(storageid_,displaypar) )
	displaypar = par;

    useDisplayPar( displaypar );
}


void EMObject::saveDisplayPars() const
{
    IOPar displaypar;
    fillDisplayPar( displaypar );
    EMM().writeDisplayPars( storageid_, displaypar );
}


void EMObject::posIDChangeCB(CallBacker* cb)
{
    mCBCapsuleUnpack(const EMObjectCallbackData&,cbdata,cb);
    if ( cbdata.event != EMObjectCallbackData::PosIDChange )
	return;

    for ( int idx=0; idx<posattribs_.size(); idx++ )
    {
	if ( !posattribs_[idx] )
	    continue;

	TypeSet<PosID>& nodes = posattribs_[idx]->posids_;
	while ( true )
	{
	    const int idy = nodes.indexOf( cbdata.pid0 );
	    if ( idy==-1 )
		break;

	    nodes[idy] = cbdata.pid1;
	}
    }
}


Interval<float> EMObject::getZRange( bool docompute ) const
{
    Interval<float> zrg = Interval<float>::udf();
    const bool ischanged = isChanged();
    if ( !ischanged )
    {
	IOObjInfo info( multiID() );
	zrg = info.getZRange();
    }

    if ( !zrg.isUdf() || !docompute )
	return zrg;

    PtrMan<EMObjectIterator> it = createIterator( -1, nullptr );
    if ( !it )
	return zrg;

    EM::PosID pid = it->next();
    while ( pid.isValid() )
    {
	const double depth = getPos( pid ).z;
	if ( !mIsUdf(depth) )
	    zrg.include( depth, false );

	pid = it->next();
    }

    return zrg;
}


bool EMObject::isZInDepth() const
{
    return zdomain_->isDepth();
}


void EMObject::setZInDepth()
{
    if ( isZInDepth() )
	return;

    zdomain_ = &ZDomain::Depth();
}


void EMObject::setZInTime()
{
    if ( !isZInDepth() )
	return;

    zdomain_ = &ZDomain::Time();
}

} // namespace EM
