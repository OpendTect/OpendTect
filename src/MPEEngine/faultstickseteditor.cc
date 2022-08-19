/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

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
    , scalevector_( 0, 1, SI().zScale() )
    , sceneidx_(-1)
    , sowingpivot_(Coord3::udf())
{}


ObjectEditor* FaultStickSetEditor::create( EM::EMObject& emobj )
{
    mDynamicCastGet(EM::FaultStickSet*,emfss,&emobj);
    if ( !emfss ) return nullptr;
    return new FaultStickSetEditor( *emfss );
}


void FaultStickSetEditor::initClass()
{ MPE::EditorFactory().addCreator( create, EM::FaultStickSet::typeStr() ); }


Geometry::ElementEditor* FaultStickSetEditor::createEditor()
{
    const Geometry::Element* ge = emObject().geometryElement();
    if ( !ge ) return nullptr;

    mDynamicCastGet(const Geometry::FaultStickSet*,fss,ge);
    if ( !fss ) return nullptr;

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

void FaultStickSetEditor::setLastClicked( const EM::PosID& pid )
{
    lastclickedpid_ = pid;

    EM::EMObject& emobj = const_cast<EM::EMObject&>( emObject() );
    Geometry::Element* ge = emobj.geometryElement();
    mDynamicCastGet( Geometry::FaultStickSet*, fss, ge );
    if ( fss )
	fss->preferStick( pid.getRowCol().row()  );

    if ( sowingpivot_.isDefined() )
    {
	const Coord3 pos = emObject().getPos( pid );
	if ( pos.isDefined() )
	    sowinghistory_.insert( 0, pos );
    }
}


int FaultStickSetEditor::getLastClickedStick() const
{
    if ( lastclickedpid_.objectID() != emObject().id() )
	return mUdf(int);

    const Geometry::Element* ge = emObject().geometryElement();
    mDynamicCastGet( const Geometry::FaultStickSet*, fss, ge );

    if ( fss )
    {
	const int lastclickedsticknr = lastclickedpid_.getRowCol().row();
	if ( lastclickedsticknr == fss->preferredStickNr() )
	    return lastclickedsticknr;
    }

    return mUdf(int);
}


void FaultStickSetEditor::setSowingPivot( const Coord3 pos )
{
    if ( sowingpivot_.isDefined() && !pos.isDefined() )
	sowinghistory_.erase();

    sowingpivot_ = pos;
}


void FaultStickSetEditor::setZScale( float zscale )
{ scalevector_ = Coord3( 0, 1, zscale ); }


void FaultStickSetEditor::setScaleVector( const Coord3& scalevec )
{ scalevector_ = scalevec; }


#define mWorldScale(crd) \
    Coord3( crd.x, crd.y, SI().zScale()*crd.z )

#define mCustomScale(crd) \
    Coord3( crd.x, Coord(scalevector_).dot(crd), scalevector_.z*crd.z )


float FaultStickSetEditor::distToStick( int sticknr,
			const MultiID* pickedmid, const char* pickednm,
			Pos::GeomID pickedgeomid, const Coord3& mousepos,
			const Coord3* posnormal ) const
{
    mDynamicCastGet( const EM::FaultStickSet*, emfss, &emObject() );
    if ( !emfss || !mousepos.isDefined() )
	return mUdf(float);

    const Geometry::Element* ge = emfss->geometryElement();
    mDynamicCastGet(const Geometry::FaultStickSet*,fss,ge);
    if ( !ge || !fss || fss->isStickHidden(sticknr,sceneidx_) )
	return mUdf(float);

    const EM::FaultStickSetGeometry& fssg = emfss->geometry();

    if ( fssg.pickedOn2DLine(sticknr) )
    {
	const Pos::GeomID geomid = fssg.pickedGeomID( sticknr );
	if ( geomid != pickedgeomid )
	    return mUdf(float);
    }
    else if ( !fssg.pickedOnPlane(sticknr) )
    {
	const MultiID* mid = fssg.pickedMultiID( sticknr );
	if ( !pickedmid || !mid || *pickedmid!=*mid )
	    return mUdf(float);

	const StringView nm( fssg.pickedName(sticknr) );
	if ( (pickednm || nm) && ( nm != pickednm ) )
	   return mUdf(float);
    }

    const StepInterval<int> colrange = fss->colRange( sticknr );
    if ( colrange.isUdf() )
	return mUdf(float);

    const Plane3 plane( fss->getEditPlaneNormal(sticknr),
			mWorldScale(mousepos), false );

    if ( posnormal && *posnormal!=Coord3::udf() &&
	 fabs( posnormal->dot(plane.normal()) ) < 0.5 )
	return mUdf(float);

    const double onestepdist =
		mWorldScale( SI().oneStepTranslation(plane.normal()) ).abs();

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
	    const double curdist = plane.distanceToPoint(mWorldScale(pos),true);

	    if ( curdist*prevdist<0.0 || fabs(curdist)< 0.5*onestepdist )
		insameplane = true;

	    prevdist = curdist;
	    avgpos += pos;
	    count++;
	}
    }

    if ( !count )
	return mUdf(float);

    if ( fssg.pickedOnPlane(sticknr) && !insameplane )
	return mUdf(float);

    avgpos /= count;

    return (float)(mCustomScale(avgpos).Coord::distTo( mCustomScale(mousepos)));
}


