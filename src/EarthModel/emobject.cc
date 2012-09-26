/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : Apr 2002
-*/

static const char* rcsID mUsedVar = "$Id$";

#include "emobject.h"

#include "color.h"
#include "emundo.h"
#include "emsurfacetr.h"
#include "emmanager.h"
#include "emobjectselremoval.h"
#include "errh.h"
#include "geomelement.h"
#include "ioman.h"
#include "ioobj.h"
#include "iopar.h"
#include "rowcolsurface.h"
#include "ptrman.h"
#include "selector.h"
#include "survinfo.h"

using namespace EM;



int EMObject::sPermanentControlNode() {return PosAttrib::PermanentControlNode;}
int EMObject::sTemporaryControlNode() {return PosAttrib::TemporaryControlNode;}
int EMObject::sEdgeControlNode()	{ return PosAttrib::EdgeControlNode; }
int EMObject::sTerminationNode()	{ return PosAttrib::TerminationNode; }
int EMObject::sSeedNode()		{ return PosAttrib::SeedNode; }

const char* EMObject::prefcolorstr() 	    { return "Color"; }
const char* EMObject::posattrprefixstr()    { return "Pos Attrib "; }
const char* EMObject::posattrsectionstr()   { return " Section"; }
const char* EMObject::posattrposidstr()	    { return " SubID"; }
const char* EMObject::nrposattrstr()	    { return "Nr Pos Attribs"; }
const char* EMObject::markerstylestr()	    { return " Marker Style"; }

EMObject::EMObject( EMManager& emm )
    : manager_( emm )
    , change( this )
    , id_( -1 )
    , preferredcolor_( *new Color(255,0,0) )
    , changed_( false )
    , fullyloaded_( false )
    , locked_( false )
    , burstalertcount_( 0 )
    , insideselremoval_( false )
    , selremoving_( false )
{
    static EM::ObjectID oid = 0;
    id_ = oid++;

    removebypolyposbox_.setEmpty();

    change.notify( mCB(this,EMObject,posIDChangeCB) );
}


EMObject::~EMObject()
{
    deepErase( posattribs_ );
    delete &preferredcolor_;

    change.remove( mCB(this,EMObject,posIDChangeCB) );
    id_ = -2;	//To check easier if it has been deleted
}


void EMObject::prepareForDelete() const
{
    manager_.removeObject( this );
}


BufferString EMObject::name() const
{
    PtrMan<IOObj> ioobj = IOM().get( multiID() );
    return ioobj ? BufferString(ioobj->name()) : objname_;
}


void EMObject::setNewName()
{ setName("<New EM Object>"); }

void EMObject::setMultiID( const MultiID& mid )
{ storageid_ = mid; }


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
			bool addtoundo ) 
{
    if ( pid.objectID()!=id() )
	mRetErr("");

    return setPos( pid.sectionID(), pid.subID(), newpos, addtoundo );
}


bool EMObject::setPos(	const SectionID& sid, const SubID& subid,
			const Coord3& newpos, bool addtoundo ) 
{
    Geometry::Element* element = sectionGeometryInternal( sid );
    if ( !element ) mRetErr( "" );

    const Coord3 oldpos = element->getPosition( subid );

    if ( !element->setPosition(subid,newpos) )
	 mRetErr( element->errMsg() );

    const PosID pid (id(), sid, subid );

    if ( !newpos.isDefined() )
    {
	for ( int idx=0; idx<posattribs_.size(); idx++ )
	{
	    TypeSet<PosID>& nodes = posattribs_[idx]->posids_;
	    if ( !&nodes ) continue;

	    const int idy = nodes.indexOf(pid);
	    if ( idy!=-1 )
		setPosAttrib( pid, attribs_[idx], false, addtoundo );
	}
    }

    if ( addtoundo )
    {
	UndoEvent* undo = new SetPosUndoEvent( oldpos, pid );
	EMM().undo().addEvent( undo, 0 );
    }

    if ( !burstalertcount_ )
    {
	EMObjectCallbackData cbdata;
	cbdata.event = EMObjectCallbackData::PositionChange;
	cbdata.pid0 = pid;
	change.trigger( cbdata );
    }

    changed_ = true;
    return true;
}


bool EMObject::isAtEdge( const PosID& ) const
{
    pErrMsg("Not implemented");
    return false;
}


const Color& EMObject::preferredColor() const
{ return preferredcolor_; }


void EMObject::setPreferredColor( const Color& col, bool addtoundo )
{
    if ( col==preferredcolor_ )
	return;

    if ( addtoundo )
    {
	UndoEvent* undo = new SetPrefColorEvent( id(), preferredcolor_, col );
	EMM().undo().addEvent( undo );
    }

    changed_ = true;
    preferredcolor_ = col;
    EMObjectCallbackData cbdata;
    cbdata.event = EMObjectCallbackData::PrefColorChange;
    change.trigger( cbdata );
}


void EMObject::setBurstAlert( bool yn )
{
    if ( !yn ) burstalertcount_--;
    if ( !burstalertcount_ )
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
	EMM().undo().addEvent( event, 0 );
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
	posids.remove( idy, false );
    else 
	return;

    if ( addtoundo )
    {
	UndoEvent* event = new SetPosAttribUndoEvent( pid, attr, yn );
	EMM().undo().addEvent( event, 0 );
    }

    change.trigger( cbdata );
    changed_ = true;
}


