/*
___________________________________________________________________

 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : J.C. Glas
 * DATE     : October 2008
___________________________________________________________________

-*/

static const char* rcsID = "$Id: faultstickseteditor.cc,v 1.8 2010-07-27 09:00:03 cvsjaap Exp $";

#include "faultstickseteditor.h"

#include "emfaultstickset.h"
#include "emmanager.h"
#include "stickseteditor.h"
#include "mpeengine.h"
#include "selector.h"
#include "survinfo.h"
#include "trigonometry.h"
#include "undo.h"

namespace MPE
{

FaultStickSetEditor::FaultStickSetEditor( EM::FaultStickSet& emfss )
    : ObjectEditor(emfss)
    , editpids_(0)
    , sowingpivot_(Coord3::udf())
{}


ObjectEditor* FaultStickSetEditor::create( EM::EMObject& emobj )
{
    mDynamicCastGet(EM::FaultStickSet*,emfss,&emobj);
    if ( !emfss ) return 0;
    return new FaultStickSetEditor( *emfss );
}


void FaultStickSetEditor::initClass()
{ MPE::EditorFactory().addCreator( create, EM::FaultStickSet::typeStr() ); }


Geometry::ElementEditor* FaultStickSetEditor::createEditor(
						    const EM::SectionID& sid )
{
    const Geometry::Element* ge = emObject().sectionGeometry( sid );
    if ( !ge ) return 0;

    mDynamicCastGet(const Geometry::FaultStickSet*,fss,ge);
    if ( !fss ) return 0;
    
    return new Geometry::StickSetEditor(
			*const_cast<Geometry::FaultStickSet*>(fss) );
}

void FaultStickSetEditor::setEditIDs( const TypeSet<EM::PosID>* editpids )
{
    editpids_ = editpids;
}


void FaultStickSetEditor::getEditIDs( TypeSet<EM::PosID>& ids ) const
{
    if ( editpids_ )
	ids = *editpids_;
    else
	ObjectEditor::getEditIDs( ids );
}


static EM::PosID lastclickedpid_ = EM::PosID::udf();
static Coord3 lastclickedpos_;

void FaultStickSetEditor::setLastClicked( const EM::PosID& pid )
{
    lastclickedpid_ = pid;
    lastclickedpos_ = emObject().getPos( pid );

    if ( sowingpivot_.isDefined() && lastclickedpos_.isDefined() )
	sowinghistory_.insert( 0, lastclickedpos_ );
}


const EM::PosID& FaultStickSetEditor::getLastClicked() const
{
    if ( !lastclickedpid_.isUdf() )
    {
	if ( lastclickedpos_ != emObject().getPos(lastclickedpid_) )
	    lastclickedpid_ = EM::PosID::udf();
    }
    return lastclickedpid_;
}


void FaultStickSetEditor::setSowingPivot( const Coord3 pos )
{
    if ( sowingpivot_.isDefined() && !pos.isDefined() )
	sowinghistory_.erase();

    sowingpivot_ = pos;
}


#define mCompareCoord( crd ) Coord3( crd, crd.z*zfactor )

float FaultStickSetEditor::distToStick(
				const int sticknr,const EM::SectionID& sid,
				const MultiID* lineset, const char* linenm,
				const Coord3& mousepos, float zfactor,
				const Coord3* posnormal ) const
{
    mDynamicCastGet( const EM::FaultStickSet*, emfss, &emObject() );
    if ( !emfss || !mousepos.isDefined() )
	return mUdf(float);

    const Geometry::Element* ge = emfss->sectionGeometry( sid );
    mDynamicCastGet(const Geometry::FaultStickSet*,fss,ge);
    if ( !ge || !fss || fss->isStickHidden(sticknr) )
	return mUdf(float);

    const EM::FaultStickSetGeometry& fssg = emfss->geometry();

    if ( fssg.pickedOn2DLine(sid, sticknr) )
    {
	if ( !lineset || *lineset!=*fssg.lineSet(sid,sticknr) ||
	     strcmp(linenm,fssg.lineName(sid,sticknr)) )
	    return mUdf(float);
    }

    const StepInterval<int> colrange = fss->colRange( sticknr );
    if ( colrange.isUdf() )
	return mUdf(float);

    const Plane3 plane( fss->getEditPlaneNormal(sticknr),
	    		mCompareCoord(mousepos), false );

    if ( posnormal && *posnormal!=Coord3::udf() &&
	 fabs( posnormal->dot(plane.normal()) ) < 0.5 )
	return mUdf(float);

    const float onestepdist = SI().oneStepDistance( plane.normal(), zfactor );

    bool insameplane = false;
    double prevdist = 0.0;
    Coord3 avgpos( 0, 0, 0 );
    int count = 0;

    for ( int knotidx=colrange.nrSteps(); knotidx>=0; knotidx-- )
    {
	const RowCol rc( sticknr, colrange.atIndex(knotidx) );
	const Coord3 pos = fss->getKnot( rc );
	if ( pos.isDefined() )
	{
	    const double curdist =
			 plane.distanceToPoint( mCompareCoord(pos), true );

	    if ( curdist*prevdist<0.0 || fabs(curdist)< 0.5*onestepdist )
		insameplane = true;

	    prevdist = curdist;
	    avgpos += mCompareCoord( pos );
	    count++;
	}
    }

    if ( !count )
	return mUdf(float);

    if ( fssg.pickedOnPlane(sid, sticknr) && !insameplane )
	return mUdf(float);

    avgpos /= count;

    return avgpos.Coord::distTo( mCompareCoord(mousepos) );
}


void FaultStickSetEditor::getInteractionInfo( EM::PosID& insertpid,
				const MultiID* lineset, const char* linenm,
				const Coord3& mousepos, float zfactor,
				const Coord3* posnormal ) const
{
    insertpid = EM::PosID::udf();

    int sticknr;
    EM::SectionID sid;

    const Coord3& pos = sowingpivot_.isDefined() && sowinghistory_.isEmpty()
			? sowingpivot_ : mousepos;

    const EM::PosID lastclicked = getLastClicked();
    if ( !lastclicked.isUdf() && lastclicked.objectID()==emobject.id() )
    {
	sid = lastclicked.sectionID();
	sticknr = RowCol( lastclicked.subID() ).row;

	const float dist = distToStick( sticknr, sid, lineset, linenm,
					pos, zfactor, posnormal );
	if ( !mIsUdf(dist) )
	{
	    getPidsOnStick( insertpid, sticknr, sid, pos, zfactor );
	    return;
	}
    }

    if ( getNearestStick(sticknr,sid,lineset,linenm,pos,zfactor,posnormal) )
	getPidsOnStick( insertpid, sticknr, sid, pos, zfactor );
}


bool FaultStickSetEditor::removeSelection( const Selector<Coord3>& selector )
{
    mDynamicCastGet(EM::FaultStickSet*,emfss,&emobject);
    bool change = false;
    for ( int sectionidx=emfss->nrSections()-1; sectionidx>=0; sectionidx--)
    {
	const EM::SectionID currentsid = emfss->sectionID( sectionidx );
	const Geometry::Element* ge = emfss->sectionGeometry( currentsid );
	if ( !ge ) continue;

	mDynamicCastGet(const Geometry::FaultStickSet*,fss,ge);
	if ( !fss ) continue;

	const StepInterval<int> rowrange = fss->rowRange();
	if ( rowrange.isUdf() )
	    continue;

	for ( int stickidx=rowrange.nrSteps(); stickidx>=0; stickidx-- )
	{
	    const int curstick = rowrange.atIndex(stickidx);
	    const StepInterval<int> colrange = fss->colRange( curstick );
	    if ( fss->isStickHidden(curstick) || colrange.isUdf() )
		continue;

	    for ( int knotidx=colrange.nrSteps(); knotidx>=0; knotidx-- )
	    {
		const RowCol rc( curstick,colrange.atIndex(knotidx) );
		const Coord3 pos = fss->getKnot( rc );

		if ( !pos.isDefined() || !selector.includes(pos) )
		    continue;

		EM::FaultStickSetGeometry& fssg = emfss->geometry();
		const bool res = fssg.nrKnots( currentsid,curstick)==1
		   ? fssg.removeStick( currentsid, curstick, true )
		   : fssg.removeKnot( currentsid, rc.toInt64(), true );

		if ( res )
		    change = true;
	    }
	}
    }

    if ( change )
    {
	EM::EMM().undo().setUserInteractionEnd(
		                            EM::EMM().undo().currentEventID() );
    }

    return change;
}


bool FaultStickSetEditor::getNearestStick( int& sticknr, EM::SectionID& sid,
				const MultiID* lineset, const char* linenm,
				const Coord3& mousepos, float zfactor,
				const Coord3* posnormal) const
{
    mDynamicCastGet( const EM::FaultStickSet*, emfss, &emObject() );
    if ( !emfss || !mousepos.isDefined() )
	return false;

    int selsid, selsticknr;
    float minlinedist = mUdf(float);

    for ( int sectionidx=emfss->nrSections()-1; sectionidx>=0; sectionidx--)
    {
	const EM::SectionID cursid = emfss->sectionID( sectionidx );
	const Geometry::Element* ge = emfss->sectionGeometry( cursid );
	if ( !ge ) continue;

	mDynamicCastGet(const Geometry::FaultStickSet*,fss,ge);
	if ( !fss ) continue;

	const StepInterval<int> rowrange = fss->rowRange();
	if ( rowrange.isUdf() )
	    continue;

	for ( int stickidx=rowrange.nrSteps(); stickidx>=0; stickidx-- )
	{

	    const int cursticknr = rowrange.atIndex(stickidx);
	    const float disttoline = distToStick( cursticknr, cursid, lineset,
						  linenm, mousepos, zfactor,
						  posnormal );
	    if ( !mIsUdf(disttoline) )
	    {
		if ( mIsUdf(minlinedist) || disttoline<minlinedist )
		{
		    minlinedist = disttoline;
		    selsticknr = cursticknr;
		    selsid = cursid;
		}
	    }
	}
    }

    if ( mIsUdf(minlinedist) )
	return false;

    sid = selsid;
    sticknr = selsticknr;
    return true;
}


void FaultStickSetEditor::getPidsOnStick( EM::PosID& insertpid, int sticknr,
	const EM::SectionID& sid, const Coord3& mousepos, float zfactor ) const
{
    EM::PosID nearestpid0 = EM::PosID::udf();
    EM::PosID nearestpid1 = EM::PosID::udf();
    insertpid = EM::PosID::udf();

    if ( !mousepos.isDefined() )
	return;

    const Geometry::Element* ge = emObject().sectionGeometry( sid );
    mDynamicCastGet(const Geometry::FaultStickSet*,fss,ge);

    const StepInterval<int> colrange = fss->colRange( sticknr );
    const int nrknots = colrange.nrSteps()+1;

    TypeSet<int> definedknots;
    int nearestknotidx = -1;
    float minsqdist;
    for ( int knotidx=0; knotidx<nrknots; knotidx++ )
    {
	const RowCol rc( sticknr, colrange.atIndex(knotidx));
	const Coord3 pos = fss->getKnot( rc );

	if ( !pos.isDefined() )
	    continue;

	float sqdist = 0;
	if ( sowinghistory_.isEmpty() || sowinghistory_[0]!=pos )
	    sqdist = mCompareCoord(pos).sqDistTo( mCompareCoord(mousepos) );

	if ( nearestknotidx==-1 || sqdist<minsqdist )
	{
	    minsqdist = sqdist;
	    nearestknotidx = definedknots.size();
	}

	definedknots += colrange.atIndex( knotidx );
    }

    if ( nearestknotidx==-1 )
	return;

    nearestpid0.setObjectID( emObject().id() );
    nearestpid0.setSectionID( sid );
    nearestpid0.setSubID(
	RowCol(sticknr, definedknots[nearestknotidx]).toInt64() );

    if ( definedknots.size()<=1 )
    {
	const int defcol = definedknots[nearestknotidx];
	const Coord3 pos = fss->getKnot( RowCol(sticknr, defcol) );

	const bool isstickvertical = fss->getEditPlaneNormal(sticknr).z < 0.5;
	const int insertcol = defcol + ( isstickvertical
	    ? mousepos.z>pos.z ? 1 : -1
	    : mousepos.coord()>pos.coord() ? 1 : -1) * colrange.step;

	insertpid.setObjectID( emObject().id() );
	insertpid.setSectionID( sid );
	insertpid.setSubID( RowCol( sticknr, insertcol ).toInt64() );
	return;
    }

    const Coord3 pos =
	fss->getKnot( RowCol(sticknr,definedknots[nearestknotidx]) );

    Coord3 nextpos = pos, prevpos = pos;

    if ( nearestknotidx )
	prevpos = fss->getKnot( RowCol(sticknr,definedknots[nearestknotidx-1]));

    if ( nearestknotidx<definedknots.size()-1 )
	nextpos = fss->getKnot( RowCol(sticknr,definedknots[nearestknotidx+1]));

    Coord3 v0 = nextpos-prevpos;
    Coord3 v1 = mousepos-pos;

    bool takeprevious = mCompareCoord(v0).dot( mCompareCoord(v1) ) < 0;
    if ( sowinghistory_.size() > 1 )
	takeprevious = sowinghistory_[1]==prevpos;

    if ( takeprevious ) 
    {
	if ( nearestknotidx )
	{
	    nearestpid1 = nearestpid0;
	    nearestpid1.setSubID(
		RowCol(sticknr,definedknots[nearestknotidx-1]).toInt64());
	    insertpid = nearestpid0;
	}
	else
	{
	    insertpid = nearestpid0;
	    const int insertcol = definedknots[nearestknotidx]-colrange.step;
	    insertpid.setSubID( RowCol(sticknr,insertcol).toInt64() );
	}
    }
    else // take next
    {
	if ( nearestknotidx<definedknots.size()-1 )
	{
	    nearestpid1 = nearestpid0;
	    nearestpid1.setSubID(
		RowCol(sticknr,definedknots[nearestknotidx+1]).toInt64());
	    insertpid = nearestpid1;
	}
	else
	{
	    insertpid = nearestpid0;
	    const int insertcol = definedknots[nearestknotidx]+colrange.step;
	    insertpid.setSubID( RowCol(sticknr,insertcol).toInt64() );
	}
    }
}


};  // namespace MPE
