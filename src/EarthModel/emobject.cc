/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : Apr 2002
-*/

static const char* rcsID = "$Id: emobject.cc,v 1.36 2005-01-06 09:39:57 kristofer Exp $";

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



int EM::EMObject::sPermanentControlNode	= 0;
int EM::EMObject::sTemporaryControlNode	= 1;
int EM::EMObject::sEdgeControlNode	= 2;
int EM::EMObject::sTerminationNode	= 3;


const char* EM::EMObject::prefcolorstr = "Color";
const char* EM::EMObject::posattrprefixstr = "Pos Attrib ";
const char* EM::EMObject::posattrsectionstr = " Section";
const char* EM::EMObject::posattrposidstr = " SubID";
const char* EM::EMObject::nrposattrstr = "Nr Pos Attribs";


EM::ObjectFactory::ObjectFactory( EMObjectCreationFunc cf, 
				  const IOObjContext& ctx,
				  const char* tstr )
    : creationfunc( cf )
    , context( ctx )
    , typestr( tstr )
{}


EM::EMObject* EM::ObjectFactory::create( const char* name, bool tmpobj,
					 EMManager& emm )
{
    if ( tmpobj )
	return creationfunc( -1, emm );

    if ( !IOM().to(IOObjContext::getStdDirData(context.stdseltype)->id) )
	return 0;

    PtrMan<IOObj> ioobj = IOM().getLocal( name );
    IOM().back();
    if ( ioobj )
	return creationfunc( emm.multiID2ObjectID(ioobj->key()), emm );

     CtxtIOObj ctio(context);
     ctio.ctxt.forread = false;
     ctio.ioobj = 0;
     ctio.setName( name );
     ctio.fillObj();
     if ( !ctio.ioobj ) return 0;

     return creationfunc( emm.multiID2ObjectID(ctio.ioobj->key()), emm );
}


void EM::EMObject::ref() const
{
    const_cast<EM::EMObject*>(this)->manager.ref(id());
}


void EM::EMObject::unRef() const
{
    const_cast<EM::EMObject*>(this)->manager.unRef(id());
}


void EM::EMObject::unRefNoDel() const
{
    const_cast<EM::EMObject*>(this)->manager.unRefNoDel(id());
}


EM::EMObject::EMObject( EMManager& emm_, const EM::ObjectID& id__ )
    : manager(emm_)
    , notifier(this)
    , id_(id__)
    , preferredcolor( *new Color(255, 0, 0) )
{
    notifier.notify( mCB( this, EMObject, posIDChangeCB ) );
}


EM::EMObject::~EMObject()
{
    deepErase( posattribs );
    delete &preferredcolor;

    notifier.remove( mCB( this, EMObject, posIDChangeCB ) );
}


BufferString EM::EMObject::name() const
{
    PtrMan<IOObj> ioobj = IOM().get( multiID() );
    return ioobj ? ioobj->name() : BufferString("");
}


const Geometry::Element* EM::EMObject::getElement( SectionID sec ) const
{ return 0; }


Coord3 EM::EMObject::getPos( const EM::PosID& pid ) const
{
    if ( pid.objectID()!=id() )
	return  Coord3::udf();

    const Geometry::Element* element = getElement( pid.sectionID() );
    return element ? element->getPosition( pid.subID() ) : Coord3::udf();
}

#define mRetErr( msg ) { errmsg = msg; return false; }

bool EM::EMObject::setPos( const EM::PosID& pid, const Coord3& newpos,
			     bool addtohistory ) 
{
    if ( pid.objectID()!=id() )
	mRetErr("");

    Geometry::Element* element = getElement( pid.sectionID() );
    if ( !element ) mRetErr( "" );

    HistoryEvent* history =  addtohistory
	? new SetPosHistoryEvent(element->getPosition(pid.subID()),pid)
	: 0;

     if ( !element->setPosition(pid.subID(), newpos ) )
     {
	 delete history;
	 mRetErr(element->errMsg());
     }

     if ( history ) EM::EMM().history().addEvent( history, 0, 0 );

     return true;
}


const Color& EM::EMObject::preferredColor() const
{ return preferredcolor; }