bool EMObject::isPosAttrib( const PosID& pid, int attr ) const
{
    const int idx = attribs_.indexOf( attr );
    return idx != -1 && posattribs_[idx]->posids_.indexOf( pid ) != -1;
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


const MarkerStyle3D& EMObject::getPosAttrMarkerStyle( int attr ) 
{
    addPosAttrib( attr );
    const int idx=attribs_.indexOf( attr );
    return posattribs_[idx]->style_;
}


void EMObject::setPosAttrMarkerStyle( int attr, const MarkerStyle3D& ms ) 
{
    addPosAttrib( attr );
    const int idx=attribs_.indexOf( attr );
    posattribs_[idx]->style_ = ms;
    
    EMObjectCallbackData cbdata;
    cbdata.event = EMObjectCallbackData::AttribChange;
    cbdata.attrib = attr;
    change.trigger( cbdata );
    changed_ = true;
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

    removebypolyposbox_.setEmpty();

    insideselremoval_ = true;
//#ifdef    
    for ( int idx=0; idx<nrSections(); idx++ )
    {
	Geometry::Element* ge = sectionGeometry( sectionID(idx) );
	if ( !ge ) continue;

	mDynamicCastGet(const Geometry::RowColSurface*,surface,ge);
	if ( !surface ) continue;

	int startrow = surface->rowRange().start;
	int nrrows = surface->rowRange().nrSteps() +1 ;
	int startcol = surface->colRange().start;
	int nrcols = surface->colRange().nrSteps() + 1 ;

	EMObjectRowColSelRemoval selremoval( *this, sectionID(idx), selector,
					     nrrows, nrcols, 
					     startrow, startcol );
	selremoval.execute( tr );

	TypeSet<EM::SubID> removallist = selremoval.getRemovelList();

	setBurstAlert( true );
	int poscount = 0;
	ge->blockCallBacks( true, false );
	for ( int sididx = 0; sididx < removallist.size(); sididx++ )
	{
	    unSetPos( sectionID(idx), removallist[sididx], true );

	    BinID bid = BinID::fromInt64( removallist[sididx] );
	    const Coord3 pos = getPos( sectionID(idx), removallist[sididx] );
	    if ( removebypolyposbox_.isEmpty() )
	    {
		removebypolyposbox_.hrg.start = removebypolyposbox_.hrg.stop
		    			      = bid;
		removebypolyposbox_.zrg.start = removebypolyposbox_.zrg.stop
		    			      = (float) pos.z;
	    }
	    else
	    {
		removebypolyposbox_.hrg.include(bid);
		removebypolyposbox_.zrg.include((float) pos.z);
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

/*#else
    PtrMan<EM::EMObjectIterator> iterator = createIterator( -1 );
    while ( true )
    {
	const EM::PosID pid = iterator->next();
	if ( pid.objectID()==-1 )
	    break;

	const Coord3 pos = getPos(pid);
	if ( selector.includes(pos) )
	{
	    unSetPos( pid, true );
	    if ( !selremoving_ )
		selremoving_ = true;
	    
	    dummypid = pid;

	    if ( !pos.isDefined() ||
		 isPosAttrib(pid, EM::EMObject::sSeedNode()) )
		continue;

	    const BinID bid = SI().transform(pos);
	    if ( removebypolyposbox_.isEmpty() )
	    {
		removebypolyposbox_.hrg.start = removebypolyposbox_.hrg.stop
		    			      = bid;
		removebypolyposbox_.zrg.start = removebypolyposbox_.zrg.stop
		    			      = pos.z;
	    }
	    else
	    {
		removebypolyposbox_.hrg.include(bid);
		removebypolyposbox_.zrg.include(pos.z);
	    }
	}
    }
#endif */
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

	if ( !isPosAttrib(pid, EM::EMObject::sSeedNode()) )
	    unSetPos( pid, true );
    }
    setBurstAlert( false );
}


const CubeSampling EMObject::getRemovedPolySelectedPosBox()
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


const char* EMObject::errMsg() const
{
    return errmsg_.str();
}


bool EMObject::usePar( const IOPar& par )
{
    int col;
    if ( par.get(prefcolorstr(),col) )
    {
	Color newcol; newcol.setRgb(col);
	newcol.setTransparency( 0 );
	setPreferredColor( newcol );
    }

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
	    if ( !isDefined(sections[idy],subids[idy]) )
		continue;
	    const PosID pid = PosID( id(), sections[idy], subids[idy] );
	    setPosAttrib( pid, attrib, true, false );
	}

	const int curposattridx = posattribs_.size()-1;
	if ( curposattridx<0 )
	    continue;

	BufferString markerstylekey = attribkey;
	markerstylekey += markerstylestr();
	BufferString markerstyleparstr;
	if ( par.get(markerstylekey.buf(),markerstyleparstr) )
	    posattribs_[curposattridx]->style_.fromString(
		    			markerstyleparstr.buf() );
    }

    return true;
}


void EMObject::fillPar( IOPar& par ) const
{
    par.set( prefcolorstr(), (int)preferredColor().rgb() );

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
	
	BufferString markerstylekey = attribkey;
	markerstylekey += markerstylestr();
	BufferString markerstyleparstr;
	posattribs_[idx]->style_.toString( markerstyleparstr );
	par.set( markerstylekey.buf(), markerstyleparstr.buf() );
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
