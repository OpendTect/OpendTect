/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        N. Fredman
 Date:          Sep 2002
 RCS:           $Id: emfault3d.cc,v 1.1 2008-09-30 12:47:52 cvsnanne Exp $
________________________________________________________________________

-*/

#include "emfault3d.h"

#include "emsurfacetr.h"
#include "emmanager.h"
#include "emrowcoliterator.h"
#include "undo.h"
#include "errh.h"
#include "survinfo.h"
#include "tabledef.h"
#include "unitofmeasure.h"

namespace EM {

mImplementEMObjFuncs( Fault3D, EMFault3DTranslatorGroup::keyword ) 

class FaultStickUndoEvent : public UndoEvent
{
public:

//Interface for insert
FaultStickUndoEvent( const EM::PosID& posid )
    : posid_( posid )
    , remove_( false )
{
    RefMan<EMObject> emobj = EMM().getObject( posid_.objectID() );
    mDynamicCastGet( Fault3D*, fault, emobj.ptr() );
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
    mDynamicCastGet( Fault3D*, fault, emobj.ptr() );
    if ( !fault ) return false;

    const int row = RowCol(posid_.subID()).row;

    return remove_
	? fault->geometry().insertStick( posid_.sectionID(), row,
		RowCol(posid_.subID()).col, pos_, normal_, false )
	: fault->geometry().removeStick( posid_.sectionID(), row, false );
}


bool reDo()
{
    RefMan<EMObject> emobj = EMM().getObject( posid_.objectID() );
    mDynamicCastGet( Fault3D*, fault, emobj.ptr() );
    if ( !fault ) return false;

    const int row = RowCol(posid_.subID()).row;

    return remove_
	? fault->geometry().removeStick( posid_.sectionID(), row, false )
	: fault->geometry().insertStick( posid_.sectionID(), row,
		RowCol(posid_.subID()).col, pos_, normal_, false );
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
    mDynamicCastGet( Fault3D*, fault, emobj.ptr() );
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
    mDynamicCastGet( Fault3D*, fault, emobj.ptr() );
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

    
Fault3D::Fault3D( EMManager& em )
    : Surface(em)
    , geometry_( *this )
{
    geometry_.addSection( "", false );
}


Fault3D::~Fault3D()
{}


Fault3DGeometry& Fault3D::geometry()
{ return geometry_; }


const Fault3DGeometry& Fault3D::geometry() const
{ return geometry_; }


const IOObjContext& Fault3D::getIOObjContext() const
{ return EMFault3DTranslatorGroup::ioContext(); }


Fault3DGeometry::Fault3DGeometry( Surface& surf )
    : FaultGeometry(surf)
{ }


Fault3DGeometry::~Fault3DGeometry()
{}


Geometry::FaultStickSurface*
Fault3DGeometry::sectionGeometry( const SectionID& sid )
{
    Geometry::Element* res = SurfaceGeometry::sectionGeometry( sid );
    return (Geometry::FaultStickSurface*) res;
}


const Geometry::FaultStickSurface*
Fault3DGeometry::sectionGeometry( const SectionID& sid ) const
{
    const Geometry::Element* res = SurfaceGeometry::sectionGeometry( sid );
    return (const Geometry::FaultStickSurface*) res;
}


Geometry::FaultStickSurface* Fault3DGeometry::createSectionGeometry() const
{ return new Geometry::FaultStickSurface; }


EMObjectIterator* Fault3DGeometry::createIterator( const SectionID& sid,
						 const CubeSampling* cs) const
{ return new RowColIterator( surface_, sid, cs ); }


int Fault3DGeometry::nrSticks( const SectionID& sid ) const
{
    const Geometry::FaultStickSurface* fss = sectionGeometry( sid );
    return !fss || fss->isEmpty() ? 0 : fss->rowRange().nrSteps()+1;
}


int Fault3DGeometry::nrKnots( const SectionID& sid, int sticknr ) const
{
    const Geometry::FaultStickSurface* fss = sectionGeometry( sid );
    return !fss || fss->isEmpty() ? -1 : fss->colRange(sticknr).nrSteps()+1;
}


bool Fault3DGeometry::insertStick( const SectionID& sid, int sticknr, 
				 int firstcol, const Coord3& pos,
				 const Coord3& editnormal, bool addtohistory )
{
    Geometry::FaultStickSurface* fss = sectionGeometry( sid );
    if ( !fss || !fss->insertStick(pos,editnormal,sticknr, firstcol) )
	return false;


    if ( addtohistory )
    {
	const PosID posid( surface_.id(),sid,RowCol(sticknr,0).getSerialized());
	UndoEvent* undo = new FaultStickUndoEvent( posid );
	EMM().undo().addEvent( undo, 0 );
    }

    surface_.setChangedFlag();
    EMObjectCallbackData cbdata;
    cbdata.event = EMObjectCallbackData::BurstAlert;
    surface_.change.trigger( cbdata );

    return true;
}


bool Fault3DGeometry::removeStick( const SectionID& sid, int sticknr,
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

    surface_.setChangedFlag();
    EMObjectCallbackData cbdata;
    cbdata.event = EMObjectCallbackData::BurstAlert;
    surface_.change.trigger( cbdata );

    return true;
}


bool Fault3DGeometry::insertKnot( const SectionID& sid, const SubID& subid,
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

    surface_.setChangedFlag();
    EMObjectCallbackData cbdata;
    cbdata.event = EMObjectCallbackData::BurstAlert;
    surface_.change.trigger( cbdata );

    return true;
}


bool Fault3DGeometry::areSticksVertical( const SectionID& sid ) const
{
    const Geometry::FaultStickSurface* fss = sectionGeometry( sid );
    return fss ? fss->areSticksVertical() : false;
}


const Coord3& Fault3DGeometry::getEditPlaneNormal( const SectionID& sid,
						 int sticknr ) const
{
    const Geometry::FaultStickSurface* fss = sectionGeometry( sid );
    return fss ? fss->getEditPlaneNormal(sticknr) : Coord3::udf();
}


bool Fault3DGeometry::removeKnot( const SectionID& sid, const SubID& subid,
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

    surface_.setChangedFlag();
    EMObjectCallbackData cbdata;
    cbdata.event = EMObjectCallbackData::BurstAlert;
    surface_.change.trigger( cbdata );

    return true;
}


#define mDefEditNormalStr( editnormstr, sid, sticknr ) \
    BufferString editnormstr("Edit normal of section "); \
    editnormstr += sid; editnormstr += " sticknr "; editnormstr += sticknr; 

void Fault3DGeometry::fillPar( IOPar& par ) const
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


bool Fault3DGeometry::usePar( const IOPar& par )
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


// ***** Fault3DAscIO *****

Table::FormatDesc* Fault3DAscIO::getDesc()
{
    Table::FormatDesc* fd = new Table::FormatDesc( "Fault3D" );

    Table::TargetInfo* posinfo = new Table::TargetInfo( "X/Y", FloatInpSpec(),
	    						Table::Required );
    posinfo->form(0).add( FloatInpSpec() );
//    posinfo->add( posinfo->form(0).duplicate("Inl/Crl") );
//  Maybe support Inl/Crl later
    fd->bodyinfos_ += posinfo;

    Table::TargetInfo* zti = new Table::TargetInfo( "Z", FloatInpSpec(),
						    Table::Required );
    zti->setPropertyType( PropertyRef::surveyZType() );
    zti->selection_.unit_ = UoMR().get( SI().getZUnit(false) );
    fd->bodyinfos_ += zti;
    fd->bodyinfos_ += new Table::TargetInfo( "Stick index", IntInpSpec(),
	    				     Table::Optional );
    return fd;
}


bool Fault3DAscIO::isXY() const
{
    const Table::TargetInfo* posinfo = fd_.bodyinfos_[0];
    return !posinfo || posinfo->selection_.form_ == 0;
}


struct FaultStick
{
    			FaultStick(int idx)	: stickidx_(idx)	{}

    int			stickidx_;
    TypeSet<Coord3>	crds_;

Coord3 getNormal() const
{
    int oninl = 0;
    int oncrl = 0;
    const BinID firstbid = SI().transform( crds_[0] );
    for ( int idx=1; idx<crds_.size(); idx++ )
    {
	const BinID bid = SI().transform( crds_[idx] );
	if ( bid.inl == firstbid.inl ) oninl++;
	if ( bid.crl == firstbid.crl ) oncrl++;
    }

    if ( oninl == 0 && oncrl == 0 )
	return Coord3( 0, 0, 1 );

    return oninl>oncrl ? Coord3( SI().binID2Coord().rowDir(), 0 )
		       : Coord3( SI().binID2Coord().colDir(), 0 );
}

};


bool Fault3DAscIO::get( std::istream& strm, EM::Fault3D& flt ) const
{
    getHdrVals( strm );

    Coord3 crd;
    Coord3 normal( 1, 0, 0 );
    const SectionID sid = flt.sectionID( 0 );
    int curstickidx = -1;
    bool hasstickidx = false;

    bool oninl = false;
    bool oncrl = false;
    int linenr;
    BinID firstbid;

    ObjectSet<FaultStick> sticks;

    while ( true )
    {
	const int ret = getNextBodyVals( strm );
	if ( ret < 0 ) return false;
	if ( ret == 0 ) break;

	crd.x = getfValue( 0 );
	crd.y = getfValue( 1 );
	crd.z = getfValue( 2 );
	const int stickidx = getIntValue( 3 );
	if ( !hasstickidx && !mIsUdf(stickidx) )
	    hasstickidx = true;

	if ( !crd.isDefined() )
	    continue;

	if ( hasstickidx )
	{
	    if ( stickidx != curstickidx )
	    {
		FaultStick* stick = new FaultStick( stickidx );
		stick->crds_ += crd;
		sticks += stick;
		curstickidx = stickidx;
	    }
	    else
	    {
		FaultStick* stick = sticks[ sticks.size()-1 ];
		stick->crds_ += crd;
	    }
	}
	else
	{
	    const BinID curbid = SI().transform( crd );
	    if ( sticks.isEmpty() )
	    {
		curstickidx = 0;
		FaultStick* stick = new FaultStick( curstickidx );
		stick->crds_ += crd;
		sticks += stick;
		firstbid = curbid;
	    }
	    else if ( (oninl && curbid.inl!=firstbid.inl) || 
		      (oncrl && curbid.crl!=firstbid.crl) )
	    {
		curstickidx++;
		FaultStick* stick = new FaultStick( curstickidx );
		stick->crds_ += crd;
		sticks += stick;
		firstbid = curbid;
		oninl = false;
		oncrl = false;
	    }
	    else
	    {
		const BinID bid = SI().transform( crd );
		oninl = bid.inl == firstbid.inl;
		oncrl = bid.crl == firstbid.crl;

		FaultStick* stick = sticks[ sticks.size()-1 ];
		stick->crds_ += crd;
	    }
	}
    }

    for ( int idx=0; idx<sticks.size(); idx++ )
    {
	FaultStick* stick = sticks[idx];
	if ( stick->crds_.isEmpty() )
	    continue;

	// TODO: sort sticks

	bool res = flt.geometry().insertStick( sid, stick->stickidx_, 0,
					       stick->crds_[0],
					       stick->getNormal(), false );
	if ( !res ) continue;

	for ( int crdidx=1; crdidx<stick->crds_.size(); crdidx++ )
	{
	    const RowCol rc( stick->stickidx_, crdidx );
	    flt.geometry().insertKnot( sid, rc.getSerialized(),
		    		       stick->crds_[crdidx], false );
	}
    }

    deepErase( sticks );
    return true;
}


} // namespace EM
