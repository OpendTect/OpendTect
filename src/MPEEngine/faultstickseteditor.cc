/*
___________________________________________________________________

 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : J.C. Glas
 * DATE     : October 2008
___________________________________________________________________

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


ObjectEditor* FaultStickSetEditor::create( EM::Object& emobj )
{
    mDynamicCastGet(EM::FaultStickSet*,emfss,&emobj);
    if ( !emfss ) return 0;
    return new FaultStickSetEditor( *emfss );
}


void FaultStickSetEditor::initClass()
{
    MPE::ObjectEditor::factory().addCreator( create,
					     EM::FaultStickSet::typeStr() );
}


Geometry::ElementEditor* FaultStickSetEditor::createEditor()
{
    const Geometry::Element* ge = emObject().geometryElement();
    if ( !ge ) return 0;

    mDynamicCastGet(const Geometry::FaultStickSet*,fss,ge);
    if ( !fss ) return 0;

    return new Geometry::StickSetEditor(
			*const_cast<Geometry::FaultStickSet*>(fss) );
}


const EM::FaultStickSet* FaultStickSetEditor::emFaultStickSet() const
{
    mDynamicCastGet(const EM::FaultStickSet*,emfss,emobject_.ptr());
    return emfss;
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


static EM::PosID lastclickedpid_ = EM::PosID::getInvalid();

void FaultStickSetEditor::setLastClicked( const EM::PosID& pid )
{
    lastclickedpid_ = pid;

    EM::FaultStickSet* emfss =const_cast<EM::FaultStickSet*>(emFaultStickSet());
    if ( emfss )
	emfss->preferStick( pid.getRowCol().row()  );

    if ( sowingpivot_.isDefined() )
    {
	const Coord3 pos = emObject().getPos( pid );
	if ( pos.isDefined() )
	    sowinghistory_.insert( 0, pos );
    }
}


int FaultStickSetEditor::getLastClickedStick() const
{
    const EM::FaultStickSet* emfss = emFaultStickSet();
    if ( emfss )
    {
	const int lastclickedsticknr = lastclickedpid_.getRowCol().row();
	if ( lastclickedsticknr == emfss->preferredStickNr() )
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
    Coord3( crd.x_, crd.y_, SI().zScale()*crd.z_ )

#define mCustomScale(crd) \
    Coord3( crd.x_, Coord(scalevector_.getXY()).dot(crd.getXY()), \
	    scalevector_.z_*crd.z_ )


float FaultStickSetEditor::distToStick( int sticknr,const DBKey* pickeddbkey,
			const char* pickednm, GeomID pickedgeomid,
			const Coord3& mousepos, const Coord3* posnormal ) const
{

    const EM::FaultStickSet* emfss = emFaultStickSet();
    if ( !emfss || emfss->isStickHidden(sticknr,sceneidx_)
		|| !mousepos.isDefined() )
	return mUdf(float);

    if ( emfss->pickedOn2DLine(sticknr) )
    {
	const GeomID geomid = emfss->pickedGeomID( sticknr );
	if ( geomid != pickedgeomid )
	    return mUdf(float);
    }
    else if ( !emfss->pickedOnPlane(sticknr) )
    {
	const DBKey* mid = emfss->pickedDBKey( sticknr );
	if ( !pickeddbkey || !mid || *pickeddbkey!=*mid )
	    return mUdf(float);

	const FixedString nm( emfss->pickedName(sticknr) );
	if ( (pickednm || nm) && ( nm != pickednm ) )
	   return mUdf(float);
    }

    const StepInterval<int> colrange = emfss->colRange( sticknr );
    if ( colrange.isUdf() )
	return mUdf(float);

    const Plane3 plane( emfss->getEditPlaneNormal(sticknr),
			mWorldScale(mousepos), false );

    if ( posnormal && !posnormal->isUdf() &&
	 fabs( posnormal->dot(plane.normal()) ) < 0.5 )
	return mUdf(float);

    const double onestepdist =
	   mWorldScale( SI().oneStepTranslation(plane.normal()) ).abs<double>();

    bool insameplane = false;
    double prevdist = 0.0;
    Coord3 avgpos( 0, 0, 0 );
    int count = 0;
    MonitorLock ml( *emfss );
    for ( int knotidx=colrange.nrSteps(); knotidx>=0; knotidx-- )
    {
	const RowCol rc( sticknr, colrange.atIndex(knotidx) );
	const Coord3 pos = emfss->getKnot( rc );
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

    if ( emfss->pickedOnPlane(sticknr) && !insameplane )
	return mUdf(float);

    avgpos /= count;

    return mCustomScale(avgpos).xyDistTo<float>( mCustomScale(mousepos) );
}


void FaultStickSetEditor::getInteractionInfo( EM::PosID& insertpid,
			const DBKey* pickeddbkey, const char* pickednm,
			GeomID pickedgeomid, const Coord3& mousepos,
			const Coord3* posnormal ) const
{
    insertpid = EM::PosID::getInvalid();

    int sticknr = getLastClickedStick();

    const Coord3& pos = sowingpivot_.isDefined() && sowinghistory_.isEmpty()
			? sowingpivot_ : mousepos;

    if ( !mIsUdf(sticknr) )
    {
	const float dist = distToStick( sticknr, pickeddbkey, pickednm,
					pickedgeomid, pos, posnormal );
	if ( !mIsUdf(dist) )
	{
	    getPidsOnStick( insertpid, sticknr, pos );
	    return;
	}
    }

    if ( getNearestStick(sticknr,pickeddbkey,pickednm,pickedgeomid,pos,
			 posnormal) )
	getPidsOnStick( insertpid, sticknr, pos );
}


const EM::PosID FaultStickSetEditor::getNearestStick( const Coord3& mousepos,
    GeomID pickedgeomid, const Coord3* normal ) const
{
    EM::PosID pid = EM::PosID::getInvalid();
    int sticknr = getLastClickedStick();
    const Coord3& pos = sowingpivot_.isDefined() && sowinghistory_.isEmpty()
			? sowingpivot_ : mousepos;

    DBKey pickeddbkey;

    if ( getNearestStick(sticknr,&pickeddbkey,"",pickedgeomid,pos, normal) )
	getPidsOnStick( pid, sticknr, pos );

    return pid;
}


bool FaultStickSetEditor::removeSelection( const Selector<Coord3>& selector )
{
    EM::FaultStickSet* emfss =const_cast<EM::FaultStickSet*>(emFaultStickSet());
    bool change = false;

    const StepInterval<int> rowrange = emfss->rowRange();
    if ( rowrange.isUdf() )
	return false;

    for ( int stickidx=rowrange.nrSteps(); stickidx>=0; stickidx-- )
    {
	const int curstick = rowrange.atIndex(stickidx);
	const StepInterval<int> colrange = emfss->colRange( curstick );
	if ( emfss->isStickHidden(curstick,sceneidx_) || colrange.isUdf() )
	    continue;

	for ( int knotidx=colrange.nrSteps(); knotidx>=0; knotidx-- )
	{
	    const RowCol rc( curstick,colrange.atIndex(knotidx) );
	    const Coord3 pos = emfss->getKnot( rc );

	    if ( !pos.isDefined() || !selector.includes(pos) )
		continue;

	    const bool res = emfss->nrKnots(curstick)==1
		? emfss->removeStick( curstick, true )
		: emfss->removeKnot( EM::PosID::getFromRowCol(rc), true );

	    if ( res )
		change = true;
	}
    }

    if ( change )
    {
	EM::FSSMan().undo(emfss->id()).setUserInteractionEnd(
			EM::FSSMan().undo(emfss->id()).currentEventID() );
    }

    return change;
}


bool FaultStickSetEditor::getNearestStick( int& sticknr,
			const DBKey* pickeddbkey, const char* pickednm,
			GeomID pickedgeomid, const Coord3& mousepos,
			const Coord3* posnormal) const
{
    const EM::FaultStickSet* emfss = emFaultStickSet();
    if ( !emfss || !mousepos.isDefined() )
	return false;

    int selsticknr = mUdf(int);
    float minlinedist = mUdf(float);

    const StepInterval<int> rowrange = emfss->rowRange();
    if ( rowrange.isUdf() )
	return false;

    for ( int stickidx=rowrange.nrSteps(); stickidx>=0; stickidx-- )
    {
	const int cursticknr = rowrange.atIndex(stickidx);
	const float disttoline = distToStick( cursticknr, pickeddbkey, pickednm,
					    pickedgeomid, mousepos, posnormal );
	if ( !mIsUdf(disttoline) )
	{
	    if ( mIsUdf(minlinedist) || disttoline<minlinedist )
	    {
		minlinedist = disttoline;
		selsticknr = cursticknr;

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
    EM::PosID nearestpid0 = EM::PosID::getInvalid();
    EM::PosID nearestpid1 = EM::PosID::getInvalid();
    insertpid = EM::PosID::getInvalid();
    const EM::FaultStickSet* emfss = emFaultStickSet();
    if ( !emfss || !mousepos.isDefined() )
	return;

    const StepInterval<int> colrange = emfss->colRange( sticknr );
    const int nrknots = colrange.nrSteps()+1;

    TypeSet<int> definedknots;
    int nearestknotidx = -1;
    float minsqdist = mUdf(float);
    for ( int knotidx=0; knotidx<nrknots; knotidx++ )
    {
	const RowCol rc( sticknr, colrange.atIndex(knotidx));
	const Coord3 pos = emfss->getKnot( rc );

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

    nearestpid0 = EM::PosID::getFromRowCol( sticknr,
					definedknots[nearestknotidx] );

    if ( definedknots.size()<=1 )
    {
	const int defcol = definedknots[nearestknotidx];
	const Coord3 pos = emfss->getKnot( RowCol(sticknr, defcol) );

	const bool isstickvertical =
				    emfss->getEditPlaneNormal(sticknr).z_ < 0.5;
	const int insertcol = defcol + ( isstickvertical
	    ? mousepos.z_>pos.z_ ? 1 : -1
	    : mousepos.getXY()>pos.getXY() ? 1 : -1) * colrange.step;

	insertpid = EM::PosID::getFromRowCol( sticknr, insertcol );
	return;
    }

    const Coord3 pos =
	emfss->getKnot( RowCol(sticknr,definedknots[nearestknotidx]) );

    Coord3 nextpos = pos, prevpos = pos;

    if ( nearestknotidx )
	prevpos =
		emfss->getKnot( RowCol(sticknr,definedknots[nearestknotidx-1]));

    if ( nearestknotidx<definedknots.size()-1 )
	nextpos =
		emfss->getKnot( RowCol(sticknr,definedknots[nearestknotidx+1]));

    Coord3 v0 = nextpos-prevpos;
    Coord3 v1 = mousepos-pos;

    bool takeprevious = mCustomScale(v0).dot( mCustomScale(v1) ) < 0;
    if ( sowinghistory_.size() > 1 )
	takeprevious = sowinghistory_[1]==prevpos;

    if ( takeprevious )
    {
	if ( nearestknotidx )
	{
	    nearestpid1 = EM::PosID::getFromRowCol( sticknr,
				definedknots[nearestknotidx-1] );
	    insertpid = nearestpid0;
	}
	else
	{
	    const int insertcol = definedknots[nearestknotidx]-colrange.step;
	    insertpid = EM::PosID::getFromRowCol( sticknr, insertcol );
	}
    }
    else // take next
    {
	if ( nearestknotidx<definedknots.size()-1 )
	{
	    nearestpid1 = EM::PosID::getFromRowCol( sticknr,
					definedknots[nearestknotidx+1] );
	    insertpid = nearestpid1;
	}
	else
	{
	    const int insertcol = definedknots[nearestknotidx]+colrange.step;
	    insertpid = EM::PosID::getFromRowCol( sticknr, insertcol );
	}
    }
}


void FaultStickSetEditor::cloneMovingNode(CallBacker*)
{
    setLastClicked( movingnode_ );
    mDynamicCastGet( EM::FaultStickSet*, emfss, emobject_.ptr() );
    EM::FaultStickSetGeometry& fssg = emfss->geometry();
    const int sticknr = movingnode_.getRowCol().row();
    Geometry::FaultStickSet* fss = fssg.geometryElement();
    const DBKey* pickeddbkey = fssg.pickedDBKey( sticknr );
    const GeomID pickedgeomid = fssg.pickedGeomID( sticknr );
    const char* pickednm = fssg.pickedName( sticknr );
    const Coord3& normal = fss->getEditPlaneNormal( sticknr );
    EM::PosID insertpid;
    getInteractionInfo( insertpid, pickeddbkey, pickednm, pickedgeomid,
			startpos_, &normal );
    if ( insertpid.isInvalid() )
	return;

    if ( movingnode_ != insertpid )
    {
	fssg.insertKnot( insertpid, startpos_, true );
	return;
    }

    // Performs knot insertion without changing PosID of moving node
    emfss->setBurstAlert( true );
    const StepInterval<int> colrg = fss->colRange( sticknr );
    for ( int col=colrg.start; col<=colrg.stop; col+=colrg.step )
    {
	const RowCol currc( sticknr, col );
	const RowCol prevrc( sticknr, col-colrg.step );
	const EM::PosID prevpid = EM::PosID::getFromRowCol( prevrc );

	if ( EM::PosID::getFromRowCol(currc) == insertpid )
	{
	    ObjectEditor::setPosition( prevpid, startpos_ );
	    break;
	}

	const Coord3 prevpos = fss->getKnot( currc );
	if ( col == colrg.start )
	    fssg.insertKnot( prevpid, prevpos, true );
	else
	    ObjectEditor::setPosition( prevpid, prevpos );
    }

    emfss->setBurstAlert( false );
}


};  // namespace MPE
