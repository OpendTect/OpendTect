/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        N. Fredman
 Date:          Sep 2002
 RCS:           $Id: emfault.cc,v 1.44 2008-05-21 06:30:38 cvsnanne Exp $
________________________________________________________________________

-*/

#include "emfault.h"

#include "emsurfacetr.h"
#include "emmanager.h"
#include "emrowcoliterator.h"
#include "undo.h"
#include "errh.h"
#include "survinfo.h"
#include "tabledef.h"

namespace EM {

mImplementEMObjFuncs( Fault, EMFaultTranslatorGroup::keyword ) 

class FaultStickUndoEvent : public UndoEvent
{
public:

//Interface for insert
FaultStickUndoEvent( const EM::PosID& posid )
    : posid_( posid )
    , remove_( false )
{
    RefMan<EMObject> emobj = EMM().getObject( posid_.objectID() );
    mDynamicCastGet( Fault*, fault, emobj.ptr() );
    if ( !fault ) return;

    pos_ = fault->getPos( posid_ );
    const int row = RowCol(posid_.subID()).row;
    normal_ = fault->geometry().getEditPlaneNormal( posid_.sectionID(), row );
}


//Interface for removal
FaultStickUndoEvent( const EM::PosID& posid, const Coord3& oldpos,
		     const Coord3& oldnormal )
    : posid_( posid )
    , pos_( oldpos )
    , normal_( oldnormal )
    , remove_( true )
{ }


const char* getStandardDesc() const
{ return remove_ ? "Remove stick" : "Insert stick"; }


bool unDo()
{
    RefMan<EMObject> emobj = EMM().getObject( posid_.objectID() );
    mDynamicCastGet( Fault*, fault, emobj.ptr() );
    if ( !fault ) return false;

    const int row = RowCol(posid_.subID()).row;

    return remove_
	? fault->geometry().insertStick( posid_.sectionID(), row, pos_, normal_,
					 false )
	: fault->geometry().removeStick( posid_.sectionID(), row, false );
}


bool reDo()
{
    RefMan<EMObject> emobj = EMM().getObject( posid_.objectID() );
    mDynamicCastGet( Fault*, fault, emobj.ptr() );
    if ( !fault ) return false;

    const int row = RowCol(posid_.subID()).row;

    return remove_
	? fault->geometry().removeStick( posid_.sectionID(), row, false )
	: fault->geometry().insertStick( posid_.sectionID(), row, pos_, normal_,
					 false );
}

protected:

    Coord3	pos_;
    Coord3	normal_;
    EM::PosID	posid_;
    bool	remove_;
};

    
class FaultKnotUndoEvent : public UndoEvent
{
public:

//Interface for insert
FaultKnotUndoEvent( const EM::PosID& posid )
    : posid_( posid )
    , remove_( false )
{
    RefMan<EMObject> emobj = EMM().getObject( posid_.objectID() );
    if ( !emobj ) return;
    pos_ = emobj->getPos( posid_ );
}


//Interface for removal
FaultKnotUndoEvent( const EM::PosID& posid, const Coord3& oldpos )
    : posid_( posid )
    , pos_( oldpos )
    , remove_( true )
{ }


const char* getStandardDesc() const
{ return remove_ ? "Remove knot" : "Insert knot"; }


bool unDo()
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


bool reDo()
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

protected:

