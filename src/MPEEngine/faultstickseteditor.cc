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


RefMan<MPE::FaultStickSetEditor>
	MPE::FaultStickSetEditor::create( const EM::FaultStickSet& emfss )
{
    return new FaultStickSetEditor( emfss );
}


MPE::FaultStickSetEditor::FaultStickSetEditor( const EM::FaultStickSet& emfss )
    : ObjectEditor(emfss)
    , scalevector_(0,1,SI().zScale())
{}


MPE::FaultStickSetEditor::~FaultStickSetEditor()
{}


Geometry::ElementEditor* MPE::FaultStickSetEditor::createEditor()
{
    RefMan<EM::EMObject> emobject = emObject();
    Geometry::Element* ge = emobject ? emobject->geometryElement() : nullptr;
    mDynamicCastGet(Geometry::FaultStickSet*,fss,ge);
    return fss ? new Geometry::StickSetEditor( *fss ) : nullptr;
}


void MPE::FaultStickSetEditor::setEditIDs( const TypeSet<EM::PosID>* editpids )
{
    editpids_ = editpids;
}


void MPE::FaultStickSetEditor::getEditIDs( TypeSet<EM::PosID>& ids ) const
{
    if ( editpids_ )
	ids = *editpids_;
    else
	ObjectEditor::getEditIDs( ids );
}


void MPE::FaultStickSetEditor::setLastClicked( const EM::PosID& pid )
{
    RefMan<EM::EMObject> emobject = emObject();
    if ( !emobject )
	return;

    lastclickedpid() = pid;
    Geometry::Element* ge = emobject->geometryElement();
    mDynamicCastGet( Geometry::FaultStickSet*, fss, ge );
    if ( fss )
	fss->preferStick( pid.getRowCol().row()  );

    if ( sowingpivot_.isDefined() )
    {
	const Coord3 pos = emobject->getPos( pid );
	if ( pos.isDefined() )
	    sowinghistory_.insert( 0, pos );
    }
}


int MPE::FaultStickSetEditor::getLastClickedStick() const
{
    ConstRefMan<EM::EMObject> emobject = emObject();
    if ( !emobject || lastclickedpid().objectID() != emobject->id() )
	return mUdf(int);

    const Geometry::Element* ge = emobject->geometryElement();
    mDynamicCastGet( const Geometry::FaultStickSet*, fss, ge );
    if ( fss )
    {
	const int lastclickedsticknr = lastclickedpid().getRowCol().row();
	if ( lastclickedsticknr == fss->preferredStickNr() )
	    return lastclickedsticknr;
    }

    return mUdf(int);
}


void MPE::FaultStickSetEditor::setSowingPivot( const Coord3 pos )
{
    if ( sowingpivot_.isDefined() && !pos.isDefined() )
	sowinghistory_.erase();

    sowingpivot_ = pos;
}


void MPE::FaultStickSetEditor::setZScale( float zscale )
{ scalevector_ = Coord3( 0, 1, zscale ); }


void MPE::FaultStickSetEditor::setScaleVector( const Coord3& scalevec )
{ scalevector_ = scalevec; }


#define mWorldScale(crd) \
    Coord3( crd.x_, crd.y_, SI().zScale()*crd.z_ )

#define mCustomScale(crd) \
    Coord3( crd.x_, Coord(scalevector_).dot(crd), scalevector_.z_*crd.z_ )


