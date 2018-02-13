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


namespace EM
{
#define mGetConstGeomFSS( ret ) \
    mDynamicCastGet(const Geometry::FaultStickSet*,geomfss,\
		geometry().geometryElement() ); \
    if ( !geomfss ) \
	return ret; \

#define mGetGeomFSS() \
mDynamicCastGet(Geometry::FaultStickSet*,geomfss,\
	    geometry().geometryElement() ); \
if ( !geomfss ) \
    return; \


StepInterval<int> Fault::rowRange() const
{
    mGetConstGeomFSS( StepInterval<int>::udf() );
    return geomfss->rowRange();
}


StepInterval<int> Fault::colRange( int row ) const
{
    mGetConstGeomFSS( StepInterval<int>::udf() );
    return geomfss->colRange( row );
}


Coord3 Fault::getKnot( RowCol rc ) const
{
    mGetConstGeomFSS( Coord3::udf() );
    return geomfss->getKnot( rc );
}


bool Fault::isKnotHidden( RowCol rc, int sceneidx ) const
{
    mGetConstGeomFSS( false );
    return geomfss->isKnotHidden( rc, sceneidx );
}



unsigned int Fault::totalSize() const
{
    mLock4Read();

    mDynamicCastGet( const Geometry::RowColSurface*, rcs,
		    geometry().geometryElement() );
    unsigned int tosz = (rcs->rowRange().nrSteps()+1)
			*(rcs->colRange().nrSteps()+1);
    return tosz;
}


bool Fault::insertStick(int sticknr,int firstcol,
				    const Coord3& pos,const Coord3& editnormal,
				    const DBKey* pickeddbkey,
				    const char* pickednm,bool addtohistory)
{
    mLock4Write();
    const bool ret = geometry().insertStick( sticknr, firstcol, pos,editnormal,
					pickeddbkey, pickednm, addtohistory );
    mSendEMCBNotif( Object::cBurstAlert() );
    return ret;
}


bool Fault::insertStick( int sticknr, int firstcol, const Coord3& pos,
			const Coord3& editnormal, Pos::GeomID pickedgeomid,
							    bool addtohistory )
{
     mLock4Write();
     const bool ret =  geometry().
		insertStick( sticknr, firstcol, pos,editnormal,addtohistory);
     mSendEMCBNotif( Object::cBurstAlert() );
     return ret;
}


bool Fault::insertStick(int sticknr,int firstcol,
				    const Coord3& pos,const Coord3& editnormal,
				    bool addtohistory)
{
     mLock4Write();
     const bool ret = geometry().
		insertStick( sticknr, firstcol, pos,editnormal,addtohistory);
     mSendEMCBNotif( Object::cBurstAlert() );
     return ret;
}


bool Fault::insertKnot( const PosID& posid, const Coord3& pos , bool adtoh )
{
    mLock4Write();
    const bool ret = geometry().insertKnot( posid, pos, adtoh );
    mSendEMCBNotif( Object::cBurstAlert() );
    return ret;
}


bool Fault::removeStick( int sticknr, bool addtohistory )
{
    mLock4Write();
    const bool ret =  geometry().removeStick( sticknr, addtohistory );
    mSendEMCBNotif( Object::cBurstAlert() );
    return ret;
}


bool Fault::removeKnot( const PosID& posid, bool addtoh )
{
    mLock4Write();
    const bool ret = geometry().removeKnot( posid, addtoh );
    mSendEMCBNotif( Object::cBurstAlert() );
    return ret;
}


Coord3 Fault::getEditPlaneNormal( int sticknr ) const
{
    mLock4Read();
    return geometry().getEditPlaneNormal( sticknr );
}


int Fault::nrSticks() const
{
    mLock4Read();
    mGetConstGeomFSS( -1 )
    return geomfss->nrSticks();
}


TypeSet<Coord3> Fault::getStick( int sticknr ) const
{
    TypeSet<Coord3> coords;
    mGetConstGeomFSS( coords );
    mLock4Read();
    coords.copy( *geomfss->getStick(sticknr) );
    return coords;
}


bool Fault::isStickHidden( int sticknr, int sceneidx ) const
{
    mGetConstGeomFSS( false );
    return geomfss->isStickHidden( sticknr, sceneidx );
}


void Fault::hideKnot( RowCol rc, bool yn, int scnidx )
{
    mLock4Write();
    hidKnot( rc, yn, scnidx );
}


void Fault::hideStick( int sticknr, bool yn, int scnidx )
{
    mLock4Write();
    hidStick( sticknr, yn, scnidx );
}


void Fault::hidKnot( RowCol rc, bool yn, int scnidx )
{
    mGetGeomFSS();
    geomfss->hideKnot( rc, yn, scnidx );
}


void Fault::hidStick( int sticknr, bool yn, int scnidx )
{
    mGetGeomFSS();
    geomfss->hideStick( sticknr, yn, scnidx );
}


void Fault::hideSticks( const TypeSet<int>& sticknrs, bool yn, int sceneidx )
{
    mLock4Write();
    for ( int idx=0; idx<sticknrs.size(); idx++ )
	hidStick( sticknrs[idx], yn, sceneidx );
}


void Fault::hideKnots( const TypeSet<RowCol>& rcs, bool yn, int sceneidx )
{
    mLock4Write();
    for ( int idx=0; idx<rcs.size(); idx++ )
	hidKnot( rcs[idx], yn, sceneidx );
}


void Fault::hideAllSticks( bool yn, int sceneidx )
{
    mLock4Write();
    mGetGeomFSS();
    geomfss->hideAllSticks( yn, sceneidx );
}


void Fault::hideAllKnots( bool yn, int sceneidx )
{
    mLock4Write();
    mGetGeomFSS();
    geomfss->hideAllKnots( yn, sceneidx );
}


void Fault::removeSelectedSticks( bool yn )
{
    mLock4Write();
    geometry().removeSelectedSticks( yn );
}


bool Fault::isStickSelected( int sticknr )
{
    mGetConstGeomFSS( false );
    return geomfss->isStickSelected( sticknr );
}


void Fault::selectStick( int sticknr, bool yn )
{
    mGetGeomFSS();
    mLock4Write();
    geomfss->selectStick( sticknr, yn );
}


void Fault::preferStick( int sticknr )
{
    mGetGeomFSS();
    mLock4Write();
    geomfss->preferStick( sticknr );
}


int Fault::preferredStickNr() const
{
    mGetConstGeomFSS( false );
    return geomfss->preferredStickNr();
}


void Fault::removeAll()
{
    Surface::removeAll();
    geometry().removeAll();
}


ObjectIterator* Fault::createIterator(const TrcKeyZSampling* tks ) const
{
    mLock4Write();
    return geometry().createIterator( tks );
}

//FaultGeometry
Coord3 FaultGeometry::getEditPlaneNormal( int sticknr ) const
{
    mDynamicCastGet( const Geometry::FaultStickSet*,fss,geometryElement() );
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
				      pickedDBKey(rc.row()),
				      pickedName(rc.row()),
				      addtohistory );
	    }
	    else
	    {
		const RowCol destrc( sticknr,knotnr );
		destfssg.insertKnot( PosID::getFromRowCol(destrc),
				     pos, addtohistory );
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
    PtrMan<EM::ObjectIterator> iter = createIterator();
    while ( true )
    {
	EM::PosID pid = iter->next();
	if ( pid.isInvalid() )
	    break;

	const int sticknr = pid.getRowCol().row();
	mDynamicCastGet( Geometry::FaultStickSet*, fss, geometryElement() );

	if ( !doublesref || nrStickDoubles( sticknr, doublesref) )
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
    mDynamicCastGet( Geometry::FaultStickSet*, fss, geometryElement() );
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

	if ( doublesref && !nrStickDoubles( rc.row(), doublesref) )
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
		removeKnot( PosID::getFromRowCol(rc), addtohistory );
	}

	return true;
    }

    return false;
}