void FaultStickSetEditor::getInteractionInfo( EM::PosID& insertpid,
			const MultiID* pickedmid, const char* pickednm,
			Pos::GeomID pickedgeomid, const Coord3& mousepos,
			const Coord3* posnormal ) const
{
    insertpid = EM::PosID::udf();

    int sticknr = getLastClickedStick();

    const Coord3& pos = sowingpivot_.isDefined() && sowinghistory_.isEmpty()
			? sowingpivot_ : mousepos;

    if ( !mIsUdf(sticknr) )
    {
	const float dist = distToStick( sticknr, pickedmid, pickednm,
					pickedgeomid, pos, posnormal );
	if ( !mIsUdf(dist) )
	{
	    getPidsOnStick( insertpid, sticknr, pos );
	    return;
	}
    }

    if ( getNearestStick(sticknr,pickedmid,pickednm,pickedgeomid,pos,
			 posnormal) )
	getPidsOnStick( insertpid, sticknr, pos );
}


const EM::PosID FaultStickSetEditor::getNearestStick( const Coord3& mousepos,
    Pos::GeomID pickedgeomid, const Coord3* normal ) const
{
    EM::PosID pid = EM::PosID::udf();
    int sticknr = getLastClickedStick();
    const Coord3& pos = sowingpivot_.isDefined() && sowinghistory_.isEmpty()
			? sowingpivot_ : mousepos;

    MultiID pickedmid;

    if ( getNearestStick(sticknr,&pickedmid,"",pickedgeomid,pos, normal) )
	getPidsOnStick( pid, sticknr, pos );

    return pid;
}


bool FaultStickSetEditor::removeSelection( const Selector<Coord3>& selector )
{
    mDynamicCastGet(EM::FaultStickSet*,emfss,emobject_.ptr());
    bool change = false;
    for ( int sectionidx=emfss->nrSections()-1; sectionidx>=0; sectionidx--)
    {
	const Geometry::Element* ge = emfss->geometryElement();
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
	    if ( fss->isStickHidden(curstick,sceneidx_) || colrange.isUdf() )
		continue;

	    for ( int knotidx=colrange.nrSteps(); knotidx>=0; knotidx-- )
	    {
		const RowCol rc( curstick,colrange.atIndex(knotidx) );
		const Coord3 pos = fss->getKnot( rc );

		if ( !pos.isDefined() || !selector.includes(pos) )
		    continue;

		EM::FaultStickSetGeometry& fssg = emfss->geometry();
		const bool res = fssg.nrKnots(curstick)==1
		   ? fssg.removeStick( curstick, true )
		   : fssg.removeKnot( rc.toInt64(), true );

		if ( res )
		    change = true;
	    }
	}
    }

    if ( change )
    {
	EM::EMM().undo(emObject().id()).setUserInteractionEnd(
	    EM::EMM().undo(emObject().id()).currentEventID() );
    }

    return change;
}