float MPE::FaultStickSetEditor::distToStick( int sticknr,
			const MultiID* pickedmid, const char* pickednm,
			const Pos::GeomID& pickedgeomid, const Coord3& mousepos,
			const Coord3* posnormal ) const
{
    ConstRefMan<EM::EMObject> emobject = emObject();
    mDynamicCastGet( const EM::FaultStickSet*, emfss, emobject.ptr() );
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
	if ( (pickednm || !nm.isNull()) && ( nm != pickednm ) )
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


void MPE::FaultStickSetEditor::getInteractionInfo( EM::PosID& insertpid,
			const MultiID* pickedmid, const char* pickednm,
			const Pos::GeomID& pickedgeomid, const Coord3& mousepos,
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


const EM::PosID MPE::FaultStickSetEditor::getNearestStick(
			const Coord3& mousepos, const Pos::GeomID& pickedgeomid,
			const Coord3* normal ) const
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


bool MPE::FaultStickSetEditor::removeSelection(
					const Selector<Coord3>& selector )
{
    RefMan<EM::EMObject> emobject = emObject();
    mDynamicCastGet(EM::FaultStickSet*,emfss,emobject.ptr());
    bool change = false;
    for ( int sectionidx=emfss->nrSections()-1; sectionidx>=0; sectionidx--)
    {
	const Geometry::Element* ge = emfss ? emfss->geometryElement()
					    : nullptr;
	if ( !ge )
	    continue;

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
	EM::EMM().undo(emobject->id()).setUserInteractionEnd(
	    EM::EMM().undo(emobject->id()).currentEventID() );
    }

    return change;
}


bool MPE::FaultStickSetEditor::getNearestStick( int& sticknr,
			const MultiID* pickedmid, const char* pickednm,
			const Pos::GeomID& pickedgeomid, const Coord3& mousepos,
			const Coord3* posnormal) const
{
    ConstRefMan<EM::EMObject> emobject = emObject();
    mDynamicCastGet( const EM::FaultStickSet*, emfss, emobject.ptr() );
    if ( !emfss || !mousepos.isDefined() )
	return false;

    int selsticknr = mUdf(int);
    float minlinedist = mUdf(float);

    for ( int sectionidx=emfss->nrSections()-1; sectionidx>=0; sectionidx--)
    {
	const Geometry::Element* ge = emfss->geometryElement();
	if ( !ge )
	    continue;

	mDynamicCastGet(const Geometry::FaultStickSet*,fss,ge);
	if ( !fss )
	    continue;

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


void MPE::FaultStickSetEditor::getPidsOnStick( EM::PosID& insertpid,
				int sticknr, const Coord3& mousepos ) const
{
    EM::PosID nearestpid0 = EM::PosID::udf();
    EM::PosID nearestpid1 = EM::PosID::udf();
    insertpid = EM::PosID::udf();

    if ( !mousepos.isDefined() )
	return;

    ConstRefMan<EM::EMObject> emobject = emObject();
    if ( !emobject )
	return;

    const Geometry::Element* ge = emobject->geometryElement();
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

    nearestpid0.setObjectID( emobject->id() );
    nearestpid0.setSubID(
	RowCol(sticknr, definedknots[nearestknotidx]).toInt64() );

    if ( definedknots.size()<=1 )
    {
	const int defcol = definedknots[nearestknotidx];
	const Coord3 pos = fss->getKnot( RowCol(sticknr, defcol) );

        const bool isstickvertical = fss->getEditPlaneNormal(sticknr).z_ < 0.5;
	const int insertcol = defcol + ( isstickvertical
		    ? mousepos.z_>pos.z_ ? 1 : -1
		    : mousepos.coord()>pos.coord() ? 1 : -1) * colrange.step_;

	insertpid.setObjectID( emobject->id() );
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
            const int insertcol = definedknots[nearestknotidx]-colrange.step_;
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
            const int insertcol = definedknots[nearestknotidx]+colrange.step_;
	    insertpid.setSubID( RowCol(sticknr,insertcol).toInt64() );
	}
    }
}


void MPE::FaultStickSetEditor::cloneMovingNode( CallBacker* )
{
    setLastClicked( movingnode );
    RefMan<EM::EMObject> emobject = emObject();
    mDynamicCastGet(EM::FaultStickSet*,emfss,emobject.ptr());
    if ( !emfss )
	return;

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
    for ( int col=colrg.start_; col<=colrg.stop_; col+=colrg.step_ )
    {
	const RowCol currc( sticknr, col );
        const RowCol prevrc( sticknr, col-colrg.step_ );
	const EM::PosID prevpid( emfss->id(), prevrc.toInt64() );

	if ( currc.toInt64() == insertpid.subID() )
	{
	    ObjectEditor::setPosition( prevpid, startpos );
	    break;
	}

	const Coord3 prevpos = fss->getKnot( currc );
        if ( col == colrg.start_ )
	    fssg.insertKnot( prevrc.toInt64(), prevpos, true );
	else
	    ObjectEditor::setPosition( prevpid, prevpos );
    }
    emfss->setBurstAlert( false );
}


EM::PosID& MPE::FaultStickSetEditor::lastclickedpid()
{
    static EM::PosID lastclickedpid_;
    return lastclickedpid_;
}
