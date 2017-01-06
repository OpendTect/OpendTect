/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : Apr 2002
-*/


#include "emobject.h"

#include "color.h"
#include "emundo.h"
#include "emsurfacetr.h"
#include "emmanager.h"
#include "emobjectposselector.h"
#include "geomelement.h"
#include "dbman.h"
#include "ioobj.h"
#include "iopar.h"
#include "rowcolsurface.h"
#include "ptrman.h"
#include "selector.h"
#include "survinfo.h"
#include "uistrings.h"
#include "keystrs.h"

namespace EM
{

int EMObject::sTerminationNode()	{ return PosAttrib::TerminationNode; }
int EMObject::sSeedNode()		{ return PosAttrib::SeedNode; }
int EMObject::sIntersectionNode()	{ return PosAttrib::IntersectionNode; }

const char* EMObject::posattrprefixstr()    { return "Pos Attrib "; }
const char* EMObject::posattrsectionstr()   { return " Section"; }
const char* EMObject::posattrposidstr()	    { return " SubID"; }
const char* EMObject::nrposattrstr()	    { return "Nr Pos Attribs"; }

Color EMObject::sDefaultSelectionColor() { return Color::Orange(); }
Threads::Atomic<int>	EMObjectCallbackData::curcbid_ = 0;

EMObject::EMObject( EMManager& emm )
    : SharedObject( "" )
    , manager_( emm )
    , change( this )
    , id_( -1 )
    , preferredcolor_( *new Color(Color::Green()) )
    , changed_( false )
    , fullyloaded_( false )
    , locked_( false )
    , burstalertcount_( 0 )
    , selremoving_( false )
    , preferredlinestyle_( *new OD::LineStyle(OD::LineStyle::Solid,3) )
    , preferredmarkerstyle_(
	*new OD::MarkerStyle3D(OD::MarkerStyle3D::Cube,2,Color::White()))
    , selectioncolor_( *new Color(sDefaultSelectionColor()) )
    , haslockednodes_( false )
{
    mDefineStaticLocalObject( Threads::Atomic<int>, oid, (0) );
    id_ = oid++;

    removebypolyposbox_.setEmpty();

    change.notify( mCB(this,EMObject,posIDChangeCB) );
}


EMObject::~EMObject()
{
    deepErase( posattribs_ );
    delete &preferredcolor_;
    delete &preferredlinestyle_;
    delete &preferredmarkerstyle_;
    delete &selectioncolor_;

    change.remove( mCB(this,EMObject,posIDChangeCB) );
    id_ = -2;	//To check easier if it has been deleted
    deepErase( emcbdatas_ );
}


void EMObject::prepareForDelete()
{
    manager_.removeObject( this );
}


void EMObject::setNewName()
{ setName("<New EM Object>"); }

void EMObject::setDBKey( const DBKey& mid )
{
    storageid_ = mid;
    PtrMan<IOObj> ioobj = DBM().get( storageid_ );
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


Geometry::Element* EMObject::sectionGeometryInternal( const SectionID& sec )
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

bool EMObject::setPos(	const PosID& pid, const Coord3& newpos,
			bool addtoundo, NodeSourceType tp )
{
    if ( pid.objectID()!=id() )
	mRetErr(uiString::emptyString());

    return setPosition( pid.sectionID(), pid.subID(), newpos, addtoundo, tp );
}


bool EMObject::setPos(	const SectionID& sid, const SubID& subid,
			const Coord3& newpos, bool addtoundo,
			NodeSourceType tp )
{
    return setPosition( sid, subid, newpos, addtoundo, tp );
}


bool EMObject::setPosition( const SectionID& sid, const SubID& subid,
			    const Coord3& newpos, bool addtoundo,
			    NodeSourceType tp )
{
    //Threads::Locker locker( setposlock_ );
    mLock4Write();

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
	    TypeSet<PosID>& nodes = posattribs_[idx]->posids_;
	    if ( !&nodes ) continue;

	    if ( nodes.isPresent(pid) )
		setPosAttrib( pid, attribs_[idx], false, addtoundo );
	}
    }

