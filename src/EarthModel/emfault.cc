/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        J.C. Glas
 Date:          January 2010
________________________________________________________________________

-*/

#include "emfault.h"

#include "emfaultstickset.h"
#include "emmanager.h"
#include "survinfo.h"


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


const Coord3& FaultGeometry::getEditPlaneNormal( int sticknr ) const
{
    mDynamicCastGet(const Geometry::FaultStickSet*,fss,geometryElement())
    return fss ? fss->getEditPlaneNormal(sticknr) : Coord3::udf();
}


void FaultGeometry::copySelectedSticksTo( FaultStickSetGeometry& destfssg,
			bool addtohistory ) const
{
    Geometry::FaultStickSet* destfss = destfssg.geometryElement();
    int sticknr = destfss->isEmpty() ? 0 : destfss->rowRange().stop+1;

    mDynamicCastGet(const Geometry::FaultStickSet*,srcfss,geometryElement())
    if ( !srcfss )
	return;

    const StepInterval<int> rowrg = srcfss->rowRange();
    if ( rowrg.isUdf() )
	return;

    RowCol rc;
    for ( rc.row()=rowrg.start; rc.row()<=rowrg.stop; rc.row()+=rowrg.step )
    {
	if ( !srcfss->isStickSelected(rc.row()) )
	    continue;

	const StepInterval<int> colrg = srcfss->colRange( rc.row() );
	if ( colrg.isUdf() )
	    continue;

	int knotnr = 0;

	for ( rc.col()=colrg.start; rc.col()<=colrg.stop;
				    rc.col()+=colrg.step )
	{
	    const Coord3 pos = srcfss->getKnot( rc );

	    if ( rc.col() == colrg.start )
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
	if ( !doublesref || nrStickDoubles(sticknr,doublesref) )
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
    for ( rc.row()=rowrg.start; rc.row()<=rowrg.stop; rc.row()+=rowrg.step )
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

	for ( rc.col()=colrg.start; rc.col()<=colrg.stop;
				    rc.col()+=colrg.step )
	{

	    if ( rc.col() == colrg.stop )
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
    for ( rc.row()=rowrg.start; rc.row()<=rowrg.stop; rc.row()+=rowrg.step )
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

    return fabs(pos1.z -pos2.z) < 0.1 * SI().zStep();
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
	return -1;

    const StepInterval<int> rowrg = reffss->rowRange();
    if ( rowrg.isUdf() )
	return -1;

    RowCol rc;
    for ( rc.row()=rowrg.start; rc.row()<=rowrg.stop; rc.row()+=rowrg.step )
    {
	const StepInterval<int> colrg = reffss->colRange( rc.row() );
	if ( colrg.isUdf() || colrg.width()!=srccolrg.width() )
	    continue;

	RowCol uprc( sticknr, srccolrg.start);
	RowCol downrc( sticknr, srccolrg.stop);
	for ( rc.col()=colrg.start; rc.col()<=colrg.stop;
				    rc.col()+=colrg.step )
	{
	    if ( isSameKnot(srcfss->getKnot(uprc), reffss->getKnot(rc)) )
		uprc.col() += srccolrg.step;
	    if ( isSameKnot(srcfss->getKnot(downrc), reffss->getKnot(rc)) )
		downrc.col() -= srccolrg.step;
	}
	if ( uprc.col()>srccolrg.stop || downrc.col()<srccolrg.start )
	    nrdoubles++;
    }

    return ref==this ? nrdoubles-1 : nrdoubles;
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
{ }


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
{ }


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
