/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        J.C. Glas
 Date:          January 2010
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "emfault.h"

#include "emfaultstickset.h"
#include "emmanager.h"
#include "survinfo.h"


namespace EM {


void Fault::removeAll()
{
    Surface::removeAll();
    geometry().removeAll();
}


const Coord3& FaultGeometry::getEditPlaneNormal( const SectionID& sid,
						 int sticknr ) const
{
    mDynamicCastGet( const Geometry::FaultStickSet*,fss,sectionGeometry(sid) );
    return fss ? fss->getEditPlaneNormal(sticknr) : Coord3::udf();
}


void FaultGeometry::copySelectedSticksTo( FaultStickSetGeometry& destfssg,
			  const SectionID& destsid, bool addtohistory ) const
{
    Geometry::FaultStickSet* destfss = destfssg.sectionGeometry( destsid );
    int sticknr = destfss->isEmpty() ? 0 : destfss->rowRange().stop+1;

    for ( int sidx=0; sidx<nrSections(); sidx++ )
    {
	const EM::SectionID sid = sectionID( sidx );
	mDynamicCastGet( const Geometry::FaultStickSet*, srcfss,
			 sectionGeometry(sid) );
	if ( !srcfss )
	    continue;

	const StepInterval<int> rowrg = srcfss->rowRange();
	if ( rowrg.isUdf() )
	    continue;

	RowCol rc;
	for ( rc.row=rowrg.start; rc.row<=rowrg.stop; rc.row+=rowrg.step )
	{
	    if ( !srcfss->isStickSelected(rc.row) )
		continue;

	    const StepInterval<int> colrg = srcfss->colRange( rc.row );
	    if ( colrg.isUdf() )
		continue;

	    int knotnr = 0;

	    for ( rc.col=colrg.start; rc.col<=colrg.stop; rc.col+=colrg.step )
	    {
		const Coord3 pos = srcfss->getKnot( rc );

		if ( rc.col == colrg.start )
		{
		    destfssg.insertStick( destsid, sticknr, knotnr, pos,
					  getEditPlaneNormal(sid,rc.row),
					  pickedMultiID(sid,rc.row),
					  pickedName(sid,rc.row),
					  addtohistory );
		}
		else
		{
		    const RowCol destrc( sticknr,knotnr );
		    destfssg.insertKnot( destsid, destrc.toInt64(), pos,
					 addtohistory );
		}
		knotnr++;
	    }
	    sticknr++; 
	}
    }
}


void FaultGeometry::selectAllSticks( bool select )
{ selectSticks( select ); }


void FaultGeometry::selectStickDoubles( bool select, const FaultGeometry* ref )
{ selectSticks( select, (ref ? ref : this) ); }


void FaultGeometry::selectSticks( bool select, const FaultGeometry* doublesref )
{
    PtrMan<EM::EMObjectIterator> iter = createIterator(-1);
    while ( true )
    {
	EM::PosID pid = iter->next();
	if ( pid.objectID() == -1 )
	    break;

	const int sticknr = pid.getRowCol().row;
	const EM::SectionID sid = pid.sectionID();
	mDynamicCastGet( Geometry::FaultStickSet*, fss, sectionGeometry(sid) );

	if ( !doublesref || nrStickDoubles(sid, sticknr, doublesref) )
	    fss->selectStick( sticknr, select );
    }
}


void FaultGeometry::removeSelectedSticks( bool addtohistory )
{ while ( removeSelStick(0,addtohistory) ); }


void FaultGeometry::removeSelectedDoubles( bool addtohistory,
					   const FaultGeometry* ref )
{
    for ( int selidx=nrSelectedSticks()-1; selidx>=0; selidx-- )
	removeSelStick( selidx, addtohistory, (ref ? ref : this) );
}


bool FaultGeometry::removeSelStick( int selidx, bool addtohistory,
				    const FaultGeometry* doublesref)
{
    for ( int sidx=nrSections()-1; sidx>=0; sidx-- )
    {
	const EM::SectionID sid = sectionID( sidx );
	mDynamicCastGet( Geometry::FaultStickSet*, fss, sectionGeometry(sid) );
	if ( !fss )
	    continue;

	const StepInterval<int> rowrg = fss->rowRange();
	if ( rowrg.isUdf() )
	    continue;

	RowCol rc;
	for ( rc.row=rowrg.start; rc.row<=rowrg.stop; rc.row+=rowrg.step )
	{
	    if ( !fss->isStickSelected(rc.row) )
		continue;

	    if ( selidx ) 
	    {
		selidx--;
		continue;
	    }

	    if ( doublesref && !nrStickDoubles(sid, rc.row, doublesref) )
		return false;

	    const StepInterval<int> colrg = fss->colRange( rc.row );
	    if ( colrg.isUdf() )
		continue;

	    for ( rc.col=colrg.start; rc.col<=colrg.stop; rc.col+=colrg.step )
	    {

		if ( rc.col == colrg.stop )
		    removeStick( sid, rc.row, addtohistory );
		else
		    removeKnot( sid, rc.toInt64(), addtohistory );
	    }

	    if ( nrSections()>1 && !fss->nrSticks() )
		removeSection( sid, false );

	    return true;
	}
    }
    return false;
}


int FaultGeometry::nrSelectedSticks() const
{
    int nrselectedsticks = 0;
    for ( int sidx=0; sidx<nrSections(); sidx++ )
    {
	mDynamicCastGet( const Geometry::FaultStickSet*, fss,
			 sectionGeometry(sectionID(sidx)) );
	if ( !fss )
	    continue;

	const StepInterval<int> rowrg = fss->rowRange();
	if ( rowrg.isUdf() )
	    continue;

	RowCol rc;
	for ( rc.row=rowrg.start; rc.row<=rowrg.stop; rc.row+=rowrg.step )
	{
	    if ( fss->isStickSelected(rc.row) )
		nrselectedsticks++;
	}
    }

    return nrselectedsticks;
}

static bool isSameKnot( Coord3 pos1, Coord3 pos2 )
{
    if ( pos1.Coord::distTo(pos2) > 0.1 * SI().crlDistance() )
	return false;

    return fabs(pos1.z -pos2.z) < 0.1 * SI().zStep();
}


int FaultGeometry::nrStickDoubles( const SectionID& sid, int sticknr,
				   const FaultGeometry* doublesref ) const
{
    int nrdoubles = 0;
    const FaultGeometry* ref = doublesref ? doublesref : this;

    mDynamicCastGet( const Geometry::FaultStickSet*, srcfss,
		     sectionGeometry(sid) );

    const StepInterval<int> srccolrg = srcfss->colRange( sticknr );
    if ( srccolrg.isUdf() )
	return -1;

    for ( int sidx=0; sidx<ref->nrSections(); sidx++ )
    {
	mDynamicCastGet( const Geometry::FaultStickSet*, reffss,
			 ref->sectionGeometry(ref->sectionID(sidx)) );
	if ( !reffss )
	    continue;

	const StepInterval<int> rowrg = reffss->rowRange();
	if ( rowrg.isUdf() )
	    continue;

	RowCol rc;
	for ( rc.row=rowrg.start; rc.row<=rowrg.stop; rc.row+=rowrg.step )
	{
	    const StepInterval<int> colrg = reffss->colRange( rc.row );
	    if ( colrg.isUdf() || colrg.width()!=srccolrg.width() )
		continue;

	    RowCol uprc( sticknr, srccolrg.start);
	    RowCol downrc( sticknr, srccolrg.stop);
	    for ( rc.col=colrg.start; rc.col<=colrg.stop; rc.col+=colrg.step )
	    {
		if ( isSameKnot(srcfss->getKnot(uprc), reffss->getKnot(rc)) )
		    uprc.col += srccolrg.step;
		if ( isSameKnot(srcfss->getKnot(downrc), reffss->getKnot(rc)) )
		    downrc.col -= srccolrg.step;
	    }
	    if ( uprc.col>srccolrg.stop || downrc.col<srccolrg.start )
		nrdoubles++;
	}
    }


    return ref==this ? nrdoubles-1 : nrdoubles;
}



FaultStickUndoEvent::FaultStickUndoEvent( const EM::PosID& posid )
    : posid_( posid )
    , remove_( false )
{
    RefMan<EMObject> emobj = EMM().getObject( posid_.objectID() );
    mDynamicCastGet( Fault*, fault, emobj.ptr() );
    if ( !fault ) return;

    pos_ = fault->getPos( posid_ );
    const int row = posid.getRowCol().row;
    normal_ = fault->geometry().getEditPlaneNormal( posid_.sectionID(), row );
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

    const int row = posid_.getRowCol().row;

    return remove_
	? fault->geometry().insertStick( posid_.sectionID(), row,
		posid_.getRowCol().col, pos_, normal_, false )
	: fault->geometry().removeStick( posid_.sectionID(), row, false );
}


bool FaultStickUndoEvent::reDo()
{
    RefMan<EMObject> emobj = EMM().getObject( posid_.objectID() );
    mDynamicCastGet( Fault*, fault, emobj.ptr() );
    if ( !fault ) return false;

    const int row = posid_.getRowCol().row;

    return remove_
	? fault->geometry().removeStick( posid_.sectionID(), row, false )
	: fault->geometry().insertStick( posid_.sectionID(), row,
		posid_.getRowCol().col, pos_, normal_, false );
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
	? fault->geometry().insertKnot( posid_.sectionID(), posid_.subID(),
					pos_, false )
	: fault->geometry().removeKnot( posid_.sectionID(), posid_.subID(),
					false );
}


bool FaultKnotUndoEvent::reDo()
{
    RefMan<EMObject> emobj = EMM().getObject( posid_.objectID() );
    mDynamicCastGet( Fault*, fault, emobj.ptr() );
    if ( !fault ) return false;

    return remove_
	? fault->geometry().removeKnot( posid_.sectionID(), posid_.subID(),
					false )
	: fault->geometry().insertKnot( posid_.sectionID(), posid_.subID(),
					pos_, false );
}


} // namespace EM