    if ( addtoundo )
    {
	UndoEvent* undo = new SetPosUndoEvent( oldpos, pid );
	EMM().undo().addEvent( undo, 0 );
    }

    if ( burstalertcount_==0 )
    {
	mSendEMCBNotifPosID( EMObjectCallbackData::PositionChange, pid );
    }

    changed_ = true;
    return true;
}


bool EMObject::isAtEdge( const PosID& ) const
{
    pErrMsg("Not implemented");
    return false;
}


void EMObject::setBurstAlert( bool yn )
{
    mLock4Write();

    if ( !yn && burstalertcount_==0 )
	return;

    if ( !yn )
	burstalertcount_--;

    if ( burstalertcount_==0 )
    {
	if ( yn ) burstalertcount_++;
	EMObjectCallbackData* cbdata = getNewEMCBData();
	cbdata->flagfor2dviewer = !yn;
	cbdata->event = EMObjectCallbackData::BurstAlert;
	mSendChgNotif( cPositionChange(), cbdata->cbID().getI() );
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
    mLock4Write();

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
	EMM().undo().addEvent( event, 0 );
    }

    if ( !hasBurstAlert() )
    {
	EMObjectCallbackData* cbdata = getNewEMCBData();
	cbdata->event = EMObjectCallbackData::AttribChange;
	cbdata->pid0 = pid;
	cbdata->attrib = attr;
	mSendChgNotif( cPositionChange(), cbdata->cbID().getI() );
    }

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


const TypeSet<PosID>* EMObject::getPosAttribList( int attr ) const
{
    const int idx=attribs_.indexOf( attr );
    return idx!=-1 ? &posattribs_[idx]->posids_ : 0;
}


const OD::MarkerStyle3D& EMObject::getPosAttrMarkerStyle( int attr )
{
    addPosAttrib( attr );
    return preferredMarkerStyle3D();
}


void EMObject::setPosAttrMarkerStyle( int attr, const OD::MarkerStyle3D& ms )
{
    mLock4Write();
    addPosAttrib( attr );
    setPreferredMarkerStyle3D( ms );

    EMObjectCallbackData* cbdata = getNewEMCBData();
    cbdata->event = EMObjectCallbackData::AttribChange;
    cbdata->attrib = attr;
    mSendChgNotif( cPositionChange(), cbdata->cbID().getI() );
    changed_ = true;
}


const OD::MarkerStyle3D& EMObject::preferredMarkerStyle3D() const
{
    return preferredmarkerstyle_;
}


void EMObject::setPreferredMarkerStyle3D( const OD::MarkerStyle3D& mkst )
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
			       TaskRunner* tskr )
{
    if ( !selector.isOK() )
	return;

    removebypolyposbox_.setEmpty();

    insideselremoval_ = true;
    for ( int idx=0; idx<nrSections(); idx++ )
    {
	Geometry::Element* ge = sectionGeometry( sectionID(idx) );
	if ( !ge ) continue;

	mDynamicCastGet(const Geometry::RowColSurface*,surface,ge);
	if ( !surface ) continue;

	ObjectSet<const Selector<Coord3> > selectors;
	selectors += &selector;
	EMObjectPosSelector posselector( *this, sectionID(idx), selectors );
	posselector.executeParallel( tskr );

	const TypeSet<EM::SubID>& list = posselector.getSelected();

	setBurstAlert( true );
	int poscount = 0;
	ge->blockCallBacks( true, false );
	for ( int sididx=0; sididx<list.size(); sididx++ )
	{
	    unSetPos( sectionID(idx), list[sididx], true );

	    BinID bid = BinID::fromInt64( list[sididx] );
	    const Coord3 pos = getPos( sectionID(idx), list[sididx] );
	    if ( removebypolyposbox_.isEmpty() )
	    {
		removebypolyposbox_.hsamp_.start_ =
			removebypolyposbox_.hsamp_.stop_ = bid;
		removebypolyposbox_.zsamp_.start =
		    removebypolyposbox_.zsamp_.stop = (float) pos.z_;
	    }
	    else
	    {
		removebypolyposbox_.hsamp_.include(bid);
		removebypolyposbox_.zsamp_.include((float) pos.z_);
	    }

	    if ( ++poscount >= 10000 )
	    {
		ge->blockCallBacks( true, true );
		poscount = 0;
	    }
	}
	ge->blockCallBacks( false, true );
	setBurstAlert( false );
    }

    insideselremoval_ = false;
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
	if ( pid.objectID()==-1 )
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
    return !iterator || iterator->next().objectID()==-1;
}