bool FaultStickSetEditor::getNearestStick( int& sticknr,
			const MultiID* pickedmid, const char* pickednm,
			Pos::GeomID pickedgeomid, const Coord3& mousepos,
			const Coord3* posnormal) const
{
    mDynamicCastGet( const EM::FaultStickSet*, emfss, &emObject() );
    if ( !emfss || !mousepos.isDefined() )
	return false;

    int selsticknr = mUdf(int);
    float minlinedist = mUdf(float);

    for ( int sectionidx=emfss->nrSections()-1; sectionidx>=0; sectionidx--)
    {
	const Geometry::Element* ge = emfss->geometryElement();
	if ( !ge ) continue;

	mDynamicCastGet(const Geometry::FaultStickSet*,fss,ge);
	if ( !fss ) continue;

	const StepInterval<int> rowrange = fss->rowRange();
	if ( rowrange.isUdf() )
	    continue;

	for ( int stickidx=rowrange.nrSteps(); stickidx>=0; stickidx-- )
	{

	    const int cursticknr = rowrange.atIndex(stickidx);
	    const float disttoline = distToStick( cursticknr, pickedmid,
				pickednm, pickedgeomid, mousepos, posnormal );
	    if ( !mIsUdf(disttoline) )
	    {
		if ( mIsUdf(minlinedist) || disttoline<minlinedist )
		{
		    minlinedist = disttoline;
		    selsticknr = cursticknr;
		}
	    }
	}
    }

    if ( mIsUdf(minlinedist) )
	return false;

    sticknr = selsticknr;
    return true;
}


void FaultStickSetEditor::getPidsOnStick( EM::PosID& insertpid, int sticknr,
		const Coord3& mousepos ) const
{
    EM::PosID nearestpid0 = EM::PosID::udf();
    EM::PosID nearestpid1 = EM::PosID::udf();
    insertpid = EM::PosID::udf();

    if ( !mousepos.isDefined() )
	return;

    const Geometry::Element* ge = emObject().geometryElement();
    mDynamicCastGet(const Geometry::FaultStickSet*,fss,ge);

    const StepInterval<int> colrange = fss->colRange( sticknr );
    const int nrknots = colrange.nrSteps()+1;

    TypeSet<int> definedknots;
    int nearestknotidx = -1;
    float minsqdist = mUdf(float);
    for ( int knotidx=0; knotidx<nrknots; knotidx++ )
    {
	const RowCol rc( sticknr, colrange.atIndex(knotidx));
	const Coord3 pos = fss->getKnot( rc );

	if ( !pos.isDefined() )
	    continue;

	float sqdist = 0;
	if ( sowinghistory_.isEmpty() || sowinghistory_[0]!=pos )
	    sqdist = (float)(mCustomScale(pos).sqDistTo(
						mCustomScale(mousepos) ));

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

    bool takeprevious = mCustomScale(v0).dot( mCustomScale(v1) ) < 0;
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


void FaultStickSetEditor::cloneMovingNode( CallBacker* )
{
    setLastClicked( movingnode );
    mDynamicCastGet( EM::FaultStickSet*, emfss, emobject_.ptr() );
    EM::FaultStickSetGeometry& fssg = emfss->geometry();
    const int sticknr = movingnode.getRowCol().row();
    Geometry::FaultStickSet* fss = fssg.geometryElement();
    const MultiID* pickedmid = fssg.pickedMultiID( sticknr );
    const Pos::GeomID pickedgeomid = fssg.pickedGeomID( sticknr );
    const char* pickednm = fssg.pickedName( sticknr );
    const Coord3& normal = fss->getEditPlaneNormal( sticknr );
    EM::PosID insertpid;
    getInteractionInfo( insertpid, pickedmid, pickednm, pickedgeomid,
			startpos, &normal );
    if ( insertpid.isUdf() )
	return;

    if ( movingnode != insertpid )
    {
	fssg.insertKnot( insertpid.subID(), startpos, true );
	return;
    }

    // Performs knot insertion without changing PosID of moving node
    emfss->setBurstAlert( true );
    const StepInterval<int> colrg = fss->colRange( sticknr );
    for ( int col=colrg.start; col<=colrg.stop; col+=colrg.step )
    {
	const RowCol currc( sticknr, col );
	const RowCol prevrc( sticknr, col-colrg.step );
	const EM::PosID prevpid( emfss->id(), prevrc.toInt64() );

	if ( currc.toInt64() == insertpid.subID() )
	{
	    ObjectEditor::setPosition( prevpid, startpos );
	    break;
	}

	const Coord3 prevpos = fss->getKnot( currc );
	if ( col == colrg.start )
	    fssg.insertKnot( prevrc.toInt64(), prevpos, true );
	else
	    ObjectEditor::setPosition( prevpid, prevpos );
    }
    emfss->setBurstAlert( false );
}


};  // namespace MPE