void EM::EMObject::setPreferredColor(const Color& col)
{
    if ( col==preferredcolor )
	return;

    preferredcolor = col;
    EMObjectCallbackData cbdata;
    cbdata.event = EMObjectCallbackData::PrefColorChange;
    notifier.trigger(cbdata);
}


bool EM::EMObject::unSetPos(const EM::PosID& pid, bool addtohistory )
{
    return setPos( pid, Coord3::udf(), addtohistory );
}


void EM::EMObject::changePosID( const EM::PosID& from, const EM::PosID& to,
				bool addtohistory )
{
    if ( from.objectID()!=id() || to.objectID()!=id() )
	return;

    const Coord3 tosprevpos = getPos( to );
    setPos( to, getPos( from ), false );
    unSetPos( from, false );

    if ( addtohistory )
    {
	SurfacePosIDChangeEvent* event = new SurfacePosIDChangeEvent( from, to,
						    tosprevpos );
	EM::EMM().history().addEvent( event, 0, 0 );
    }

    EMObjectCallbackData cbdata;
    cbdata.event = EMObjectCallbackData::PosIDChange;
    cbdata.pid0 = from;
    cbdata.pid1 = to;
    notifier.trigger(cbdata);
}


bool EM::EMObject::isDefined( const EM::PosID& pid ) const
{ 
    if ( pid.objectID()!=id() )
	return  false;

    const Geometry::Element* element = getElement( pid.sectionID() );
    return element && element->isDefined( pid.subID() );
}


MultiID EM::EMObject::multiID() const
{
    MultiID res = getIOObjContext().stdSelKey();
    res.add(id());
    return res;
}


void EM::EMObject:: removePosAttrib(int attr)
{
    const int idx=attribs.indexOf(attr);
    if ( idx==-1 )
	return;

    posattribs[idx]->erase();
}


void EM::EMObject::setPosAttrib( const EM::PosID& pid, int attr, bool yn )
{
    EMObjectCallbackData cbdata;
    cbdata.event = EMObjectCallbackData::AttribChange;
    cbdata.pid0 = pid;
    cbdata.attrib = attr;

    const int idx=attribs.indexOf(attr);
    if ( idx==-1 )
    {
	if ( yn )
	{
	    attribs += attr;
	    posattribs += new TypeSet<EM::PosID>(1,pid);
	}
    }
    else
    {
	TypeSet<EM::PosID>& posids = *posattribs[idx];
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

    notifier.trigger( cbdata );
}


bool EM::EMObject::isPosAttrib( const EM::PosID& pid, int attr ) const
{
    const int idx=attribs.indexOf(attr);
    if ( idx==-1 )
	return false;

    TypeSet<EM::PosID>& posids = *posattribs[idx];
    const int idy=posids.indexOf(pid);

    if ( idy==-1 )
	return false;

    return true;
}


const char* EM::EMObject::posAttribName( int idx ) const
{
    return 0;
}


int EM::EMObject::nrPosAttribs() const
{ return attribs.size(); }


int EM::EMObject::posAttrib(int idx) const
{ return attribs[idx]; }


int EM::EMObject::addPosAttribName( const char* name )
{
    return -1;
}


const TypeSet<EM::PosID>* EM::EMObject::getPosAttribList(int attr) const
{
    const int idx=attribs.indexOf(attr);
    return idx!=-1 ? posattribs[idx] : 0;
}


bool EM::EMObject::usePar( const IOPar& par )
{
    int col;
    if ( par.get( prefcolorstr, col ) )
    {
	Color newcol; newcol.setRgb(col);
	setPreferredColor(newcol);
    }

    for ( int idx=0; idx<nrPosAttribs(); idx++ )
	removePosAttrib(posAttrib(idx));

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
	    setPosAttrib( PosID(id(),sections[idy],subids[idy]), attrib, true );
    }

    return true;
}


void EM::EMObject::fillPar( IOPar& par ) const
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
    }

    par.set( nrposattrstr, keyid );
}


void EM::EMObject::posIDChangeCB(CallBacker* cb)
{
    mCBCapsuleUnpack(const EM::EMObjectCallbackData&,cbdata,cb);
    if ( cbdata.event!=EMObjectCallbackData::PosIDChange )
	return;

    for ( int idx=0; idx<posattribs.size(); idx++ )
    {
	TypeSet<PosID>& nodes = *posattribs[idx];
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
