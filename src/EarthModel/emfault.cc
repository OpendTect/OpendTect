/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        J.C. Glas
 Date:          January 2010
________________________________________________________________________

-*/
static const char* rcsID = "$Id: emfault.cc,v 1.55 2010-02-04 17:20:24 cvsjaap Exp $";

#include "emfault.h"

#include "emfaultstickset.h"


namespace EM {


const Coord3& FaultGeometry::getEditPlaneNormal( const SectionID& sid,
						 int sticknr ) const
{
    mDynamicCastGet( const Geometry::FaultStickSet*,fss,sectionGeometry(sid) );
    return fss ? fss->getEditPlaneNormal(sticknr) : Coord3::udf();
}


void FaultGeometry::copySelectedSticksTo( FaultStickSetGeometry& tofssg,
					  const SectionID& tosid ) const
{
    Geometry::FaultStickSet* tofss = tofssg.sectionGeometry( tosid );
    int sticknr = tofss->isEmpty() ? 0 : tofss->rowRange().stop+1;

    for ( int sidx=0; sidx<nrSections(); sidx++ )
    {
	const int sid = sectionID( sidx );
	mDynamicCastGet( const Geometry::FaultStickSet*, fromfss,
			 sectionGeometry(sid) );
	if ( !fromfss )
	    continue;

	const StepInterval<int> rowrg = fromfss->rowRange();
	if ( rowrg.isUdf() )
	    continue;

	RowCol rc;
	for ( rc.row=rowrg.start; rc.row<=rowrg.stop; rc.row+=rowrg.step )
	{
	    if ( !fromfss->isStickSelected(rc.row) )
		continue;

	    const StepInterval<int> colrg = fromfss->colRange( rc.row );
	    if ( colrg.isUdf() )
		continue;

	    int knotnr = 0;

	    for ( rc.col=colrg.start; rc.col<=colrg.stop; rc.col+=colrg.step )
	    {
		const Coord3 pos = fromfss->getKnot( rc );

		if ( rc.col == colrg.start )
		{
		    tofssg.insertStick( tosid, sticknr, knotnr, pos,
					getEditPlaneNormal(sid,rc.row),
					lineSet(sid,rc.row),
					lineName(sid,rc.row), true );
		}
		else
		{
		    const RowCol torc( sticknr,knotnr );
		    tofssg.insertKnot( tosid, torc.getSerialized(), pos, true );
		}
		knotnr++;
	    }
	    sticknr++; 
	}
    }
}


void FaultGeometry::selectAllSticks( bool select )
{
    PtrMan<EM::EMObjectIterator> iter = createIterator(-1);
    while ( true )
    {
	EM::PosID pid = iter->next();
	if ( pid.objectID() == -1 )
	    break;

	const int sticknr = RowCol( pid.subID() ).row;
	const EM::SectionID sid = pid.sectionID();
	mDynamicCastGet( Geometry::FaultStickSet*, fss, sectionGeometry(sid) );
	fss->selectStick( sticknr, select );
    }
}


bool FaultGeometry::removeNextSelStick()
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

	    const StepInterval<int> colrg = fss->colRange( rc.row );
	    if ( colrg.isUdf() )
		continue;

	    for ( rc.col=colrg.start; rc.col<=colrg.stop; rc.col+=colrg.step )
	    {

		if ( rc.col == colrg.stop )
		    removeStick( sid, rc.row, true );
		else
		    removeKnot( sid, rc.getSerialized(), true );
	    }

	    if ( nrSections()>1 && !fss->nrSticks() )
		removeSection( sid, false );

	    return true;
	}
    }
    return false;
}


void FaultGeometry::removeSelectedSticks()
{ while ( removeNextSelStick() ); }


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


} // namespace EM
