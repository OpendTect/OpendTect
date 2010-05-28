/*
___________________________________________________________________

 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : Nov 2004
___________________________________________________________________

-*/

static const char* rcsID = "$Id: faulteditor.cc,v 1.12 2010-05-28 09:51:56 cvsjaap Exp $";

#include "faulteditor.h"

#include "emfault3d.h"
#include "emmanager.h"
#include "stickseteditor.h"
#include "mpeengine.h"
#include "selector.h"
#include "trigonometry.h"
#include "undo.h"

namespace MPE
{

FaultEditor::FaultEditor( EM::Fault3D& fault )
    : ObjectEditor(fault)
    , sowingpivot_(Coord3::udf())
{}


ObjectEditor* FaultEditor::create( EM::EMObject& emobj )
{
    mDynamicCastGet(EM::Fault3D*,fault,&emobj);
    if ( !fault ) return 0;
    return new FaultEditor(*fault);
}


void FaultEditor::initClass()
{ MPE::EditorFactory().addCreator( create, EM::Fault3D::typeStr() ); }


Geometry::ElementEditor* FaultEditor::createEditor( const EM::SectionID& sid )
{
    const Geometry::Element* ge = emObject().sectionGeometry( sid );
    if ( !ge ) return 0;

    mDynamicCastGet(const Geometry::FaultStickSurface*,surface,ge);
    if ( !surface ) return 0;
    
    return new Geometry::StickSetEditor(
			*const_cast<Geometry::FaultStickSurface*>(surface) );
}


static EM::PosID lastclicked_ = EM::PosID::udf();

void FaultEditor::setLastClicked( const EM::PosID& pid )
{
    lastclicked_ = pid;

    if ( sowingpivot_.isDefined() )
    {
	const Coord3 pos = emObject().getPos( pid );
	if ( pos.isDefined() )
	    sowinghistory_.insert( 0, pos );
    }
}


void FaultEditor::setSowingPivot( const Coord3 pos )
{
    if ( sowingpivot_.isDefined() && !pos.isDefined() )
	sowinghistory_.erase();

    sowingpivot_ = pos;
}



#define mCompareCoord( crd ) Coord3( crd, crd.z*zfactor )

static float distToStick( const Geometry::FaultStickSurface& surface,
			  const EM::SectionID& cursid, const int curstick,
			  const Coord3& mousepos, float zfactor )
{
    if ( !mousepos.isDefined() )
	return mUdf(float);

    Coord3 avgpos( 0, 0, 0 );
    int count = 0;
    const StepInterval<int> colrange = surface.colRange( curstick );
    if ( colrange.isUdf() )
	return mUdf(float);

    for ( int knotidx=colrange.nrSteps(); knotidx>=0; knotidx-- )
    {
	const RowCol rc( curstick, colrange.atIndex(knotidx) );
	const Coord3 pos = surface.getKnot( rc );
	if ( pos.isDefined() )
	{
	    avgpos += mCompareCoord( pos );
	    count++;
	}
    }

    if ( !count )
	return mUdf(float);

    avgpos /= count;

    const Plane3 plane( surface.getEditPlaneNormal(curstick), avgpos, false );
    return plane.distanceToPoint( mCompareCoord(mousepos), true );
}


void FaultEditor::getInteractionInfo( bool& makenewstick, EM::PosID& insertpid,
				const Coord3& mousepos, float zfactor ) const
{ 
    const float maxdisttostickplane = 50;

    insertpid = EM::PosID::udf();

    const Coord3& pos = sowingpivot_.isDefined() && sowinghistory_.isEmpty()
			? sowingpivot_ : mousepos;

    if ( !emObject().nrSections() )
	return;

    int stick;
    EM::SectionID sid;
    EM::PosID nearestpid0, nearestpid1;

    if ( !makenewstick && !lastclicked_.isUdf() &&
	 lastclicked_.objectID()==emObject().id() )
    {
	sid = lastclicked_.sectionID();
	const Geometry::Element* ge = emObject().sectionGeometry( sid );
	mDynamicCastGet(const Geometry::FaultStickSurface*,surface,ge);
	if ( ge && surface )
	{
	    stick = RowCol( lastclicked_.subID() ).row;
	    const float dist = distToStick( *surface, sid, stick,
					    pos, zfactor );

	    if ( !mIsUdf(dist) && fabs(dist)<=maxdisttostickplane )
	    {
		getPidsOnStick( nearestpid0, nearestpid1, insertpid,
				stick, sid, pos, zfactor );
		return;
	    }
	}
    }

    const float mindist = getNearestStick( stick, sid, pos, zfactor );
    if ( mIsUdf(mindist) )
    {
	makenewstick = true;
	sid = emObject().sectionID( 0 );
	const Geometry::Element* ge = emObject().sectionGeometry( sid );
	if ( !ge ) return;

	mDynamicCastGet(const Geometry::FaultStickSurface*,surface,ge);
	if ( !surface ) return;

	const StepInterval<int> rowrange = surface->rowRange();
	if ( !rowrange.isUdf() )
	    return;

	insertpid.setObjectID( emObject().id() );
	insertpid.setSectionID( sid );
	insertpid.setSubID( RowCol( 0, 0 ).getSerialized() );
	return;
    }

    if ( fabs(mindist) > maxdisttostickplane )
	makenewstick = true;

    if ( makenewstick )
    {
	sid = emObject().sectionID( 0 );
	const Geometry::Element* ge = emObject().sectionGeometry( sid );
	if ( !ge ) return;

	mDynamicCastGet(const Geometry::FaultStickSurface*,surface,ge);
	if ( !surface ) return;

	const StepInterval<int> rowrange = surface->rowRange();
	if ( rowrange.isUdf() )
	    return;

	insertpid.setObjectID( emObject().id() );
	insertpid.setSectionID( sid );

	const int newstick = mindist>0
	    ? stick+rowrange.step
	    : stick==rowrange.start ? stick-rowrange.step : stick;
	insertpid.setSubID( RowCol( newstick, 0 ).getSerialized() );
	return;
    }

    getPidsOnStick( nearestpid0, nearestpid1, insertpid, stick, sid, pos,
	            zfactor );
}


bool FaultEditor::removeSelection( const Selector<Coord3>& selector )
{
    mDynamicCastGet(EM::Fault3D*,fault,&emobject);
    bool change = false;
    for ( int sectionidx=fault->nrSections()-1; sectionidx>=0; sectionidx--)
    {
	const EM::SectionID currentsid = fault->sectionID( sectionidx );
	const Geometry::Element* ge = fault->sectionGeometry( currentsid );
	if ( !ge ) continue;

	mDynamicCastGet(const Geometry::FaultStickSurface*,surface,ge);
	if ( !surface ) continue;

	const StepInterval<int> rowrange = surface->rowRange();
	if ( rowrange.isUdf() )
	    continue;

	for ( int stickidx=rowrange.nrSteps(); stickidx>=0; stickidx-- )
	{
	    Coord3 avgpos( 0, 0, 0 );
	    int count = 0;
	    const int curstick = rowrange.atIndex(stickidx);
	    const StepInterval<int> colrange = surface->colRange( curstick );
	    if ( colrange.isUdf() )
		continue;

	    for ( int knotidx=colrange.nrSteps(); knotidx>=0; knotidx-- )
	    {
		const RowCol rc( curstick,colrange.atIndex(knotidx) );
		const Coord3 pos = surface->getKnot( rc );

		if ( !pos.isDefined() || !selector.includes(pos) )
		    continue;

		EM::Fault3DGeometry& fg = fault->geometry();
		const bool res = fg.nrKnots( currentsid,curstick)==1
		   ? fg.removeStick( currentsid, curstick, true )
		   : fg.removeKnot( currentsid, rc.getSerialized(), true );

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


float FaultEditor::getNearestStick( int& stick, EM::SectionID& sid,
				    const Coord3& mousepos,
				    float zfactor ) const
{
    int selsid, selstick;
    float mindist = mUdf(float);

    for ( int sectionidx=emObject().nrSections()-1; sectionidx>=0; sectionidx--)
    {
	const EM::SectionID cursid = emObject().sectionID( sectionidx );
	const Geometry::Element* ge = emObject().sectionGeometry( cursid );
	if ( !ge ) continue;

	mDynamicCastGet(const Geometry::FaultStickSurface*,surface,ge);
	if ( !surface ) continue;

	const StepInterval<int> rowrange = surface->rowRange();
	if ( rowrange.isUdf() )
	    continue;

	for ( int stickidx=rowrange.nrSteps(); stickidx>=0; stickidx-- )
	{
	    const int curstick = rowrange.atIndex( stickidx );
	    const float dist = distToStick( *surface, cursid, curstick,
					    mousepos, zfactor );
	    if ( mIsUdf(dist) )
		continue;

	    if ( mIsUdf(mindist) || fabs(dist)<fabs(mindist) )
	    {
		mindist = dist;
		selstick = curstick;
		selsid = cursid;
	    }
	}
    }

    if ( !mIsUdf(mindist) )
    {
	sid = selsid;
	stick = selstick;
    }

    return mindist;
}


void FaultEditor::getPidsOnStick(  EM::PosID& nearestpid0,
	EM::PosID& nearestpid1, EM::PosID& insertpid, int stick,
	const EM::SectionID& sid, const Coord3& mousepos, float zfactor ) const
{
    nearestpid0 = EM::PosID::udf();
    nearestpid1 = EM::PosID::udf();
    insertpid = EM::PosID::udf();

    if ( !mousepos.isDefined() )
	return;

    const Geometry::Element* ge = emObject().sectionGeometry( sid );
    mDynamicCastGet(const Geometry::FaultStickSurface*,surface,ge);

    const StepInterval<int> colrange = surface->colRange( stick );
    const int nrknots = colrange.nrSteps()+1;

    TypeSet<int> definedknots;
    int nearestknotidx = -1;
    float minsqdist;
    for ( int knotidx=0; knotidx<nrknots; knotidx++ )
    {
	const RowCol rc( stick, colrange.atIndex(knotidx));
	const Coord3 pos = surface->getKnot( rc );

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
	RowCol(stick, definedknots[nearestknotidx]).getSerialized() );

    if ( definedknots.size()<=1 )
    {
	const int defcol = definedknots[nearestknotidx];
	const Coord3 pos = surface->getKnot( RowCol(stick, defcol) );
	const int insertcol = defcol + (surface->areSticksVertical() 
	    ? mousepos.z>pos.z ? 1 : -1
	    : mousepos.coord()>pos.coord() ? 1 : -1) * colrange.step;

	insertpid.setObjectID( emObject().id() );
	insertpid.setSectionID( sid );
	insertpid.setSubID( RowCol( stick, insertcol ).getSerialized() );
	return;
    }

    const Coord3 pos =
	surface->getKnot( RowCol(stick,definedknots[nearestknotidx]) );

    Coord3 nextpos = pos, prevpos = pos;

    if ( nearestknotidx )
	prevpos =
	    surface->getKnot( RowCol(stick, definedknots[nearestknotidx-1] ) );

    if ( nearestknotidx<definedknots.size()-1 )
	nextpos =
	    surface->getKnot( RowCol(stick, definedknots[nearestknotidx+1] ) );

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
		RowCol(stick,definedknots[nearestknotidx-1]).getSerialized() );
	    insertpid = nearestpid0;
	}
	else
	{
	    insertpid = nearestpid0;
	    const int insertcol = definedknots[nearestknotidx]-colrange.step;
	    insertpid.setSubID( RowCol(stick,insertcol).getSerialized() );
	}
    }
    else // take next
    {
	if ( nearestknotidx<definedknots.size()-1 )
	{
	    nearestpid1 = nearestpid0;
	    nearestpid1.setSubID(
		RowCol(stick,definedknots[nearestknotidx+1]).getSerialized() );
	    insertpid = nearestpid1;
	}
	else
	{
	    insertpid = nearestpid0;
	    const int insertcol = definedknots[nearestknotidx]+colrange.step;
	    insertpid.setSubID( RowCol(stick,insertcol).getSerialized() );
	}
    }
}


};  // namespace MPE
