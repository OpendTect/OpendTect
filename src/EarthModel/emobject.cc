/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : Apr 2002
-*/

static const char* rcsID = "$Id: emobject.cc,v 1.33 2004-09-14 14:54:19 nanne Exp $";

#include "emobject.h"

#include "color.h"
#include "emsurfacetr.h"
#include "emsticksettransl.h"
#include "emmanager.h"
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


EM::EMObject* EM::EMObject::create( const IOObj& ioobj, EM::EMManager& manager )
{
    EM::EMObject* res = 0;
    const char* group = ioobj.group();

    const EM::ObjectID id = EM::EMManager::multiID2ObjectID(ioobj.key());

    if ( !strcmp( group, EMHorizonTranslatorGroup::keyword ))
	res = new EM::Horizon( manager, id );
    else if ( !strcmp( group, EMFaultTranslatorGroup::keyword ))
	res = new EM::Fault( manager, id );
    else if ( !strcmp( group, EMStickSetTranslatorGroup::keyword ))
	res = new EM::StickSet( manager, id );

    return res;
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
    , poschnotifier(this)
    , removenotifier(this)
    , id_(id__)
    , prefColorChange(this)
    , preferredcolor( *new Color(255, 0, 0) )
{
    posattrchnotifiers.allowNull();
}


EM::EMObject::~EMObject()
{
    deepErase( posattribs );
    deepErase( posattrchnotifiers );
    delete &preferredcolor;
}


BufferString EM::EMObject::name() const
{
    PtrMan<IOObj> ioobj = IOM().get( multiID() );
    return ioobj ? ioobj->name() : BufferString("");
}


const Color& EM::EMObject::preferredColor() const
{ return preferredcolor; }


void EM::EMObject::setPreferredColor(const Color& col)
{
    if ( col==preferredcolor )
	return;

    preferredcolor = col;
    prefColorChange.trigger();
}


bool EM::EMObject::unSetPos(const EM::PosID& pid, bool addtohistory )
{
    return setPos( pid, Coord3::udf(), addtohistory );
}


bool EM::EMObject::isDefined( const EM::PosID& pid ) const
{ return getPos(pid).isDefined(); }


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
    CNotifier<EMObject, PosID>* notifier = getPosAttribChNotifier(attr,false);

    const int idx=attribs.indexOf(attr);
    if ( idx==-1 )
    {
	if ( yn )
	{
	    attribs += attr;
	    posattribs += new TypeSet<EM::PosID>(1,pid);
	    posattrchnotifiers += 0;
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
	    const EM::PosID pidcopy = pid;
	    posids.removeFast(idy);
	    if ( notifier ) notifier->trigger( pidcopy, this );
	    return;
	}
    }

    if ( notifier ) notifier->trigger( pid, this );
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


CNotifier<EM::EMObject, EM::PosID>*
EM::EMObject::getPosAttribChNotifier( int attr, bool create )
{
    int idx=attribs.indexOf(attr);
    if ( idx==-1 )
    {
	if ( !create ) return 0;

	idx = attribs.size();
	attribs += attr;
	posattribs += new TypeSet<EM::PosID>;
	posattrchnotifiers += 0;
    }

    if ( !posattrchnotifiers[idx] && create )
	posattrchnotifiers.replace( new CNotifier<EMObject, PosID>(this),idx );

    return posattrchnotifiers[idx];
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
	TypeSet<long long> subids;

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
	TypeSet<long long> subids;
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
