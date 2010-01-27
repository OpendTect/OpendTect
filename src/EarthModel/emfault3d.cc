/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        N. Fredman
 Date:          Sep 2002
________________________________________________________________________

-*/
static const char* rcsID = "$Id: emfault3d.cc,v 1.13 2010-01-27 13:48:27 cvsjaap Exp $";

#include "emfault3d.h"

#include "emsurfacetr.h"
#include "emmanager.h"
#include "emrowcoliterator.h"
#include "undo.h"
#include "errh.h"
#include "posfilter.h"
#include "survinfo.h"
#include "tabledef.h"
#include "unitofmeasure.h"

namespace EM {

mImplementEMObjFuncs( Fault3D, EMFault3DTranslatorGroup::keyword() ) 

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
    : Fault(em)
    , geometry_( *this )
{
    geometry_.addSection( "", false );
}


Fault3D::~Fault3D()
{}


void Fault3D::removeAll()
{
    Surface::removeAll();
    geometry_.removeAll();
}


Fault3DGeometry& Fault3D::geometry()
{ return geometry_; }


const Fault3DGeometry& Fault3D::geometry() const
{ return geometry_; }


const IOObjContext& Fault3D::getIOObjContext() const
{ return EMFault3DTranslatorGroup::ioContext(); }


void Fault3D::apply( const Pos::Filter& pf )
{
    for ( int idx=0; idx<nrSections(); idx++ )
    {
	mDynamicCastGet( Geometry::FaultStickSurface*, fssg,
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

    // TODO: Handle cases in which fault becomes fragmented.
}


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
    return fss ? fss->nrSticks() : 0;
}


int Fault3DGeometry::nrKnots( const SectionID& sid, int sticknr ) const
{
    const Geometry::FaultStickSurface* fss = sectionGeometry( sid );
    return fss ? fss->nrKnots(sticknr) : 0;
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


void Fault3DGeometry::selectAllSticks( bool select )
{
    PtrMan<EM::EMObjectIterator> iter = createIterator(-1);
    while ( true )
    {
	EM::PosID pid = iter->next();
	if ( pid.objectID() == -1 )
	    break;

	const int sticknr = RowCol( pid.subID() ).row;
	const EM::SectionID sid = pid.sectionID();
	Geometry::FaultStickSurface* fss = sectionGeometry(sid);
	fss->selectStick( sticknr, select );
    }
}


bool Fault3DGeometry::removeNextSelStick()
{
    for ( int sidx=nrSections()-1; sidx>=0; sidx-- )
    {
	const int sid = sectionID( sidx );
	Geometry::FaultStickSurface* fss = sectionGeometry(sid);
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
		removeSection( sid, true );

	    return true;
	}
    }
    return false;
}


void Fault3DGeometry::removeSelectedSticks()
{ while ( removeNextSelStick() ); }


// ***** FaultAscIO *****

Table::FormatDesc* FaultAscIO::getDesc( bool is2d )
{
    Table::FormatDesc* fd = new Table::FormatDesc( "Fault" );

    Table::TargetInfo* posinfo = new Table::TargetInfo( "X/Y", FloatInpSpec(),
	    						Table::Required );
    posinfo->form(0).add( FloatInpSpec() );
    fd->bodyinfos_ += posinfo;

    Table::TargetInfo* zti = new Table::TargetInfo( "Z", FloatInpSpec(),
						    Table::Required );
    zti->setPropertyType( PropertyRef::surveyZType() );
    zti->selection_.unit_ = UoMR().get( SI().getZUnitString(false) );
    fd->bodyinfos_ += zti;
    fd->bodyinfos_ += new Table::TargetInfo( "Stick index", IntInpSpec(),
	    				     Table::Optional );
    if ( is2d )
	fd->bodyinfos_ += new Table::TargetInfo( "Line name", StringInpSpec(),
						 Table::Required );
    return fd;
}


bool FaultAscIO::isXY() const
{
    const Table::TargetInfo* posinfo = fd_.bodyinfos_[0];
    return !posinfo || posinfo->selection_.form_ == 0;
}


struct FaultStick
{
    			FaultStick(int idx)	: stickidx_(idx)	{}

    int			stickidx_;
    TypeSet<Coord3>	crds_;
    BufferString	lnm_;


Coord3 getNormal() const
{
    // TODO: Determine edit normal for sticks picked on 2D lines

    const int maxdist = 5;
    int oninl = 0; int oncrl = 0; int ontms = 0;

    for ( int idx=0; idx<crds_.size()-1; idx++ )
    {
	const BinID bid0 = SI().transform( crds_[idx] );
	for ( int idy=idx+1; idy<crds_.size(); idy++ )
	{
	    const BinID bid1 = SI().transform( crds_[idy] );
	    const int inldist = abs( bid0.inl-bid1.inl );
	    if ( inldist < maxdist )
		oninl += maxdist - inldist;
	    const int crldist = abs( bid0.crl-bid1.crl );
	    if ( crldist < maxdist )
		oncrl += maxdist - crldist;
	    const int zdist = mNINT( fabs(crds_[idx].z-crds_[idy].z) /
			             fabs(SI().zStep()) );
	    if ( zdist < maxdist )
		ontms += maxdist - zdist;
	}
    }

    if ( ontms>oncrl && ontms>oninl )
	return Coord3( 0, 0, 1 );

    return oncrl>oninl ? Coord3( SI().binID2Coord().colDir(), 0 )
		       : Coord3( SI().binID2Coord().rowDir(), 0 );
}

};


bool FaultAscIO::get( std::istream& strm, EM::Fault& flt, bool sortsticks,
		      const MultiID* linesetmid, bool is2d ) const
{
    getHdrVals( strm );

    Coord3 crd;
    const SectionID sid = flt.sectionID( 0 );
    int curstickidx = -1;
    bool hasstickidx = false;

    bool oninl = false; bool oncrl = false; bool ontms = false;

    float firstz; 
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

	BufferString lnm;
	if ( is2d )
	    lnm = text( 4 );

	if ( sticks.isEmpty() && !mIsUdf(stickidx) )
	    hasstickidx = true;

	if ( !crd.isDefined() )
	    continue;

	if ( hasstickidx )
	{
	    if ( !mIsUdf(stickidx) && stickidx!=curstickidx )
	    {
		curstickidx = stickidx;
		sticks += new FaultStick( curstickidx );
	    }
	}
	else
	{
	    const BinID curbid = SI().transform( crd );
	    
	    oninl = oninl && curbid.inl==firstbid.inl;
	    oncrl = oncrl && curbid.crl==firstbid.crl;
	    ontms = ontms && fabs(crd.z-firstz) < fabs(0.5*SI().zStep());

	    if ( !oninl && !oncrl && !ontms )
	    {
		curstickidx++;
		sticks += new FaultStick( stickidx );

		firstbid = curbid; firstz = crd.z;
		oninl = true; oncrl = true; ontms = true;
	    }
	}

	sticks[ sticks.size()-1 ]->crds_ += crd;
	sticks[ sticks.size()-1 ]->lnm_ += lnm;
    }

    // Index-based stick sort
    if ( sortsticks && hasstickidx )
    {
	for ( int idx=0; idx<sticks.size()-1; idx++ )
	{
	    for ( int idy=idx+1; idy<sticks.size(); idy++ )
	    {
		if ( sticks[idx]->stickidx_ > sticks[idy]->stickidx_ )
		    sticks.swap( idx, idy );
	    }
	}
    }

    int sticknr = !sticks.isEmpty() && hasstickidx ? sticks[0]->stickidx_ : 0;

    for ( int idx=0; idx<sticks.size(); idx++ )
    {
	FaultStick* stick = sticks[idx];
	if ( stick->crds_.isEmpty() )
	    continue;

	if ( is2d )
	{
	    mDynamicCastGet(EM::FaultStickSet*,fss,&flt)
	    bool res = fss->geometry().insertStick( sid, sticknr, 0,
					stick->crds_[0], stick->getNormal(),
					linesetmid, stick->lnm_, false );
	}
	else
	{
	    bool res = flt.geometry().insertStick( sid, sticknr, 0,
				stick->crds_[0], stick->getNormal(), false );
	    if ( !res ) continue;
	}

	for ( int crdidx=1; crdidx<stick->crds_.size(); crdidx++ )
	{
	    const RowCol rc( sticknr, crdidx );
	    flt.geometry().insertKnot( sid, rc.getSerialized(),
		    		       stick->crds_[crdidx], false );
	}

	sticknr++;
    }

    deepErase( sticks );
    return true;
}


} // namespace EM
