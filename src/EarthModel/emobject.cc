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

int Object::sTerminationNode()	{ return PosAttrib::TerminationNode; }
int Object::sSeedNode()		{ return PosAttrib::SeedNode; }
int Object::sIntersectionNode()	{ return PosAttrib::IntersectionNode; }

const char* Object::posattrprefixstr()    { return "Pos Attrib "; }
const char* Object::posattrsectionstr()   { return " Section"; }
const char* Object::posattrposidstr()	    { return " PosID"; }
const char* Object::nrposattrstr()	    { return "Nr Pos Attribs"; }

Color Object::sDefaultSelectionColor()	{ return Color::Orange(); }
Color Object::sDefaultLockColor()		{ return Color::Blue(); }

Object::Object( const char* nm )
    : SharedObject( nm )
    , preferredcolor_(Color::Green())
    , changed_( false )
    , fullyloaded_( false )
    , locked_( false )
    , burstalertcount_( 0 )
    , selremoving_( false )
    , lockcolor_(sDefaultLockColor())
    , preferredlinestyle_(OD::LineStyle::Solid,3)
    , preferredmarkerstyle_(OD::MarkerStyle3D::Cube,2,Color::White())
    , selectioncolor_(sDefaultSelectionColor())
    , haslockednodes_( false )
{
    mDefineStaticLocalObject( Threads::Atomic<int>, oid, (0) );

    removebypolyposbox_.setEmpty();

    objectChanged().notify( mCB(this,Object,posIDChangeCB) );
}

Object::Object( const Object& oth )
{
   copyClassData( oth );
}

mImplMonitorableAssignment(Object,SharedObject);

Object::~Object()
{
    deepErase( posattribs_ );
    objectChanged().remove( mCB(this,Object,posIDChangeCB) );
}


void Object::copyClassData( const Object& oth )
{
#define mCopyMember( mem ) mem = oth.mem
    mCopyMember( objname_ );
    mCopyMember( storageid_ );
    mCopyMember( preferredcolor_ );
    mCopyMember( selectioncolor_ );
    mCopyMember( preferredlinestyle_ );
    mCopyMember( preferredmarkerstyle_ );
    mCopyMember( attribs_ );

    deepCopy<PosAttrib,PosAttrib>( posattribs_, oth.posattribs_ );
}


Monitorable::ChangeType Object::compareClassData( const Object& oth ) const
{
    return cNoChange();
}


void Object::prepareForDelete()
{
}


void Object::setNameToJustCreated()
{
    setName( "<New EM Object>" );
}

void Object::setDBKey( const DBKey& mid )
{
    storageid_ = mid;
    PtrMan<IOObj> ioobj = storageid_.getIOObj();
    if ( ioobj )
	name_ = ioobj->name();
}


const Geometry::Element* Object::geometryElement() const
{ return 0; }

Geometry::Element* Object::geometryElement()
{ return 0; }


Coord3 Object::getPos( const EM::PosID& posid ) const
{
    const Geometry::Element* element = geometryElement();
    return element ? element->getPosition( posid ) : Coord3::udf();
}


#define mRetErr( msg ) { errmsg_ = msg; return false; }

bool Object::setPos(	const PosID& pid, const Coord3& newpos,
			bool addtoundo, NodeSourceType tp )
{
    return setPosition( pid, newpos, addtoundo, tp );
}


bool Object::setPosition( const PosID& posid,
			    const Coord3& newpos, bool addtoundo,
			    NodeSourceType tp )
{
    mLock4Write();

    Geometry::Element* element = geometryElement();
    if ( !element ) mRetErr( uiString::empty() );

    Coord3 oldpos = Coord3::udf();
    if ( addtoundo )
	oldpos = element->getPosition( posid );

    if ( !element->setPosition(posid,newpos) )
	 mRetErr( element->errMsg() );

    if ( !newpos.isDefined() )
    {
	for ( int idx=0; idx<posattribs_.size(); idx++ )
	{
	    TypeSet<PosID>* nodes = &posattribs_[idx]->posids_;
	    if ( !nodes ) continue;

	    if ( nodes->isPresent(posid) )
		setPosAttrib( posid, attribs_[idx], false, addtoundo );
	}
    }

    if ( burstalertcount_==0 )
    {
	mSendEMCBNotifPosID( cPositionChange(), posid );
    }

    changed_ = true;
    return true;
}


