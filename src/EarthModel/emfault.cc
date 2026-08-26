/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "emfault.h"

#include "ctxtioobj.h"
#include "emfaultstickset.h"
#include "emmanager.h"
#include "emsurfacetr.h"
#include "survinfo.h"
#include "transl.h"


namespace EM {


Fault::Fault( EMManager& emm )
    : Surface(emm)
{}


Fault::~Fault()
{}


void Fault::removeAll()
{
    Surface::removeAll();
    geometry().removeAll();
}


// FaultGeometry
FaultGeometry::FaultGeometry( Surface& surf )
    : SurfaceGeometry(surf)
{}


FaultGeometry::~FaultGeometry()
{}


FaultGeometry& FaultGeometry::operator =( const FaultGeometry& oth )
{
    auto* geoelm = geometryElement();
    const auto* srcgeoelm = oth.geometryElement();
    mDynamicCastGet( Geometry::FaultStickSurface*, fssurf, geoelm );
    mDynamicCastGet( const Geometry::FaultStickSurface*, srcfssurf, srcgeoelm );

    // The following checks that the geometry is EM::Fault3D's geometryElement.
    // For EM::FaultStickSet a new case should be added here.
    if ( !fssurf || !srcfssurf )
	return *this;

    fssurf->blockCallBacks( true, false );

    *fssurf = *srcfssurf;

    fssurf->blockCallBacks( false, true );

    setEnvelope( oth.getEnvelope() );

    changed_ = true;
    surface_.setChangedFlag();

    EMObjectCallbackData cbdata;
    cbdata.pid0 = PosID( surface_.id(), SectionID::def() );
    cbdata.event = EMObjectCallbackData::SectionChange;
    surface_.change.trigger( cbdata );

    cbdata.event = EMObjectCallbackData::BurstAlert;
    surface_.change.trigger( cbdata );

    return *this;
}


const Coord3& FaultGeometry::getEditPlaneNormal( int sticknr ) const
{
    mDynamicCastGet(const Geometry::FaultStickSet*,fss,geometryElement())
    return fss ? fss->getEditPlaneNormal(sticknr) : Coord3::udf();
}


void FaultGeometry::copySelectedSticksTo( FaultStickSetGeometry& destfssg,
			bool addtohistory ) const
{
    Geometry::FaultStickSet* destfss = destfssg.geometryElement();
    int sticknr = destfss->isEmpty() ? 0 : destfss->rowRange().stop_+1;

    mDynamicCastGet(const Geometry::FaultStickSet*,srcfss,geometryElement())
    if ( !srcfss )
	return;

    const StepInterval<int> rowrg = srcfss->rowRange();
    if ( rowrg.isUdf() )
	return;

    RowCol rc;
    for ( rc.row()=rowrg.start_; rc.row()<=rowrg.stop_; rc.row()+=rowrg.step_ )
    {
	if ( !srcfss->isStickSelected(rc.row()) )
	    continue;

	const StepInterval<int> colrg = srcfss->colRange( rc.row() );
	if ( colrg.isUdf() )
	    continue;

	int knotnr = 0;

	for ( rc.col()=colrg.start_; rc.col()<=colrg.stop_;
	      rc.col()+=colrg.step_ )
	{
	    const Coord3 pos = srcfss->getKnot( rc );

	    if ( rc.col() == colrg.start_ )
	    {
		destfssg.insertStick( sticknr, knotnr, pos,
				      getEditPlaneNormal(rc.row()),
				      pickedMultiID(rc.row()),
				      pickedName(rc.row()),
				      addtohistory );
	    }
	    else
	    {
		const RowCol destrc( sticknr,knotnr );
		destfssg.insertKnot( destrc.toInt64(), pos,
				     addtohistory );
	    }
	    knotnr++;
	}
	sticknr++;
    }
}


TrcKeyZSampling FaultGeometry::getEnvelope() const
{
    const TrcKeyZSampling tksz;
    return tksz;
}


void FaultGeometry::selectAllSticks( bool select )
{ selectSticks( select ); }


void FaultGeometry::selectStickDoubles( bool select, const FaultGeometry* ref )
{ selectSticks( select, (ref ? ref : this) ); }


void FaultGeometry::selectSticks( bool select, const FaultGeometry* doublesref )
{
    PtrMan<EM::EMObjectIterator> iter = createIterator();
    while ( true )
    {
	EM::PosID pid = iter->next();
	if ( !pid.isValid() )
	    break;

	const int sticknr = pid.getRowCol().row();
	mDynamicCastGet(Geometry::FaultStickSet*,fss,geometryElement())
	if ( !doublesref || nrStickDoubles(sticknr,doublesref)>0 )
	    fss->selectStick( sticknr, select );
    }
}


void FaultGeometry::removeSelectedSticks( bool addtohistory )
{ while ( removeSelStick(0,addtohistory) ) ; }


void FaultGeometry::removeSelectedDoubles( bool addtohistory,
					   const FaultGeometry* ref )
{
    for ( int selidx=nrSelectedSticks()-1; selidx>=0; selidx-- )
	removeSelStick( selidx, addtohistory, (ref ? ref : this) );
}


bool FaultGeometry::removeSelStick( int selidx, bool addtohistory,
				    const FaultGeometry* doublesref)
{
    mDynamicCastGet(Geometry::FaultStickSet*,fss,geometryElement())
    if ( !fss )
	return false;

    const StepInterval<int> rowrg = fss->rowRange();
    if ( rowrg.isUdf() )
	return false;

    RowCol rc;
    for ( rc.row()=rowrg.start_; rc.row()<=rowrg.stop_; rc.row()+=rowrg.step_ )
    {
	if ( !fss->isStickSelected(rc.row()) )
	    continue;

	if ( selidx )
	{
	    selidx--;
	    continue;
	}

	if ( doublesref && !nrStickDoubles(rc.row(),doublesref) )
	    return false;

	const StepInterval<int> colrg = fss->colRange( rc.row() );
	if ( colrg.isUdf() )
	    continue;

	for ( rc.col()=colrg.start_; rc.col()<=colrg.stop_;
	      rc.col()+=colrg.step_ )
	{

	    if ( rc.col() == colrg.stop_ )
		removeStick( rc.row(), addtohistory );
	    else
		removeKnot( rc.toInt64(), addtohistory );
	}

	return true;
    }

    return false;
}


int FaultGeometry::nrSelectedSticks() const
{
    int nrselectedsticks = 0;
    mDynamicCastGet(const Geometry::FaultStickSet*,fss,geometryElement())
    if ( !fss )
	return 0;

    const StepInterval<int> rowrg = fss->rowRange();
    if ( rowrg.isUdf() )
	return 0;

    RowCol rc;
    for ( rc.row()=rowrg.start_; rc.row()<=rowrg.stop_; rc.row()+=rowrg.step_ )
    {
	if ( fss->isStickSelected(rc.row()) )
	    nrselectedsticks++;
    }

    return nrselectedsticks;
}


static bool isSameKnot( Coord3 pos1, Coord3 pos2 )
{
    if ( pos1.Coord::distTo(pos2) > 0.1 * SI().crlDistance() )
	return false;

    return fabs(pos1.z_ -pos2.z_) < 0.1 * SI().zStep();
}


int FaultGeometry::nrStickDoubles( int sticknr,
				   const FaultGeometry* doublesref ) const
{
    int nrdoubles = 0;
    const FaultGeometry* ref = doublesref ? doublesref : this;

    mDynamicCastGet(const Geometry::FaultStickSet*,srcfss,geometryElement())

    const StepInterval<int> srccolrg = srcfss->colRange( sticknr );
    if ( srccolrg.isUdf() )
	return -1;

    mDynamicCastGet(const Geometry::FaultStickSet*,reffss,
		    ref->geometryElement())
    if ( !reffss )
	return 0;

    const StepInterval<int> rowrg = reffss->rowRange();
    if ( rowrg.isUdf() )
	return 0;

    RowCol rc;
    for ( rc.row()=rowrg.start_; rc.row()<=rowrg.stop_; rc.row()+=rowrg.step_ )
    {
	const StepInterval<int> colrg = reffss->colRange( rc.row() );
	if ( colrg.isUdf() || colrg.width()!=srccolrg.width() )
	    continue;

	RowCol uprc( sticknr, srccolrg.start_);
	RowCol downrc( sticknr, srccolrg.stop_);
	for ( rc.col()=colrg.start_; rc.col()<=colrg.stop_;
	      rc.col()+=colrg.step_ )
	{
	    if ( isSameKnot(srcfss->getKnot(uprc), reffss->getKnot(rc)) )
		uprc.col() += srccolrg.step_;
	    if ( isSameKnot(srcfss->getKnot(downrc), reffss->getKnot(rc)) )
		downrc.col() -= srccolrg.step_;
	}
	if ( uprc.col()>srccolrg.stop_ || downrc.col()<srccolrg.start_ )
	    nrdoubles++;
    }

    return ref==this ? nrdoubles-1 : nrdoubles;
}


bool FaultGeometry::insertStick( const SectionID&, int sticknr, int firstcol,
				 const Coord3& pos, const Coord3& editnormal,
				 bool addtohistory )
{
    return insertStick( sticknr, firstcol, pos, editnormal, addtohistory );
}


bool FaultGeometry::insertKnot( const SectionID&, const SubID& subid,
				const Coord3& pos, bool addtohistory )
{
    return insertKnot( subid, pos, addtohistory );
}


bool FaultGeometry::removeStick( const SectionID&, int sticknr,
				 bool addtohistory )
{
    return removeSelStick( sticknr, addtohistory );
}


bool FaultGeometry::removeKnot( const SectionID&, const SubID& subid,
				bool addtohistory )
{
    return removeKnot( subid, addtohistory );
}

const Coord3& FaultGeometry::getEditPlaneNormal( const SectionID&,
						 int sticknr ) const
{
    return getEditPlaneNormal( sticknr );
}


const MultiID* FaultGeometry::pickedMultiID( const SectionID&,
					     int sticknr ) const
{
    return pickedMultiID( sticknr );
}


const char* FaultGeometry::pickedName( const SectionID&, int sticknr ) const
{
    return pickedName( sticknr );
}


void FaultGeometry::copySelectedSticksTo( FaultStickSetGeometry& dest,
				const SectionID&, bool addtohistory ) const
{
    copySelectedSticksTo( dest, addtohistory );
}


int FaultGeometry::nrStickDoubles( const SectionID&, int sticknr,
				   const FaultGeometry* ref ) const
{
    return nrStickDoubles( sticknr, ref );
}


// FaultStickUndoEvent
FaultStickUndoEvent::FaultStickUndoEvent( const EM::PosID& posid )
    : posid_( posid )
    , remove_( false )
{
    RefMan<EMObject> emobj = EMM().getObject( posid_.objectID() );
    mDynamicCastGet( Fault*, fault, emobj.ptr() );
    if ( !fault ) return;

    pos_ = fault->getPos( posid_ );
    const int row = posid.getRowCol().row();
    normal_ = fault->geometry().getEditPlaneNormal( row );
}


FaultStickUndoEvent::FaultStickUndoEvent( const EM::PosID& posid,
				const Coord3& oldpos, const Coord3& oldnormal )
    : posid_( posid )
    , pos_( oldpos )
    , normal_( oldnormal )
    , remove_( true )
{}


FaultStickUndoEvent::~FaultStickUndoEvent()
{}


const char* FaultStickUndoEvent::getStandardDesc() const
{ return remove_ ? "Remove stick" : "Insert stick"; }


bool FaultStickUndoEvent::unDo()
{
    RefMan<EMObject> emobj = EMM().getObject( posid_.objectID() );
    mDynamicCastGet( Fault*, fault, emobj.ptr() );
    if ( !fault ) return false;

    const int row = posid_.getRowCol().row();

    return remove_
	? fault->geometry().insertStick( row,
		posid_.getRowCol().col(), pos_, normal_, false )
	: fault->geometry().removeStick( row, false );
}


bool FaultStickUndoEvent::reDo()
{
    RefMan<EMObject> emobj = EMM().getObject( posid_.objectID() );
    mDynamicCastGet( Fault*, fault, emobj.ptr() );
    if ( !fault ) return false;

    const int row = posid_.getRowCol().row();

    return remove_
	? fault->geometry().removeStick( row, false )
	: fault->geometry().insertStick( row,
		posid_.getRowCol().col(), pos_, normal_, false );
}



// FaultKnotUndoEvent
FaultKnotUndoEvent::FaultKnotUndoEvent( const EM::PosID& posid )
    : posid_( posid )
    , remove_( false )
{
    RefMan<EMObject> emobj = EMM().getObject( posid_.objectID() );
    if ( !emobj ) return;
    pos_ = emobj->getPos( posid_ );
}


FaultKnotUndoEvent::FaultKnotUndoEvent( const EM::PosID& posid,
					const Coord3& oldpos )
    : posid_( posid )
    , pos_( oldpos )
    , remove_( true )
{}


FaultKnotUndoEvent::~FaultKnotUndoEvent()
{}

const char* FaultKnotUndoEvent::getStandardDesc() const
{ return remove_ ? "Remove knot" : "Insert knot"; }


bool FaultKnotUndoEvent::unDo()
{
    RefMan<EMObject> emobj = EMM().getObject( posid_.objectID() );
    mDynamicCastGet( Fault*, fault, emobj.ptr() );
    if ( !fault ) return false;

    return remove_
	? fault->geometry().insertKnot( posid_.subID(), pos_, false )
	: fault->geometry().removeKnot( posid_.subID(), false );
}


bool FaultKnotUndoEvent::reDo()
{
    RefMan<EMObject> emobj = EMM().getObject( posid_.objectID() );
    mDynamicCastGet( Fault*, fault, emobj.ptr() );
    if ( !fault ) return false;

    return remove_
	? fault->geometry().removeKnot( posid_.subID(), false )
	: fault->geometry().insertKnot( posid_.subID(), pos_, false );
}

} // namespace EM
