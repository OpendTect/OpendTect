/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : Apr 2002
-*/

static const char* rcsID = "$Id: emobject.cc,v 1.66 2006-08-17 13:56:47 cvsjaap Exp $";

#include "emobject.h"

#include "color.h"
#include "emhistoryimpl.h"
#include "emsurfacetr.h"
#include "emsticksettransl.h"
#include "emmanager.h"
#include "errh.h"
#include "geomelement.h"
#include "ioman.h"
#include "ioobj.h"
#include "iopar.h"
#include "ptrman.h"

using namespace EM;



int EMObject::sPermanentControlNode	= PosAttrib::PermanentControlNode;
int EMObject::sTemporaryControlNode	= PosAttrib::TemporaryControlNode;
int EMObject::sEdgeControlNode		= PosAttrib::EdgeControlNode;
int EMObject::sTerminationNode		= PosAttrib::TerminationNode;
int EMObject::sSeedNode			= PosAttrib::SeedNode;

const char* EMObject::prefcolorstr 	= "Color";
const char* EMObject::posattrprefixstr 	= "Pos Attrib ";
const char* EMObject::posattrsectionstr = " Section";
const char* EMObject::posattrposidstr 	= " SubID";
const char* EMObject::nrposattrstr 	= "Nr Pos Attribs";
const char* EMObject::markerstylestr	= " Marker Style";


ObjectFactory::ObjectFactory( EMObjectCreationFunc cf, 
			      const IOObjContext& ctx,
			      const char* tstr )
    : creationfunc( cf )
    , context( ctx )
    , typestr( tstr )
{}


EMObject* ObjectFactory::loadObject( const MultiID& mid ) const
{
    PtrMan<IOObj> ioobj = IOM().get( mid );

    if ( !ioobj || strcmp(ioobj->group(),typeStr()) )
	return 0;

    EMObject* emobj = creationfunc( EMM() );
    if ( !emobj ) return 0;

    emobj->setMultiID( ioobj->key() );
    return emobj;
}


EMObject* ObjectFactory::createObject( const char* name, bool tmpobj ) const
{
    if ( tmpobj )
	return creationfunc( EM::EMM() );

    CtxtIOObj ctio(context);
    ctio.ctxt.forread = false;
    ctio.ioobj = 0;
    ctio.setName( name );
    ctio.fillObj();
    if ( !ctio.ioobj ) return 0;

    EMObject* emobj = creationfunc( EMM() );
    if ( !emobj ) return 0;

    emobj->setMultiID( ctio.ioobj->key() );
    return emobj;
}


EMObject::EMObject( EMManager& emm )
    : manager( emm )
    , notifier( this )
    , id_( -1 )
    , preferredcolor( *new Color(255, 0, 0) )
    , changed( false )
    , storageid( -1 )
    , fullyloaded( false )
    , locked( false )
{
    mRefCountConstructor;
    id_ = manager.addObject( this );
    notifier.notify( mCB( this, EMObject, posIDChangeCB ) );
}


EMObject::~EMObject()
{
    manager.removeObject(this);
    deepErase( posattribs );
    delete &preferredcolor;

    notifier.remove( mCB( this, EMObject, posIDChangeCB ) );
    id_ = -2;	//To check easier if it has been deleted
}


BufferString EMObject::name() const
{
    PtrMan<IOObj> ioobj = IOM().get( multiID() );
    return BufferString( ioobj ? ioobj->name() : "" );
}


void EMObject::setMultiID( const MultiID& mid )
{ storageid = mid; }


int EMObject::sectionIndex( const SectionID& sid ) const
{
    for ( int idx=0; idx<nrSections(); idx++ )
	if ( sectionID(idx) == sid )
	    return idx;

    return -1;
}


BufferString EMObject::sectionName( const SectionID& sid ) const
{
    BufferString res = sid;
    return res;
}


bool EMObject::canSetSectionName() const
{ return false; }


bool EMObject::setSectionName( const SectionID&, const char*, bool )
{ return false; }


const Geometry::Element* EMObject::sectionGeometry( const SectionID& sec ) const
{ return const_cast<EMObject*>(this)->sectionGeometryInternal(sec); }


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


#define mRetErr( msg ) { errmsg = msg; return false; }

bool EMObject::setPos(	const PosID& pid, const Coord3& newpos,
			bool addtohistory ) 
{
    if ( pid.objectID()!=id() )
	mRetErr("");

    return setPos( pid.sectionID(), pid.subID(), newpos, addtohistory );
}