    Coord3	pos_;
    Coord3	normal_;
    EM::PosID	posid_;
    bool	remove_;
};

    
Fault::Fault( EMManager& em )
    : Surface(em)
    , geometry_( *this )
{
    geometry_.addSection( "", false );
}


Fault::~Fault()
{}


FaultGeometry& Fault::geometry()
{ return geometry_; }


const FaultGeometry& Fault::geometry() const
{ return geometry_; }


const IOObjContext& Fault::getIOObjContext() const
{ return EMFaultTranslatorGroup::ioContext(); }


FaultGeometry::FaultGeometry( Fault& flt )
    : SurfaceGeometry(flt)
{ }


FaultGeometry::~FaultGeometry()
{}


Geometry::FaultStickSurface*
FaultGeometry::sectionGeometry( const SectionID& sid )
{
    Geometry::Element* res = SurfaceGeometry::sectionGeometry( sid );
    return (Geometry::FaultStickSurface*) res;
}


const Geometry::FaultStickSurface*
FaultGeometry::sectionGeometry( const SectionID& sid ) const
{
    const Geometry::Element* res = SurfaceGeometry::sectionGeometry( sid );
    return (const Geometry::FaultStickSurface*) res;
}


Geometry::FaultStickSurface* FaultGeometry::createSectionGeometry() const
{ return new Geometry::FaultStickSurface; }



EMObjectIterator* FaultGeometry::createIterator( const SectionID& sid,
						 const CubeSampling* cs) const
{ return new RowColIterator( surface_, sid, cs ); }


int FaultGeometry::nrSticks( const SectionID& sid ) const
{
    const Geometry::FaultStickSurface* fss = sectionGeometry( sid );
    return !fss || fss->isEmpty() ? 0 : fss->rowRange().nrSteps()+1;
}


int FaultGeometry::nrKnots( const SectionID& sid, int sticknr ) const
{
    const Geometry::FaultStickSurface* fss = sectionGeometry( sid );
    return !fss || fss->isEmpty() ? -1 : fss->colRange(sticknr).nrSteps()+1;
}


bool FaultGeometry::insertStick( const SectionID& sid, int sticknr,
				 const Coord3& pos, const Coord3& editnormal,
				 bool addtohistory )
{
    Geometry::FaultStickSurface* fss = sectionGeometry( sid );
    if ( !fss || !fss->insertStick(pos,editnormal,sticknr) )
	return false;


    if ( addtohistory )
    {
	const PosID posid( surface_.id(),sid,RowCol(sticknr,0).getSerialized());
	UndoEvent* undo = new FaultStickUndoEvent( posid );
	EMM().undo().addEvent( undo, 0 );
    }

    EMObjectCallbackData cbdata;
    cbdata.event = EMObjectCallbackData::BurstAlert;
    surface_.change.trigger( cbdata );

    return true;
}


bool FaultGeometry::removeStick( const SectionID& sid, int sticknr,
				 bool addtohistory )
{
    Geometry::FaultStickSurface* fss = sectionGeometry( sid );
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

    if ( addtohistory )
    {
	const PosID posid( surface_.id(), sid, rc.getSerialized() );

	UndoEvent* undo = new FaultStickUndoEvent( posid, pos, normal );
	EMM().undo().addEvent( undo, 0 );
    }

    EMObjectCallbackData cbdata;
    cbdata.event = EMObjectCallbackData::BurstAlert;
    surface_.change.trigger( cbdata );

    return true;
}


bool FaultGeometry::insertKnot( const SectionID& sid, const SubID& subid,
				const Coord3& pos, bool addtohistory )
{
    Geometry::FaultStickSurface* fss = sectionGeometry( sid );
    RowCol rc;
    rc.setSerialized( subid );
    if ( !fss || !fss->insertKnot(rc,pos) )
	return false;

    if ( addtohistory )
    {
	const PosID posid( surface_.id(), sid, subid );
	UndoEvent* undo = new FaultKnotUndoEvent( posid );
	EMM().undo().addEvent( undo, 0 );
    }

    EMObjectCallbackData cbdata;
    cbdata.event = EMObjectCallbackData::BurstAlert;
    surface_.change.trigger( cbdata );

    return true;
}


bool FaultGeometry::areSticksVertical( const SectionID& sid ) const
{
    const Geometry::FaultStickSurface* fss = sectionGeometry( sid );
    return fss ? fss->areSticksVertical() : false;
}


const Coord3& FaultGeometry::getEditPlaneNormal( const SectionID& sid,
						 int sticknr ) const
{
    const Geometry::FaultStickSurface* fss = sectionGeometry( sid );
    return fss ? fss->getEditPlaneNormal(sticknr) : Coord3::udf();
}


bool FaultGeometry::removeKnot( const SectionID& sid, const SubID& subid,
				bool addtohistory )
{
    Geometry::FaultStickSurface* fss = sectionGeometry( sid );
    if ( !fss ) return false;

    RowCol rc;
    rc.setSerialized( subid );
    const Coord3 pos = fss->getKnot( rc );

    if ( !pos.isDefined() || !fss->removeKnot(rc) )
	return false;

    if ( addtohistory )
    {
	const PosID posid( surface_.id(), sid, subid );

	UndoEvent* undo = new FaultKnotUndoEvent( posid, pos );
	EMM().undo().addEvent( undo, 0 );
    }

    EMObjectCallbackData cbdata;
    cbdata.event = EMObjectCallbackData::BurstAlert;
    surface_.change.trigger( cbdata );

    return true;
}


#define mDefEditNormalStr( editnormstr, sid, sticknr ) \
    BufferString editnormstr("Edit normal of section "); \
    editnormstr += sid; editnormstr += " sticknr "; editnormstr += sticknr; 

void FaultGeometry::fillPar( IOPar& par ) const
{
    for ( int idx=0; idx<nrSections(); idx++ )
    {
	int sid = sectionID( idx );
	const Geometry::FaultStickSurface* fss = sectionGeometry( sid );
	if ( !fss ) continue;

	StepInterval<int> stickrg = fss->rowRange();
	for ( int sticknr=stickrg.start; sticknr<=stickrg.stop; sticknr++ )
	{
	    mDefEditNormalStr( editnormstr, sid, sticknr );
	    par.set( editnormstr.buf(), fss->getEditPlaneNormal(sticknr) );
	}
    }
}


bool FaultGeometry::usePar( const IOPar& par )
{
    for ( int idx=0; idx<nrSections(); idx++ )
    {
	int sid = sectionID( idx );
	Geometry::FaultStickSurface* fss = sectionGeometry( sid );
	if ( !fss ) return false;

	StepInterval<int> stickrg = fss->rowRange();
	for ( int sticknr=stickrg.start; sticknr<=stickrg.stop; sticknr++ )
	{
	    mDefEditNormalStr( editnormstr, sid, sticknr );
	    Coord3 editnormal( Coord3::udf() ); 
	    par.get( editnormstr.buf(), editnormal ); 
	    fss->addEditPlaneNormal( editnormal );
	}
    }
    return true;
}


Table::FormatDesc* FaultAscIO::getDesc()
{
    Table::FormatDesc* fd = new Table::FormatDesc( "Fault3D" );

    Table::TargetInfo* posinfo =
	new Table::TargetInfo( "", FloatInpSpec(), Table::Required );
    Table::TargetInfo::Form* form =
	new Table::TargetInfo::Form( "Inl/Crl", FloatInpSpec() );
    form->add( FloatInpSpec() );
    posinfo->add( form );
    posinfo->form(0).setName( "X/Y");
    posinfo->form(0).add( FloatInpSpec() );
    fd->bodyinfos_ += posinfo;
    Table::TargetInfo* zti = new Table::TargetInfo( "Z", FloatInpSpec(),
	    				     Table::Required );
    zti->setPropertyType( PropertyRef::surveyZType() );
    fd->bodyinfos_ += zti;
    fd->bodyinfos_ += new Table::TargetInfo( "Stick index", IntInpSpec(),
	    				     Table::Required );
    return fd;
}


} // namespace EM