uiString EMObject::errMsg() const
{
    return errmsg_;
}


void EMObject::useDisplayPars( const IOPar& par )
{
    IOPar displaypar;

    if( !EMM().readDisplayPars(storageid_,displaypar) )
	displaypar = par;

    Color col;
    if( displaypar.get(sKey::Color(),col) )
    {
	col.setTransparency( 0 );
	setPreferredColor( col );
    }

    BufferString lnststr;
    OD::LineStyle lnst;
    if( displaypar.get(sKey::LineStyle(),lnststr) )
    {
	lnst.fromString( lnststr );
	setPreferredLineStyle( lnst );
    }

    BufferString mkststr;
    OD::MarkerStyle3D mkst;
    if( displaypar.get(sKey::MarkerStyle(),mkststr) )
    {
	const double versionnr =
	    displaypar.majorVersion()+displaypar.minorVersion()*0.1;
	mkst.fromString( mkststr, versionnr>0 && versionnr<=6.0 );
	setPreferredMarkerStyle3D( mkst );
    }
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
		mCast(EM::SectionID,sections[idy]), subids[idy] );
	    setPosAttrib( pid, attrib, true, false );
	}
    }

    return true;
}


void EMObject::saveDisplayPars() const
{
    IOPar displaypar;

    displaypar.set( sKey::Color(), preferredColor() );
    BufferString lnststr;

    preferredlinestyle_.toString( lnststr );
    displaypar.set( sKey::LineStyle(), lnststr );

    BufferString mkststr;
    preferredmarkerstyle_.toString( mkststr );
    displaypar.set( sKey::MarkerStyle(), mkststr );

    EMM().writeDisplayPars( storageid_, displaypar );
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


void EMObject::posIDChangeCB(CallBacker* cb)
{
    mCBCapsuleUnpack(const EMObjectCallbackData&,cbdata,cb);
    if ( cbdata.event != EMObjectCallbackData::PosIDChange )
	return;

    for ( int idx=0; idx<posattribs_.size(); idx++ )
    {
	TypeSet<PosID>& nodes = posattribs_[idx]->posids_;
	if ( !&nodes ) continue;

	while ( true )
	{
	    const int idy = nodes.indexOf( cbdata.pid0 );
	    if ( idy==-1 )
		break;

	    nodes[idy] = cbdata.pid1;
	}
    }
}


EMObjectCallbackData* EMObject::getNewEMCBData()
{
    EMObjectCallbackData* emcbdata = new EMObjectCallbackData();
    emcbdatas_ += emcbdata;
    return emcbdata;
}


const EMObjectCallbackData* EMObject::getEMCBData( EMCBID emcbid ) const
{
    if ( emcbdatas_.isEmpty() )
	return 0;

    mLock4Read();
    for ( int idx=0; idx<emcbdatas_.size(); idx++ )
    {
	const EMObjectCallbackData* emcbdata = emcbdatas_[idx];
	if ( emcbdata->cbID() == emcbid )
	    return emcbdata;
    }

    return 0;
}


} // namespace EM
