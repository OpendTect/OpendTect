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
const char* EMObject::posattrposidstr()	    { return " PosID"; }
const char* EMObject::nrposattrstr()	    { return "Nr Pos Attribs"; }

Color EMObject::sDefaultSelectionColor() { return Color::Orange(); }

EMObject::EMObject( const char* nm )
    : SharedObject( nm )
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

    removebypolyposbox_.setEmpty();

    objectChanged().notify( mCB(this,EMObject,posIDChangeCB) );
}


EMObject::~EMObject()
{
    deepErase( posattribs_ );
    delete &preferredcolor_;
    delete &preferredlinestyle_;
    delete &preferredmarkerstyle_;
    delete &selectioncolor_;

    objectChanged().remove( mCB(this,EMObject,posIDChangeCB) );
}


void EMObject::prepareForDelete()
{
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


const Geometry::Element* EMObject::geometryElement() const
{ return 0; }

Geometry::Element* EMObject::geometryElement()
{ return 0; }


Coord3 EMObject::getPos( const EM::PosID& posid ) const
{
    const Geometry::Element* element = geometryElement();
    return element ? element->getPosition( posid ) : Coord3::udf();
}


#define mRetErr( msg ) { errmsg_ = msg; return false; }

bool EMObject::setPos(	const PosID& pid, const Coord3& newpos,
			bool addtoundo, NodeSourceType tp )
{
    return setPosition( pid, newpos, addtoundo, tp );
}


bool EMObject::setPosition( const PosID& posid,
			    const Coord3& newpos, bool addtoundo,
			    NodeSourceType tp )
{
    mLock4Write();

    Geometry::Element* element = geometryElement();
    if ( !element ) mRetErr( uiString::emptyString() );

    Coord3 oldpos = Coord3::udf();
    if ( addtoundo )
	oldpos = element->getPosition( posid );

    if ( !element->setPosition(posid,newpos) )
	 mRetErr( element->errMsg() );

    if ( !newpos.isDefined() )
    {
	for ( int idx=0; idx<posattribs_.size(); idx++ )
	{
	    TypeSet<PosID>& nodes = posattribs_[idx]->posids_;
	    if ( !&nodes ) continue;

	    if ( nodes.isPresent(posid) )
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
	RefMan<EMChangeAuxData> data = new EMChangeAuxData;
	data->flagfor2dviewer = !yn;
	mSendEMCBNotifWithData( cBurstAlert(), data );
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


bool EMObject::enableGeometryChecks( bool )
{ return true; }


bool EMObject::isGeometryChecksEnabled() const
{ return true; }


bool EMObject::isDefined( const EM::PosID& posid ) const
{
    const Geometry::Element* element = geometryElement();
    return element && element->isDefined( posid );
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

    if ( !hasBurstAlert() )
    {
	RefMan<EMChangeAuxData> data = new EMChangeAuxData;
	data->attrib = attr;
	data->pid0 = pid;
	mSendEMCBNotifWithData( cAttribChange(), data );
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


OD::MarkerStyle3D EMObject::getPosAttrMarkerStyle( int attr )
{
    addPosAttrib( attr );
    return preferredMarkerStyle3D();
}


void EMObject::setPosAttrMarkerStyle( int attr, const OD::MarkerStyle3D& ms )
{
    mLock4Write();
    addPosAttrib( attr );
    setPreferredMarkerStyle3D( ms );

    RefMan<EMChangeAuxData> data = new EMChangeAuxData;
    data->attrib = attr;
    mSendEMCBNotifWithData( cAttribChange(), data );
    touch();
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
>>>>>>> origin/master
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
    Geometry::Element* ge = geometryElement();
    if ( !ge ) return;

    mDynamicCastGet(const Geometry::RowColSurface*,surface,ge);
    if ( !surface ) return;

    ObjectSet<const Selector<Coord3> > selectors;
    selectors += &selector;
    EMObjectPosSelector posselector( *this, selectors );
    posselector.executeParallel( tskr );

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


void EMObject::removePositions( const TypeSet<EM::PosID>& posids )
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


void EMObject::removeAllUnSeedPos()
{
    setBurstAlert( true );
    PtrMan<EM::EMObjectIterator> iterator = createIterator();
    while( true )
    {
	const EM::PosID pid = iterator->next();
	if ( pid.isInvalid() )
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
    PtrMan<EM::EMObjectIterator> iterator = createIterator();
    return !iterator || iterator->next().isInvalid();
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
	TypeSet<od_int64> posidnrs;

	BufferString sectionkey = attribkey;
	sectionkey += posattrsectionstr();

	BufferString posidkey = attribkey;
	posidkey += posattrposidstr();

	par.get( sectionkey.buf(), sections );
	par.get( posidkey.buf(), posidnrs );

	const int minsz = mMIN( sections.size(), posidnrs.size() );
	for ( int idy=0; idy<minsz; idy++ )
	{
	    const PosID posid = PosID::get( posidnrs[idy] );
	    if ( !isDefined(posid) )
		continue;

	    setPosAttrib( posid, attrib, true, false );
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


void EMObject::posIDChangeCB(CallBacker* cb)
{
    mCBCapsuleUnpack(EMObjectCallbackData,cbdata,cb);
    if ( cbdata.changeType() != cPosIDChange() )
	return;

    RefMan<EMChangeAuxData> cbauxdata = cbdata.auxDataAs<EMChangeAuxData>();
    if ( !cbauxdata )
	return;

    for ( int idx=0; idx<posattribs_.size(); idx++ )
    {
	TypeSet<PosID>& nodes = posattribs_[idx]->posids_;
	if ( !&nodes ) continue;

	while ( true )
	{
	    const int idy = nodes.indexOf( cbauxdata->pid0 );
	    if ( idy==-1 )
		break;

	    nodes[idy] = cbauxdata->pid1;
	}
    }
}

} // namespace EM
