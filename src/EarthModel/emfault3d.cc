/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        N. Fredman
 Date:          Sep 2002
________________________________________________________________________

-*/
static const char* rcsID = "$Id: emfault3d.cc,v 1.7 2009-03-11 08:18:46 cvsjaap Exp $";

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


double distTo( const FaultStick& fs, double zscale ) const
{
/*  Take distance between the two end points of each stick. Difference
    of both distances is minimum amount of rope required to connect both
    sticks when moving freely. Our distance measure equals the amount of
    extra rope required when both sticks are fixed in space. */
   
    if ( crds_.isEmpty() || fs.crds_.isEmpty() )
	return mUdf(double);
   
    Coord3 a0 = crds_[0];
    Coord3 a1 = crds_[crds_.size()-1];
    Coord3 b0 = fs.crds_[0];
    Coord3 b1 = fs.crds_[fs.crds_.size()-1];

    if ( zscale==MAXDOUBLE )
    {
	a0.x=0; a0.y=0; a1.x=0; a1.y=0; b0.x=0; b0.y=0; b1.x=0; b1.y=0; 
    }
    else 
    {
	a0.z *= zscale; a1.z *= zscale; b0.z *= zscale; b1.z *= zscale;
    }

    const double mindist  = fabs( a0.distTo(a1)-b0.distTo(b1) );
    const double straight = a0.distTo(b0) + a1.distTo(b1) - mindist;
    const double crossed  = a0.distTo(b1) + a1.distTo(b0) - mindist;

    return mMIN( straight, crossed );
}


Coord3 getNormal() const
{
    int oninl = 0; int oncrl = 0; int ontms = 0;

    for ( int idx=0; idx<crds_.size()-1; idx++ )
    {
	const BinID bid0 = SI().transform( crds_[idx] );
	for ( int idy=idx+1; idy<crds_.size(); idy++ )
	{
	    const BinID bid1 = SI().transform( crds_[idy] );
	    if ( bid0.inl == bid1.inl )
		oninl++;
	    if ( bid0.crl == bid1.crl )
		oncrl++;
	    if ( fabs(crds_[idx].z-crds_[idy].z) < fabs(0.5*SI().zStep()) )
		ontms++;
	}
    }

    if ( ontms==oninl && ontms==oncrl )
	return Coord3::udf();

    if ( ontms>=oncrl && ontms>=oninl )
	return Coord3( 0, 0, 1 );

    if ( oninl==oncrl )
	return Coord3( Coord::udf(), 0 );

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

    // Analyse normals
    bool sticksontimeslice = false;
    bool sticksoninlcrl = false;

    TypeSet<Coord3> normals;
    for ( int idx=0; idx<sticks.size(); idx++ )
    {
	normals += sticks[idx]->getNormal();
	if ( mIsZero(normals[idx].z, mDefEps) )
	    sticksoninlcrl = true;
	else if ( normals[idx].isDefined() )
	    sticksontimeslice = true;
    }

    double zscale = SI().zScale();
    if ( sticksoninlcrl && !sticksontimeslice )
	zscale = 0;
    if ( !sticksoninlcrl && sticksontimeslice )
	zscale = MAXDOUBLE;

    // Geometrically based stick sort
    if ( sortsticks && !hasstickidx && sticks.size()>2 )
    {
	double mindist = MAXDOUBLE;
	int minidx0, minidx1;

	for ( int idx=0; idx<sticks.size()-1; idx++ )
	{
	    for ( int idy=idx+1; idy<sticks.size(); idy++ )
	    {
		const double dist = sticks[idx]->distTo( *sticks[idy], zscale );
		if ( dist < mindist )
		{
		    mindist = dist;
		    minidx0 = idx;
		    minidx1 = idy;
		}
	    }
	}
	sticks.swap( 0, minidx0 );
	sticks.swap( 1, minidx1 );

	for ( int tailidx=1; tailidx<sticks.size()-1; tailidx++ )
	{
	    mindist = MAXDOUBLE;
	    bool reverse = false;
	    for ( int idy=tailidx+1; idy<sticks.size(); idy++ )
	    {
		const double dist0 = sticks[0]->distTo( *sticks[idy], zscale );
		const double dist1 = sticks[tailidx]->distTo( *sticks[idy],
							      zscale );
		if ( mMIN(dist0,dist1) < mindist )
		{
		    mindist = mMIN( dist0, dist1 );
		    minidx0 = idy;
		    reverse = dist0 < dist1;
		}
	    }
	    for ( int idx=0; reverse && idx<tailidx*0.5; idx++ )
		sticks.swap( idx, tailidx-idx );

	    sticks.swap( tailidx+1, minidx0 );
	}
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

    // Consult neighboring sticks to resolve undefined normals
    for ( int idx=0; idx<sticks.size(); idx++ )
    {
	if ( normals[idx].isDefined() )
	    continue;

	for ( int idy=(idx ? idx-1 : idx+1); idy<sticks.size(); idy++ )
	{
	    if ( !normals[idy].isDefined() )
		continue;
	    if ( mIsUdf(normals[idx].z) || normals[idx].z==normals[idy].z )
	    {
		normals[idx] = normals[idy];
		break;
	    }
	}
	if ( !normals[idx].isDefined() )
	    normals[idx] = Coord3( SI().binID2Coord().rowDir(), 0 );
    }

    //Create fault
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
					stick->crds_[0], normals[idx],
					linesetmid, stick->lnm_, false );
	}
	else
	{
	    bool res = flt.geometry().insertStick( sid, sticknr, 0,
					stick->crds_[0], normals[idx], false );
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