bool EMObject::setPos(	const SectionID& sid, const SubID& subid,
			const Coord3& newpos, bool addtohistory ) 
{
    Geometry::Element* element = sectionGeometryInternal( sid );
    if ( !element ) mRetErr( "" );

    const Coord3 oldpos = element->getPosition(subid);

     if ( !element->setPosition(subid, newpos) )
	 mRetErr(element->errMsg());

    const PosID pid (id(),sid,subid);

    if ( !newpos.isDefined() )
    {
	for ( int idx=0; idx<posattribs.size(); idx++ )
	{
	    TypeSet<PosID>& nodes = posattribs[idx]->posids_;
	    if ( !&nodes ) continue;

	    const int idy = nodes.indexOf(pid);
	    if ( idy!=-1 )
		setPosAttrib( pid, attribs[idx], false, addtohistory );
	}
    }

    if ( addtohistory )
    {
	HistoryEvent* history = new SetPosHistoryEvent( oldpos, pid );
	EMM().history().addEvent( history, 0, 0 );
    }

    EMObjectCallbackData cbdata;
    cbdata.event = EMObjectCallbackData::PositionChange;
    cbdata.pid0 = pid;
    notifier.trigger( cbdata );

    changed = true;
    return true;
}


bool EMObject::isAtEdge( const PosID& ) const
{
    pErrMsg("Not implemented");
    return false;
}


const Color& EMObject::preferredColor() const
{ return preferredcolor; }


void EMObject::setPreferredColor(const Color& col)
{
    if ( col==preferredcolor )
	return;

    preferredcolor = col;
    EMObjectCallbackData cbdata;
    cbdata.event = EMObjectCallbackData::PrefColorChange;
    notifier.trigger(cbdata);
}


bool EMObject::unSetPos(const PosID& pid, bool addtohistory )
{
    return setPos( pid, Coord3::udf(), addtohistory );
}


bool EMObject::unSetPos( const EM::SectionID& sid, const EM::SubID& subid,
			 bool addtohistory )
{
    return setPos( sid, subid, Coord3::udf(), addtohistory );
}


bool EMObject::enableGeometryChecks(bool)
{ return true; }


bool EMObject::isGeometryChecksEnabled() const
{ return true; }


void EMObject::changePosID( const PosID& from, const PosID& to,
			    bool addtohistory )
{
    if ( from==to )
    {
	pErrMsg("From and to are identical");
	return;
    }

    if ( from.objectID()!=id() || to.objectID()!=id() )
	return;

    const Coord3 tosprevpos = getPos( to );
    setPos( to, getPos( from ), false );

    if ( addtohistory )
    {
	PosIDChangeEvent* event = new PosIDChangeEvent( from, to, tosprevpos );
	EMM().history().addEvent( event, 0, 0 );
    }

    EMObjectCallbackData cbdata;
    cbdata.event = EMObjectCallbackData::PosIDChange;
    cbdata.pid0 = from;
    cbdata.pid1 = to;
    notifier.trigger(cbdata);

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
    if ( attribs.indexOf(attr) < 0 )
    {
	attribs += attr;
	posattribs += new PosAttrib();
	const int idx = attribs.indexOf(attr);
	posattribs[idx]->type_ = (PosAttrib::Type) attr;
    }
}    


void EMObject::removePosAttribList( int attr, bool addtohistory )
{
    const int idx=attribs.indexOf(attr);
    if ( idx==-1 )
	return;

    const TypeSet<PosID>& attrlist = posattribs[idx]->posids_;

    while ( attrlist.size() ) 
	setPosAttrib( attrlist[0], attr, false, addtohistory );
}


void EMObject::setPosAttrib( const PosID& pid, int attr, bool yn,
			     bool addtohistory )
{
    EMObjectCallbackData cbdata;
    cbdata.event = EMObjectCallbackData::AttribChange;
    cbdata.pid0 = pid;
    cbdata.attrib = attr;

    if ( yn )
	addPosAttrib( attr );

    const int idx = attribs.indexOf(attr);
    
    if ( idx != -1 )
    {
	TypeSet<PosID>& posids = posattribs[idx]->posids_;
	const int idy=posids.indexOf(pid);

	if ( idy==-1 )
	{
	    if ( yn ) posids += pid;
	}
	else if ( !yn )
	{
	    posids.removeFast(idy);
	}
    }

    if ( addtohistory )
    {
	HistoryEvent* event = new SetPosAttribHistoryEvent( pid, attr, yn );
	EMM().history().addEvent( event, 0, 0 );
    }


    notifier.trigger( cbdata );
    changed = true;
}


bool EMObject::isPosAttrib( const PosID& pid, int attr ) const
{
    const int idx = attribs.indexOf( attr );
    return idx != -1 && posattribs[idx]->posids_.indexOf( pid ) != -1;
}


const char* EMObject::posAttribName( int idx ) const
{
    return 0;
}


int EMObject::nrPosAttribs() const
{ return attribs.size(); }


