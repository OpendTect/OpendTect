/*
___________________________________________________________________

 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : Nov 2004
___________________________________________________________________

-*/

static const char* rcsID mUsedVar = "$Id$";

#include "faulteditor.h"

#include "emfault3d.h"
#include "emmanager.h"
#include "stickseteditor.h"
#include "mpeengine.h"
#include "selector.h"
#include "survinfo.h"
#include "trigonometry.h"
#include "undo.h"

namespace MPE
{

FaultEditor::FaultEditor( EM::Fault3D& fault )
    : ObjectEditor(fault)
    , scalevector_( 0, 1, SI().zScale() )
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


static EM::PosID lastclickedpid_ = EM::PosID::udf();

void FaultEditor::setLastClicked( const EM::PosID& pid )
{
    lastclickedpid_ = pid;

    EM::EMObject& emobj = const_cast<EM::EMObject&>( emObject() );
    Geometry::Element* ge = emobj.sectionGeometry( pid.sectionID() );
    mDynamicCastGet( Geometry::FaultStickSet*, fss, ge );
    if ( fss )
	fss->preferStick( pid.getRowCol().row  );

    if ( sowingpivot_.isDefined() )
    {
	const Coord3 pos = emObject().getPos( pid );
	if ( pos.isDefined() )
	    sowinghistory_.insert( 0, pos );
    }
}


int FaultEditor::getLastClickedStick() const
{
    if ( lastclickedpid_.objectID() != emObject().id() )
       return mUdf(int);

    const EM::SectionID& sid = lastclickedpid_.sectionID();
    const Geometry::Element* ge = emObject().sectionGeometry( sid );
    mDynamicCastGet( const Geometry::FaultStickSet*, fss, ge );

    if ( fss )
    {
	const int lastclickedsticknr = lastclickedpid_.getRowCol().row;
	if ( lastclickedsticknr == fss->preferredStickNr() )
	    return lastclickedsticknr;
    }

    return mUdf(int);
}


void FaultEditor::setSowingPivot( const Coord3 pos )
{
    if ( sowingpivot_.isDefined() && !pos.isDefined() )
	sowinghistory_.erase();

    sowingpivot_ = pos;
}


void FaultEditor::setZScale( float zscale )
{ scalevector_ = Coord3( 0, 1, zscale ); }


void FaultEditor::setScaleVector( const Coord3& scalevec )
{ scalevector_ = scalevec; }


#define mWorldScale(crd) \
    Coord3( crd.x, crd.y, SI().zScale()*crd.z )

#define mCustomScale(crd) \
    Coord3( crd.x, Coord(scalevector_).dot(crd), scalevector_.z*crd.z )


float FaultEditor::distToStick( const Geometry::FaultStickSurface& surface,
	  int curstick, const Coord3& mousepos,const Coord3* posnormal ) const
{
    if ( !mousepos.isDefined() )
	return mUdf(float);

    if ( surface.isStickHidden(curstick) )
	return mUdf(float);

    const StepInterval<int> colrange = surface.colRange( curstick );
    if ( colrange.isUdf() )
	return mUdf(float);

    const Coord3 sticknormal = surface.getEditPlaneNormal( curstick );

    if ( posnormal && *posnormal!=Coord3::udf() &&
	 fabs( posnormal->dot(sticknormal) ) < 0.5 )
	return mUdf(float);

    const Plane3 plane( posnormal ? *posnormal : sticknormal,
			mWorldScale(mousepos), false );

    const double onestepdist =
		mWorldScale( SI().oneStepTranslation( plane.normal()) ).abs();

    bool insameplane = false;
    double prevdist = 0.0;
    Coord3 avgpos( 0, 0, 0 );
    int count = 0;

    for ( int knotidx=colrange.nrSteps(); knotidx>=0; knotidx-- )
    {
	const RowCol rc( curstick, colrange.atIndex(knotidx) );
	const Coord3 pos = surface.getKnot( rc );
	if ( pos.isDefined() )
	{
	    const double curdist = plane.distanceToPoint(mWorldScale(pos),true);

	    if ( curdist*prevdist<0.0 || fabs(curdist)< 0.5*onestepdist )
		insameplane = true;

	    prevdist = curdist;
	    avgpos += pos;
	    count++;
	}
    }

    if ( !count || !insameplane )
	return mUdf(float);

    avgpos /= count;
 
    return (float)(mCustomScale(avgpos).Coord::distTo(mCustomScale(mousepos)));
}


static Coord3 avgStickPos( const Geometry::FaultStickSurface& surface,
			   int sticknr )
{
    const StepInterval<int> colrange = surface.colRange( sticknr );
    if ( colrange.isUdf() )
	return Coord3::udf();

    Coord3 avgpos( 0, 0, 0 );
    int count = 0;
    for ( int knotidx=colrange.nrSteps(); knotidx>=0; knotidx-- )
    {
	const RowCol rc( sticknr, colrange.atIndex(knotidx) );
	const Coord3 pos = surface.getKnot( rc );
	if ( pos.isDefined() )
	{
	    avgpos += pos;
	    count++;
	}
    }

    return count ? avgpos/count : Coord3::udf();
}


float FaultEditor::panelIntersectDist(
			const Geometry::FaultStickSurface& surface, int sticknr,
			const Coord3& mousepos, const Coord3& posnormal ) const
{
    if ( !mousepos.isDefined() || !posnormal.isDefined() || !posnormal.abs() )
	return mUdf(float);

    const StepInterval<int> rowrange = surface.rowRange();
    if ( rowrange.isUdf() )
	return mUdf(float);

    const int sticknr0 = sticknr<rowrange.start ? rowrange.stop : sticknr;
    const int sticknr1 = sticknr>=rowrange.stop ? rowrange.start : sticknr+1;

    Coord3 avgpos0 = avgStickPos( surface, sticknr0 );
    Coord3 avgpos1 = avgStickPos( surface, sticknr1 );

    if ( !avgpos0.isDefined() || !avgpos1.isDefined() )
	return mUdf(float);

    const Plane3 plane( posnormal, mWorldScale(mousepos), false );

    float d0 = (float) plane.distanceToPoint( mWorldScale(avgpos0), true );
    float d1 = (float) plane.distanceToPoint( mWorldScale(avgpos1), true );
    if ( mIsUdf(d0) || mIsUdf(d1) )
	return mUdf(float);

    const double onestepdist =
		mWorldScale( SI().oneStepTranslation( plane.normal()) ).abs();
	
    if ( fabs(d0) < 0.5*onestepdist )
	d0 = 0.0;
    if ( fabs(d1) < 0.5*onestepdist )
	d1 = 0.0;

    Coord3 pos = (avgpos0+avgpos1) / 2;
    if ( d0 || d1 )
	pos = (avgpos0*fabs(d1) + avgpos1*fabs(d0)) / (fabs(d0) + fabs(d1));

    if ( sticknr < rowrange.start )
    {
	pos = avgpos1;
	if ( d0*d1<0.0 || fabs(d1)-fabs(d0)>0.5*onestepdist )
	    return mUdf(float);
    }
    else if ( sticknr >= rowrange.stop )
    {
	pos = avgpos0;
	if ( d0*d1<0.0 || fabs(d0)-fabs(d1)>0.5*onestepdist )
	    return mUdf(float);
    }
    else if ( d0*d1 > 0.0 )
	return mUdf(float);

    return (float) (mCustomScale(pos).Coord::distTo( mCustomScale(mousepos) ));
}


int FaultEditor::getSecondKnotNr( const Geometry::FaultStickSurface& surface,
				  int sticknr, const Coord3& mousepos ) const
{
    const StepInterval<int> rowrange = surface.rowRange();
    if ( rowrange.isUdf() || !mousepos.isDefined() )
	return mUdf(int);

    const StepInterval<int> colrange0 = surface.colRange( sticknr );
    if ( colrange0.isUdf() || colrange0.nrSteps() )
	return mUdf(int);

    int res = colrange0.start + colrange0.step;

    int refnr = sticknr;
    for ( int count=1; count<=2*rowrange.nrSteps(); count++ )
    {
	refnr += rowrange.step * count * (refnr<sticknr ? 1 : -1);
	if ( !rowrange.includes(refnr,false) )
	    continue;

	const StepInterval<int> colrange1 = surface.colRange( refnr );
	if ( colrange1.isUdf() || !colrange1.nrSteps() )
	    continue;

	const Coord3 p0 = mWorldScale( mousepos );
	const Coord3 p1 = mWorldScale( surface.getKnot(
				       RowCol(sticknr,colrange0.start) ) );
	const Coord3 p2 = mWorldScale( surface.getKnot(
				       RowCol(refnr,colrange1.start) ) );
	const Coord3 p3 = mWorldScale( surface.getKnot(
				       RowCol(refnr,colrange1.stop) ) );

	if ( p0.distTo(p2)+p1.distTo(p3) < p0.distTo(p3)+p1.distTo(p2) )
	    res = colrange0.start - colrange0.step;

	return res;
    }

    return res;
}
		

void FaultEditor::getInteractionInfo( bool& makenewstick, EM::PosID& insertpid,
		      const Coord3& mousepos, const Coord3* posnormal ) const
{ 
    insertpid = EM::PosID::udf();

    const Coord3& pos = sowingpivot_.isDefined() && sowinghistory_.isEmpty()
			? sowingpivot_ : mousepos;

    if ( !emObject().nrSections() )
	return;

    int sticknr = getLastClickedStick();
    EM::SectionID sid;

    if ( !makenewstick && !mIsUdf(sticknr) )
    {
	sid = lastclickedpid_.sectionID();
	const Geometry::Element* ge = emObject().sectionGeometry( sid );
	mDynamicCastGet(const Geometry::FaultStickSurface*,surface,ge);
	if ( ge && surface )
	{
	    const float dist = distToStick( *surface, sticknr, pos, posnormal );
	    if ( !mIsUdf(dist) )
	    {
		getPidsOnStick( insertpid, sticknr, sid, pos );
		return;
	    }
	}
    }

    makenewstick = makenewstick ||
		   mIsUdf( getNearestStick(sticknr, sid, pos, posnormal) );

    if ( makenewstick )
    {
	sid = emObject().sectionID( 0 );
	sticknr = 0;
	getInsertStick( sticknr, sid, pos, posnormal );

	insertpid.setObjectID( emObject().id() );
	insertpid.setSectionID( sid );
	insertpid.setSubID( RowCol( sticknr, 0 ).toInt64() );
	return;
    }

    getPidsOnStick( insertpid, sticknr, sid, pos );
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
	    const int curstick = rowrange.atIndex(stickidx);
	    const StepInterval<int> colrange = surface->colRange( curstick );
	    if ( surface->isStickHidden(curstick) || colrange.isUdf() )
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
		   : fg.removeKnot( currentsid, rc.toInt64(), true );

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
			const Coord3& mousepos, const Coord3* posnormal ) const
{
    EM::SectionID selsid = mUdf(EM::SectionID); 
    int selstick = mUdf(int);
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
	    const float dist = distToStick( *surface, curstick,
					    mousepos, posnormal );
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


bool FaultEditor::getInsertStick( int& stick, EM::SectionID& sid,
		      const Coord3& mousepos, const Coord3* posnormal ) const
{
    EM::SectionID selsid = mUdf(EM::SectionID); 
    int selstick = mUdf(int);
    float mindist = mUdf(float);
    Coord3 normal = Coord3::udf();

    for ( int sectionidx=0; sectionidx<emObject().nrSections(); sectionidx++)
    {
	const EM::SectionID cursid = emObject().sectionID( sectionidx );
	const Geometry::Element* ge = emObject().sectionGeometry( cursid );
	if ( !ge ) continue;

	mDynamicCastGet(const Geometry::FaultStickSurface*,surface,ge);
	if ( !surface ) continue;

	const StepInterval<int> rowrange = surface->rowRange();
	if ( rowrange.isUdf() )
	    continue;

	if ( !normal.isDefined() )
	{
	    normal = posnormal ? *posnormal :
		     surface->getEditPlaneNormal( rowrange.start );
	}

	for ( int stickidx=rowrange.nrSteps(); stickidx>=-1; stickidx-- )
	{
	    const int sticknr = rowrange.atIndex( stickidx );
	    const float dist = panelIntersectDist( *surface, sticknr,
						   mousepos, normal );
	    if ( mIsUdf(dist) )
		continue;

	    if ( mIsUdf(mindist) || fabs(dist)<fabs(mindist) )
	    {
		mindist = dist;
 		selstick = stickidx==-1 ? sticknr : sticknr+rowrange.step;
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


void FaultEditor::getPidsOnStick( EM::PosID& insertpid, int stick,
			const EM::SectionID& sid, const Coord3& mousepos ) const
{
    EM::PosID nearestpid0 = EM::PosID::udf();
    EM::PosID nearestpid1 = EM::PosID::udf();
    insertpid = EM::PosID::udf();

    if ( !mousepos.isDefined() )
	return;

    const Geometry::Element* ge = emObject().sectionGeometry( sid );
    mDynamicCastGet(const Geometry::FaultStickSurface*,surface,ge);

    const StepInterval<int> colrange = surface->colRange( stick );
    const int nrknots = colrange.nrSteps()+1;

    TypeSet<int> definedknots;
    int nearestknotidx = -1;
    float minsqdist = mUdf(float);
    for ( int knotidx=0; knotidx<nrknots; knotidx++ )
    {
	const RowCol rc( stick, colrange.atIndex(knotidx));
	const Coord3 pos = surface->getKnot( rc );

	if ( !pos.isDefined() )
	    continue;

	float sqdist = 0;
	if ( sowinghistory_.isEmpty() || sowinghistory_[0]!=pos )
	    sqdist = (float)mCustomScale(pos).sqDistTo( mCustomScale(mousepos) );

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
	RowCol(stick, definedknots[nearestknotidx]).toInt64() );

    if ( definedknots.size()<=1 )
    {
	const int insertcol = getSecondKnotNr( *surface, stick, mousepos );
	insertpid.setObjectID( emObject().id() );
	insertpid.setSectionID( sid );
	insertpid.setSubID( RowCol( stick, insertcol ).toInt64() );
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

    bool takeprevious = mCustomScale(v0).dot( mCustomScale(v1) ) < 0;
    if ( sowinghistory_.size() > 1 )
	takeprevious = sowinghistory_[1]==prevpos;

    if ( takeprevious )
    {
	if ( nearestknotidx )
	{
	    nearestpid1 = nearestpid0;
	    nearestpid1.setSubID(
		RowCol(stick,definedknots[nearestknotidx-1]).toInt64() );
	    insertpid = nearestpid0;
	}
	else
	{
	    insertpid = nearestpid0;
	    const int insertcol = definedknots[nearestknotidx]-colrange.step;
	    insertpid.setSubID( RowCol(stick,insertcol).toInt64() );
	}
    }
    else // take next
    {
	if ( nearestknotidx<definedknots.size()-1 )
	{
	    nearestpid1 = nearestpid0;
	    nearestpid1.setSubID(
		RowCol(stick,definedknots[nearestknotidx+1]).toInt64() );
	    insertpid = nearestpid1;
	}
	else
	{
	    insertpid = nearestpid0;
	    const int insertcol = definedknots[nearestknotidx]+colrange.step;
	    insertpid.setSubID( RowCol(stick,insertcol).toInt64() );
	}
    }
}


void FaultEditor::cloneMovingNode()
{
    setLastClicked( movingnode );
    mDynamicCastGet( EM::Fault3D*, emfault, &emobject );
    EM::Fault3DGeometry& fg = emfault->geometry();
    const EM::SectionID& sid = movingnode.sectionID();
    const int sticknr = movingnode.getRowCol().row;
    Geometry::FaultStickSurface* fss = fg.sectionGeometry( sid );

    const Coord3& normal = fss->getEditPlaneNormal( sticknr );
    EM::PosID insertpid;
    bool makenewstick = false; 
    getInteractionInfo( makenewstick, insertpid, startpos, &normal );
    if ( makenewstick || insertpid.isUdf() )
	return;

    if ( movingnode != insertpid )
    {
	fg.insertKnot( sid, insertpid.subID(), startpos, true );
	return;
    }

    // Performs knot insertion without changing PosID of moving node 
    emfault->setBurstAlert( true );
    const StepInterval<int> colrg = fss->colRange( sticknr );
    for ( int col=colrg.start; col<=colrg.stop; col+=colrg.step )
    {
	const RowCol currc( sticknr, col ); 
	const RowCol prevrc( sticknr, col-colrg.step );
	const EM::PosID prevpid( emfault->id(), sid, prevrc.toInt64() );

	if ( currc.toInt64() == insertpid.subID() )
	{
	    ObjectEditor::setPosition( prevpid, startpos );
	    break;
	}

	const Coord3 prevpos = fss->getKnot( currc );
	if ( col == colrg.start )
	    fg.insertKnot( sid, prevrc.toInt64(), prevpos, true );
	else
	    ObjectEditor::setPosition( prevpid, prevpos );
    }
    emfault->setBurstAlert( false );
}


};  // namespace MPE