bool Object::isAtEdge( const PosID& ) const
{
    pErrMsg("Not implemented");
    return false;
}


void Object::setBurstAlert( bool yn )
{
    mLock4Write();

    if ( !yn && burstalertcount_==0 )
	return;

    if ( !yn )
	burstalertcount_--;

    if ( burstalertcount_==0 )
    {
	if ( yn ) burstalertcount_++;
	RefMan<ChangeAuxData> data = new ChangeAuxData;
	data->flagfor2dviewer = !yn;
	mSendEMCBNotifWithData( cBurstAlert(), data );
    }
    else if ( yn )
	burstalertcount_++;
}


bool Object::hasBurstAlert() const
{ return burstalertcount_>0; }


bool Object::unSetPos(const PosID& pid, bool addtoundo )
{
    return setPos( pid, Coord3::udf(), addtoundo );
}


bool Object::enableGeometryChecks( bool )
{ return true; }


bool Object::isGeometryChecksEnabled() const
{ return true; }


bool Object::isDefined( const EM::PosID& posid ) const
{
    const Geometry::Element* element = geometryElement();
    return element && element->isDefined( posid );
}


void Object::addPosAttrib( int attr )
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


void Object::removePosAttribList( int attr, bool addtoundo )
{
    const int idx=attribs_.indexOf( attr );
    if ( idx==-1 )
	return;

    const TypeSet<PosID>& attrlist = posattribs_[idx]->posids_;

    while ( attrlist.size() )
	setPosAttrib( attrlist[0], attr, false, addtoundo );
}


void Object::setPosAttrib( const PosID& pid, int attr, bool yn,
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

    if ( !hasBurstAlert() )
    {
	RefMan<ChangeAuxData> data = new ChangeAuxData;
	data->attrib = attr;
	data->pid0 = pid;
	mSendEMCBNotifWithData( cAttribChange(), data );
    }

    changed_ = true;
}


bool Object::isPosAttrib( const PosID& pid, int attr ) const
{
    const int idx = attribs_.indexOf( attr );
    return idx != -1 && posattribs_[idx]->posids_.isPresent( pid );
}


const char* Object::posAttribName( int idx ) const
{ return 0; }

int Object::nrPosAttribs() const
{ return attribs_.size(); }

int Object::posAttrib(int idx) const
{ return attribs_[idx]; }

int Object::addPosAttribName( const char* nm )
{ return -1; }


const TypeSet<PosID>* Object::getPosAttribList( int attr ) const
{
    const int idx=attribs_.indexOf( attr );
    return idx!=-1 ? &posattribs_[idx]->posids_ : 0;
}


OD::MarkerStyle3D Object::getPosAttrMarkerStyle( int attr )
{
    addPosAttrib( attr );
    return preferredMarkerStyle3D();
}


void Object::setPosAttrMarkerStyle( int attr, const OD::MarkerStyle3D& ms )
{
    addPosAttrib( attr );
    setPreferredMarkerStyle3D( ms );
}


void Object::lockPosAttrib( int attr, bool yn )
{
    addPosAttrib( attr );
    const int idx=attribs_.indexOf( attr );
    posattribs_[idx]->locked_ = yn;
}


bool Object::isPosAttribLocked( int attr ) const
{
    const int idx=attribs_.indexOf( attr );
    return idx!=-1 ? posattribs_[idx]->locked_ : false;
}