int EMObject::posAttrib(int idx) const
{ return attribs[idx]; }


int EMObject::addPosAttribName( const char* nm )
{
    return -1;
}


const TypeSet<PosID>* EMObject::getPosAttribList( int attr ) const
{
    const int idx=attribs.indexOf(attr);
    return idx!=-1 ? &posattribs[idx]->posids_ : 0;
}


const MarkerStyle3D& EMObject::getPosAttrMarkerStyle( int attr ) 
{
    addPosAttrib( attr );
    const int idx=attribs.indexOf(attr);
    return posattribs[idx]->style_;
}


void EMObject::setPosAttrMarkerStyle( int attr, const MarkerStyle3D& ms ) 
{
    addPosAttrib( attr );
    const int idx=attribs.indexOf(attr);
    posattribs[idx]->style_ = ms;
    
    EMObjectCallbackData cbdata;
    cbdata.event = EMObjectCallbackData::AttribChange;
    cbdata.attrib = attr;
    notifier.trigger( cbdata );
    changed = true;
}


bool EMObject::isEmpty() const
{
    PtrMan<EM::EMObjectIterator> iterator = createIterator(-1);
    return iterator->next().objectID()==-1;
}


bool EMObject::usePar( const IOPar& par )
{
    int col;
    if ( par.get( prefcolorstr, col ) )
    {
	Color newcol; newcol.setRgb(col);
	setPreferredColor(newcol);
    }

    for ( int idx=0; idx<nrPosAttribs(); idx++ )
	removePosAttribList( posAttrib(idx), false );

    int nrattribs = 0;
    par.get( nrposattrstr, nrattribs );
    for ( int idx=0; idx<nrattribs; idx++ )
    {
	BufferString attribkey = posattrprefixstr;
	attribkey += idx;

	int attrib;
	if ( !par.get( attribkey, attrib ) )
	    continue;

	TypeSet<int> sections;
	TypeSet<SubID> subids;

	BufferString sectionkey = attribkey;
	sectionkey += posattrsectionstr;

	BufferString subidkey = attribkey;
	subidkey += posattrposidstr;

	par.get( sectionkey, sections );
	par.get( subidkey, subids );

	const int minsz = mMIN(sections.size(), subids.size() );

	for ( int idy=0; idy<minsz; idy++ )
	{
	    if ( !isDefined(sections[idy],subids[idy]) )
		continue;
	    const PosID pid = PosID( id(), sections[idy], subids[idy] );
	    setPosAttrib( pid, attrib, true, false );
	}
	
	BufferString markerstylekey = attribkey;
	markerstylekey += markerstylestr;
	BufferString markerstyleparstr;
	if ( par.get(markerstylekey, markerstyleparstr) )
	    posattribs[idx]->style_.fromString( markerstyleparstr );
    }

    return true;
}


void EMObject::fillPar( IOPar& par ) const
{
    par.set( prefcolorstr, (int) preferredColor().rgb() );

    int keyid = 0;
    for ( int idx=0; idx<nrPosAttribs(); idx++ )
    {
	const int attrib = posAttrib(idx);
	const TypeSet<PosID>* pids = getPosAttribList(attrib);
	if ( !pids ) continue;

	BufferString attribkey = posattrprefixstr;
	attribkey += keyid++;
	par.set( attribkey, attrib );

	TypeSet<int> attrpatches;
	TypeSet<SubID> subids;
	for ( int idy=0; idy<pids->size(); idy++ )
	{
	    attrpatches += (*pids)[idy].sectionID();
	    subids += (*pids)[idy].subID();
	}

	BufferString patchkey = attribkey;
	patchkey += posattrsectionstr;
	BufferString subidkey = attribkey;
	subidkey += posattrposidstr;

	par.set( patchkey, attrpatches );
	par.set( subidkey, subids );
	
	BufferString markerstylekey = attribkey;
	markerstylekey += markerstylestr;
	BufferString markerstyleparstr;
	posattribs[idx]->style_.toString( markerstyleparstr );
	par.set( markerstylekey, markerstyleparstr );
    }

    par.set( nrposattrstr, keyid );
}


void EMObject::posIDChangeCB(CallBacker* cb)
{
    mCBCapsuleUnpack(const EMObjectCallbackData&,cbdata,cb);
    if ( cbdata.event!=EMObjectCallbackData::PosIDChange )
	return;

    for ( int idx=0; idx<posattribs.size(); idx++ )
    {
	TypeSet<PosID>& nodes = posattribs[idx]->posids_;
	if ( !&nodes ) continue;

	while ( true )
	{
	    int idy = nodes.indexOf(cbdata.pid0);
	    if ( idy==-1 )
		break;

	    nodes[idy] = cbdata.pid1;
	}
    }
}