int FaultGeometry::nrSelectedSticks() const
{
    int nrselectedsticks = 0;
    mDynamicCastGet( const Geometry::FaultStickSet*, fss, geometryElement() )
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
    if ( pos1.xyDistTo<float>(pos2) > 0.1 * SI().crlDistance() )
	return false;

    return fabs(pos1.z_ -pos2.z_) < 0.1 * SI().zStep();
}


int FaultGeometry::nrStickDoubles( int sticknr,
				   const FaultGeometry* doublesref ) const
{
    int nrdoubles = 0;
    const FaultGeometry* ref = doublesref ? doublesref : this;

    mDynamicCastGet( const Geometry::FaultStickSet*, srcfss, geometryElement() )

    const StepInterval<int> srccolrg = srcfss->colRange( sticknr );
    if ( srccolrg.isUdf() )
	return -1;

    mDynamicCastGet( const Geometry::FaultStickSet*, reffss,
		     ref->geometryElement() );
    if ( !reffss )
	return 0;

    const StepInterval<int> rowrg = reffss->rowRange();
    if ( rowrg.isUdf() )
	return 0;

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



FaultStickUndoEvent::FaultStickUndoEvent( const EM::PosID& posid )
    : posid_( posid )
    , remove_( false )
{
    RefMan<Object> emobj = 0;
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
    RefMan<Object> emobj = 0;
    mDynamicCastGet( Fault*, fault, emobj.ptr() );
    if ( !fault ) return false;

    const int row = posid_.getRowCol().row();

    return remove_
	? fault->geometry().insertStick( row, posid_.getRowCol().col(),
					 pos_, normal_, false )
	: fault->geometry().removeStick( row, false );
}


bool FaultStickUndoEvent::reDo()
{
    RefMan<Object> emobj = 0;
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
    RefMan<Object> emobj = 0;
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
    RefMan<Object> emobj = 0;
    mDynamicCastGet( Fault*, fault, emobj.ptr() );
    if ( !fault ) return false;

    return remove_
	? fault->geometry().insertKnot( posid_,
					pos_, false )
	: fault->geometry().removeKnot( posid_,
					false );
}


bool FaultKnotUndoEvent::reDo()
{
    RefMan<Object> emobj = 0;
    mDynamicCastGet( Fault*, fault, emobj.ptr() );
    if ( !fault ) return false;

    return remove_
	? fault->geometry().removeKnot( posid_,
					false )
	: fault->geometry().insertKnot( posid_,
					pos_, false );
}


} // namespace EM
