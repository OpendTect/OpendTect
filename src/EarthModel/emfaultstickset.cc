/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        J.C. Glas
 Date:          November 2008
________________________________________________________________________

-*/
static const char* rcsID = "$Id: emfaultstickset.cc,v 1.12 2010-09-23 04:46:25 cvsnanne Exp $";

#include "emfaultstickset.h"

#include "emmanager.h"
#include "emrowcoliterator.h"
#include "emsurfacetr.h"
#include "iopar.h"
#include "posfilter.h"
#include "undo.h"


namespace EM {

mImplementEMObjFuncs( FaultStickSet, EMFaultStickSetTranslatorGroup::keyword() ) 
    
FaultStickSet::FaultStickSet( EMManager& em )
    : Fault(em)
    , geometry_( *this )
{
    geometry_.addSection( "", false );
}


FaultStickSet::~FaultStickSet()
{}


void FaultStickSet::apply( const Pos::Filter& pf )
{
    for ( int idx=0; idx<nrSections(); idx++ )
    {
	mDynamicCastGet( Geometry::FaultStickSet*, fssg,
			 sectionGeometry(sectionID(idx)) );
	if ( !fssg ) continue;

	const StepInterval<int> rowrg = fssg->rowRange();
	if ( rowrg.isUdf() ) continue;

	RowCol rc;
	for ( rc.row=rowrg.stop; rc.row>=rowrg.start; rc.row-=rowrg.step )
	{
	    const StepInterval<int> colrg = fssg->colRange( rc.row );
	    if ( colrg.isUdf() ) continue;

	    for ( rc.col=colrg.stop; rc.col>=colrg.start; rc.col-=colrg.step )
	    {
		const Coord3 pos = fssg->getKnot( rc );
		if ( !pf.includes( (Coord) pos, pos.z) )
		    fssg->removeKnot( rc );
	    }
	}
    }

    // TODO: Handle case in which fault sticks become fragmented.
}


FaultStickSetGeometry& FaultStickSet::geometry()
{ return geometry_; }


const FaultStickSetGeometry& FaultStickSet::geometry() const
{ return geometry_; }


const IOObjContext& FaultStickSet::getIOObjContext() const
{ return EMFaultStickSetTranslatorGroup::ioContext(); }



// ***** FaultStickSetGeometry *****

FaultStickSetGeometry::FaultStickSetGeometry( Surface& surf )
    : FaultGeometry(surf)
{}


FaultStickSetGeometry::~FaultStickSetGeometry()
{}


Geometry::FaultStickSet*
FaultStickSetGeometry::sectionGeometry( const SectionID& sid )
{
    Geometry::Element* res = SurfaceGeometry::sectionGeometry( sid );
    return (Geometry::FaultStickSet*) res;
}


const Geometry::FaultStickSet*
FaultStickSetGeometry::sectionGeometry( const SectionID& sid ) const
{
    const Geometry::Element* res = SurfaceGeometry::sectionGeometry( sid );
    return (const Geometry::FaultStickSet*) res;
}


Geometry::FaultStickSet* FaultStickSetGeometry::createSectionGeometry() const
{ return new Geometry::FaultStickSet; }


EMObjectIterator* FaultStickSetGeometry::createIterator( const SectionID& sid,
						const CubeSampling* cs ) const
{ return new RowColIterator( surface_, sid, cs ); }


int FaultStickSetGeometry::nrSticks( const SectionID& sid ) const
{
    const Geometry::FaultStickSet* fss = sectionGeometry( sid );
    return fss ? fss->nrSticks(): 0;
}


int FaultStickSetGeometry::nrKnots( const SectionID& sid, int sticknr ) const
{
    const Geometry::FaultStickSet* fss = sectionGeometry( sid );
    return fss ? fss->nrKnots(sticknr) : 0;
}


#define mTriggerSurfaceChange( surf ) \
    surf.setChangedFlag(); \
    EMObjectCallbackData cbdata; \
    cbdata.event = EMObjectCallbackData::BurstAlert; \
    surf.change.trigger( cbdata );


bool FaultStickSetGeometry::insertStick( const SectionID& sid, int sticknr,
					 int firstcol, const Coord3& pos,
					 const Coord3& editnormal,
					 bool addtohistory )
{
    return insertStick( sid, sticknr, firstcol, pos, editnormal, 0, 0,
	    		addtohistory );
}


bool FaultStickSetGeometry::insertStick( const SectionID& sid, int sticknr,
					 int firstcol, const Coord3& pos,
					 const Coord3& editnormal,
					 const MultiID* lineset,
					 const char* linenm, bool addtohistory )
{
    Geometry::FaultStickSet* fss = sectionGeometry( sid );

    if ( !fss ) 
	return false;

    const bool firstrowchange = sticknr < fss->rowRange().start;

    if ( !fss->insertStick(pos,editnormal,sticknr,firstcol) )
	return false;

    for ( int idx=0; !firstrowchange && idx<stickinfo_.size(); idx++ )
    {
	if ( stickinfo_[idx]->sid==sid && stickinfo_[idx]->sticknr>=sticknr )
	    stickinfo_[idx]->sticknr++;
    }
    stickinfo_.insertAt( new StickInfo, 0 );
    stickinfo_[0]->sid = sid;
    stickinfo_[0]->sticknr = sticknr;
    stickinfo_[0]->lineset = lineset ? *lineset : MultiID(-1);
    stickinfo_[0]->linenm = linenm;

    if ( addtohistory )
    {
	const PosID posid( surface_.id(),sid,RowCol(sticknr,0).toInt64());
	UndoEvent* undo = new FaultStickUndoEvent( posid );
	EMM().undo().addEvent( undo, 0 );
    }

    mTriggerSurfaceChange( surface_ );
    return true;
}


bool FaultStickSetGeometry::removeStick( const SectionID& sid, int sticknr,
					 bool addtohistory )
{
    Geometry::FaultStickSet* fss = sectionGeometry( sid );
    if ( !fss )
	return false;

    const StepInterval<int> colrg = fss->colRange( sticknr );
    if ( colrg.isUdf() || colrg.width() )
	return false;

    const RowCol rc( sticknr, colrg.start );

    const Coord3 pos = fss->getKnot( rc );
    const Coord3 normal = getEditPlaneNormal( sid, sticknr );
    if ( !normal.isDefined() || !pos.isDefined() )
	return false;

    if ( !fss->removeStick(sticknr) )
	return false;

    for ( int idx=stickinfo_.size()-1; idx>=0; idx-- )
    {
	if ( stickinfo_[idx]->sid != sid )
	    continue;

	if ( stickinfo_[idx]->sticknr == sticknr )
	{
	    delete stickinfo_.remove( idx );
	    continue;
	}

	if ( sticknr>=fss->rowRange().start && stickinfo_[idx]->sticknr>sticknr)
	    stickinfo_[idx]->sticknr--;
    }

    if ( addtohistory )
    {
	const PosID posid( surface_.id(), sid, rc.toInt64() );
	UndoEvent* undo = new FaultStickUndoEvent( posid, pos, normal );
	EMM().undo().addEvent( undo, 0 );
    }

    mTriggerSurfaceChange( surface_ );
    return true;
}


bool FaultStickSetGeometry::insertKnot( const SectionID& sid,
					const SubID& subid, const Coord3& pos,
					bool addtohistory )
{
    Geometry::FaultStickSet* fss = sectionGeometry( sid );
    RowCol rc;
    rc.fromInt64( subid );
    if ( !fss || !fss->insertKnot(rc,pos) )
	return false;

    if ( addtohistory )
    {
	const PosID posid( surface_.id(), sid, subid );
	UndoEvent* undo = new FaultKnotUndoEvent( posid );
	EMM().undo().addEvent( undo, 0 );
    }

    mTriggerSurfaceChange( surface_ );
    return true;
}


bool FaultStickSetGeometry::removeKnot( const SectionID& sid,
					const SubID& subid, bool addtohistory )
{
    Geometry::FaultStickSet* fss = sectionGeometry( sid );
    if ( !fss ) return false;

    RowCol rc;
    rc.fromInt64( subid );
    const Coord3 pos = fss->getKnot( rc );

    if ( !pos.isDefined() || !fss->removeKnot(rc) )
	return false;

    if ( addtohistory )
    {
	const PosID posid( surface_.id(), sid, subid );
	UndoEvent* undo = new FaultKnotUndoEvent( posid, pos );
	EMM().undo().addEvent( undo, 0 );
    }

    mTriggerSurfaceChange( surface_ );
    return true;
}


bool FaultStickSetGeometry::pickedOnPlane( const SectionID& sid,
					   int sticknr ) const
{
    if ( pickedOn2DLine(sid,sticknr) )
	return false;

    const Coord3& editnorm = getEditPlaneNormal( sid, sticknr );
    return editnorm.isDefined();
}


bool FaultStickSetGeometry::pickedOn2DLine( const SectionID& sid,
					    int sticknr ) const
{ return lineSet( sid, sticknr ); }


const MultiID* FaultStickSetGeometry::lineSet( const SectionID& sid,
					       int sticknr) const
{
    for ( int idx=0; idx<stickinfo_.size(); idx++ )
    {
	if ( stickinfo_[idx]->sid==sid && stickinfo_[idx]->sticknr==sticknr )
	{
	    const MultiID& lset = stickinfo_[idx]->lineset;
	    return lset==MultiID(-1) ? 0 : &lset;
	}
    }
    return 0;
}


const char* FaultStickSetGeometry::lineName( const SectionID& sid,
					     int sticknr) const
    {
    for ( int idx=0; idx<stickinfo_.size(); idx++ )
    {
	if ( stickinfo_[idx]->sid==sid && stickinfo_[idx]->sticknr==sticknr )
	{
	    const MultiID& lset = stickinfo_[idx]->lineset;
	    return lset==MultiID(-1) ? 0 : stickinfo_[idx]->linenm.buf();
	}
    }
    return 0;
}


#define mDefStickInfoStr( prefixstr, stickinfostr, sid, sticknr ) \
    BufferString stickinfostr(prefixstr); stickinfostr += " of section "; \
    stickinfostr += sid; stickinfostr += " sticknr "; stickinfostr += sticknr;

#define mDefEditNormalStr( editnormalstr, sid, sticknr ) \
    mDefStickInfoStr( "Edit normal", editnormalstr, sid, sticknr )
#define mDefLineSetStr( linesetstr, sid, sticknr ) \
    mDefStickInfoStr( "Line set", linesetstr, sid, sticknr )
#define mDefLineNameStr( linenamestr, sid, sticknr ) \
    mDefStickInfoStr( "Line name", linenamestr, sid, sticknr )



void FaultStickSetGeometry::fillPar( IOPar& par ) const
{
    for ( int idx=0; idx<nrSections(); idx++ )
    {
	int sid = sectionID( idx );
	const Geometry::FaultStickSet* fss = sectionGeometry( sid );
	if ( !fss ) continue;

	StepInterval<int> stickrg = fss->rowRange();
	for ( int sticknr=stickrg.start; sticknr<=stickrg.stop; sticknr++ )
	{
	    mDefEditNormalStr( editnormalstr, sid, sticknr );
	    par.set( editnormalstr.buf(), fss->getEditPlaneNormal(sticknr) );

	    const MultiID* lineset = lineSet( sid, sticknr );
	    if ( !lineset )
		continue;

	    mDefLineSetStr( linesetstr, sid, sticknr );
	    par.set( linesetstr.buf(), *lineset );

	    mDefLineNameStr( linenamestr, sid, sticknr );
	    par.set( linenamestr.buf(), lineName(sid,sticknr) );
	}
    }
}


bool FaultStickSetGeometry::usePar( const IOPar& par )
{
    for ( int idx=0; idx<nrSections(); idx++ )
    {
	int sid = sectionID( idx );
	Geometry::FaultStickSet* fss = sectionGeometry( sid );
	if ( !fss ) return false;

	StepInterval<int> stickrg = fss->rowRange();
	for ( int sticknr=stickrg.start; sticknr<=stickrg.stop; sticknr++ )
	{
	    mDefEditNormalStr( editnormstr, sid, sticknr );
	    Coord3 editnormal( Coord3::udf() );
	    par.get( editnormstr.buf(), editnormal );
	    fss->addEditPlaneNormal( editnormal );

	    stickinfo_.insertAt( new StickInfo, 0 );
	    stickinfo_[0]->sid = sid;
	    stickinfo_[0]->sticknr = sticknr;

	    mDefLineSetStr( linesetstr, sid, sticknr );
	    if ( !par.get(linesetstr.buf(), stickinfo_[0]->lineset) )
		stickinfo_[0]->lineset = MultiID( -1 );

	    mDefLineNameStr( linenamestr, sid, sticknr );
	    par.get( linenamestr.buf(), stickinfo_[0]->linenm );
	}
    }

    return true;
}


} // namespace EM