void Object::removeSelected( const Selector<Coord3>& selector,
			       const TaskRunnerProvider& trprov )
{
    if ( !selector.isOK() )
	return;

    insideselremoval_ = true;
    removebypolyposbox_.setEmpty();

    Geometry::Element* ge = geometryElement();
    if ( !ge ) return;

    mDynamicCastGet(const Geometry::RowColSurface*,surface,ge);
    if ( !surface ) return;

    ObjectSet<const Selector<Coord3> > selectors;
    selectors += &selector;
    ObjectPosSelector posselector( *this, selectors );
    trprov.execute( posselector );

    const TypeSet<EM::PosID>& list = posselector.getSelected();

    setBurstAlert( true );
    int poscount = 0;
    ge->blockCallBacks( true, false );
    for ( int sididx=0; sididx<list.size(); sididx++ )
    {
	unSetPos( list[sididx], true );

	BinID bid = list[sididx].getBinID();
	const Coord3 pos = getPos( list[sididx] );
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

    insideselremoval_ = false;
}


void Object::removePositions( const TypeSet<EM::PosID>& posids )
{
    for ( int sididx = 0; sididx < posids.size(); sididx++ )
    {
	if ( sididx == 0 )
	    setBurstAlert( true );
	unSetPos( posids[sididx], true );
	if ( sididx == posids.size()-1 )
	    setBurstAlert( false );
    }
}


void Object::removeAllUnSeedPos()
{
    setBurstAlert( true );
    PtrMan<EM::ObjectIterator> iterator = createIterator();
    while( true )
    {
	const EM::PosID pid = iterator->next();
	if ( pid.isInvalid() )
	    break;

	if ( !isPosAttrib(pid, EM::Object::sSeedNode()) &&
	     !isNodeSourceType(pid,Manual) &&
	     !isNodeLocked(pid) )
	    unSetPos( pid, true );
    }
    setBurstAlert( false );
}


const TrcKeyZSampling Object::getRemovedPolySelectedPosBox()
{
    return removebypolyposbox_;
}


void Object::emptyRemovedPolySelectedPosBox()
{
    removebypolyposbox_.setEmpty();
}


bool Object::isEmpty() const
{
    PtrMan<EM::ObjectIterator> iterator = createIterator();
    return !iterator || iterator->next().isInvalid();
}


uiString Object::errMsg() const
{
    return errmsg_;
}


void Object::useDisplayPars( const IOPar& par )
{
    IOPar displaypar;

    if( !MGR().readDisplayPars(storageid_,displaypar) )
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


bool Object::usePar( const IOPar& par )
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

	TypeSet<od_int64> posidnrs;

	BufferString posidkey = attribkey;
	posidkey += posattrposidstr();

	par.get( posidkey.buf(), posidnrs );

	for ( int idy=0; idy<posidnrs.size(); idy++ )
	{
	    const PosID posid = PosID::get( posidnrs[idy] );
	    if ( !isDefined(posid) )
		continue;

	    setPosAttrib( posid, attrib, true, false );
	}
    }

    return true;
}


void Object::setLockColor( const Color& col )
{
    mLock4Write();
    lockcolor_ = col;
    mSendEMCBNotif( Object::cLockColorChange() );
}

const Color Object::getLockColor() const
{ return lockcolor_; }


void Object::saveDisplayPars() const
{
    IOPar displaypar;

    displaypar.set( sKey::Color(), preferredColor() );
    BufferString lnststr;

    preferredlinestyle_.toString( lnststr );
    displaypar.set( sKey::LineStyle(), lnststr );

    BufferString mkststr;
    preferredmarkerstyle_.toString( mkststr );
    displaypar.set( sKey::MarkerStyle(), mkststr );

    MGR().writeDisplayPars( storageid_, displaypar );
}


void Object::fillPar( IOPar& par ) const
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
	TypeSet<od_int64> posids;
	for ( int idy=0; idy<pids->size(); idy++ )
	{
	    attrpatches += 0;
	    posids += (*pids)[idy].getI();
	}

	BufferString patchkey = attribkey;
	patchkey += posattrsectionstr();
	BufferString posidkey = attribkey;
	posidkey += posattrposidstr();

	par.set( patchkey.buf(), attrpatches );
	par.set( posidkey.buf(), posids );
    }

    par.set( nrposattrstr(), keyid );
}


void Object::posIDChangeCB(CallBacker* cb)
{
    mCBCapsuleUnpack(ObjectCallbackData,cbdata,cb);
    if ( cbdata.changeType() != cPosIDChange() )
	return;

    RefMan<ChangeAuxData> cbauxdata = cbdata.auxDataAs<ChangeAuxData>();
    if ( !cbauxdata )
	return;

    for ( int idx=0; idx<posattribs_.size(); idx++ )
    {
	TypeSet<PosID>* nodes = &posattribs_[idx]->posids_;
	if ( !nodes ) continue;

	while ( true )
	{
	    const int idy = nodes->indexOf( cbauxdata->pid0 );
	    if ( idy==-1 )
		break;

	    (*nodes)[idy] = cbauxdata->pid1;
	}
    }
}

} // namespace EM
