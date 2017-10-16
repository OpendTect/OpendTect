/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        K. Tingdahl
 Date:          Oct 1999
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "emhorizon2d.h"
#include "emhorizonascio.h"

#include "arrayndimpl.h"
#include "emsurfacetr.h"
#include "emmanager.h"
#include "emrowcoliterator.h"
#include "horizon2dline.h"
#include "ioman.h"
#include "selector.h"
#include "toplist.h"
#include "survinfo.h"
#include "survgeom2d.h"
#include "tabledef.h"
#include "unitofmeasure.h"
#include "zaxistransform.h"
#include "color.h"

#include "hiddenparam.h"

static HiddenParam<EM::Horizon2D,StepInterval<int>*> trackingsampling(0);


namespace EM
{

Horizon2DGeometry::Horizon2DGeometry( Surface& surface )
    : HorizonGeometry( surface )
{}


Geometry::Horizon2DLine*
Horizon2DGeometry::sectionGeometry( const SectionID& sid )
{
    return (Geometry::Horizon2DLine*)SurfaceGeometry::sectionGeometry( sid );
}


const Geometry::Horizon2DLine*
Horizon2DGeometry::sectionGeometry( const SectionID& sid ) const
{
    return (const Geometry::Horizon2DLine*)
	SurfaceGeometry::sectionGeometry( sid );
}


TrcKey::SurvID Horizon2D::getSurveyID() const
{ return TrcKey::std2DSurvID(); }


int Horizon2DGeometry::nrLines() const
{ return geomids_.size(); }

int Horizon2DGeometry::lineIndex( Pos::GeomID geomid ) const
{ return geomids_.indexOf( geomid ); }

int Horizon2DGeometry::lineIndex( const char* linenm ) const
{
    for ( int idx=0; idx<geomids_.size(); idx++ )
    {
	const FixedString curlinenm( Survey::GM().getName(geomids_[idx]) );
	if ( curlinenm == linenm )
	    return idx;
    }

    return -1;
}


const char* Horizon2DGeometry::lineName( int lidx ) const
{ return geomids_.validIdx(lidx) ? Survey::GM().getName( geomids_[lidx] ) : 0; }

Pos::GeomID Horizon2DGeometry::geomID( int idx ) const
{
    return geomids_.validIdx( idx ) ? geomids_[idx]
				    : Survey::GeometryManager::cUndefGeomID();
}


PosID Horizon2DGeometry::getPosID( const TrcKey& trckey ) const
{
    mDynamicCastGet(const EM::Horizon*, hor, &surface_ );

    if ( trckey.survID()!=hor->getSurveyID() )
	return PosID::udf();

    const int lineidx = geomids_.indexOf( trckey.geomID() );
    if ( !geomids_.validIdx( lineidx ))
	return PosID::udf();

    return PosID( surface_.id(), sectionID(0),
		  RowCol(lineidx,trckey.trcNr()).toInt64() );
}


TrcKey Horizon2DGeometry::getTrcKey( const PosID& pid ) const
{
    const RowCol rc = pid.getRowCol();
    return Survey::GM().traceKey(geomID(rc.row()), rc.col() );
}


bool Horizon2DGeometry::includeLine( Pos::GeomID geomid, int step )
{
    return doAddLine( geomid, StepInterval<int>(mUdf(int),mUdf(int),step),
		      true );
}


bool Horizon2DGeometry::addLine( Pos::GeomID geomid, int step )
{
    return doAddLine( geomid, StepInterval<int>(mUdf(int),mUdf(int),step),
		      false );
}


bool Horizon2DGeometry::addLine( Pos::GeomID geomid,
				 const StepInterval<int>& trg )
{ return doAddLine( geomid, trg, false ); }

bool Horizon2DGeometry::doAddLine( Pos::GeomID geomid,
				   const StepInterval<int>& inptrg,
				   bool mergewithdouble )
{
    if ( geomids_.isPresent(geomid) )
	return false;

    mDynamicCastGet( const Survey::Geometry2D*, geom2d,
		     Survey::GM().getGeometry(geomid) );
    if ( !geom2d || geom2d->data().isEmpty() )
	return false;

    StepInterval<int> trcrg = inptrg.isUdf() ? geom2d->data().trcNrRange()
					     : inptrg;
    Geometry::Horizon2DLine* h2dl =
		    reinterpret_cast<Geometry::Horizon2DLine*>( sections_[0] );
    int oldgeomidx = -1;
    for ( int geomidx=0; mergewithdouble && geomidx<geomids_.size(); geomidx++ )
    {
	const int currow = h2dl->getRowIndex( geomids_[geomidx] );
	StepInterval<int> trg = h2dl->colRange( currow );
	trg.limitTo( trcrg );

	const Coord cur0 = h2dl->getKnot( RowCol(currow,trg.start) );
	const Coord cur1 = h2dl->getKnot( RowCol(currow,trg.stop) );
	if ( !trg.width() || !cur0.isDefined() || !cur1.isDefined() )
	    continue;

	PosInfo::Line2DPos new0; geom2d->data().getPos( trg.start, new0 );
	PosInfo::Line2DPos new1; geom2d->data().getPos( trg.stop, new1 );
	if ( !new0.coord_.isDefined() || !new1.coord_.isDefined() )
	    continue;

	const float maxdist = (float) (0.1 * cur0.distTo(cur1) / trg.width());
	if ( cur0.distTo(new0.coord_)>maxdist ||
	     cur1.distTo(new1.coord_)>maxdist )
	    continue;

	oldgeomidx = geomidx;
    }

    for ( int idx=sections_.size()-1; idx>=0; idx-- )
    {
	h2dl = reinterpret_cast<Geometry::Horizon2DLine*>( sections_[idx] );

	if ( oldgeomidx < 0 )
	    h2dl->addUdfRow( geomid, trcrg.start, trcrg.stop, trcrg.step );
	else
	    h2dl->reassignRow( geomids_[oldgeomidx], geomid );

	h2dl->syncRow( geomid, geom2d->data() );
    }

    if ( oldgeomidx < 0 )
	geomids_ += geomid;
    else
	geomids_[oldgeomidx] = geomid;

    return true;
}


void Horizon2DGeometry::removeLine( Pos::GeomID geomid )
{
    const int lidx = geomids_.indexOf( geomid );
    if ( lidx < 0 )
	return;

    geomids_.removeSingle( lidx );
    for ( int idx=sections_.size()-1; idx>=0; idx-- )
    {
	Geometry::Horizon2DLine* section =
	    reinterpret_cast<Geometry::Horizon2DLine*>(sections_[idx]);
	section->removeRow( geomid );
    }
}


PosID Horizon2DGeometry::getNeighbor( const PosID& pid, bool nextcol,
				      bool retundef ) const
{
    TypeSet<PosID> aliases;
    getLinkedPos( pid, aliases );
    aliases += pid;

    const int nraliases = aliases.size();
    for ( int idx=0; idx<nraliases; idx++ )
    {
	const SectionID sid = aliases[idx].sectionID();
	const RowCol ownrc = aliases[idx].getRowCol();
	const int colstep = colRange( sid, ownrc.row() ).step;
	const RowCol neighborrc( ownrc.row(),
		nextcol ? ownrc.col()+colstep : ownrc.col()-colstep );

	if ( surface_.isDefined( sid, neighborrc.toInt64() ) ||
	     (!retundef && idx==nraliases-1) )
	    return PosID( surface_.id(), sid, neighborrc.toInt64() );
    }

    return PosID::udf();
}


int Horizon2DGeometry::getConnectedPos( const PosID& pid,
					TypeSet<PosID>* res ) const
{
    int nrres = 0;
    PosID neighborpid = getNeighbor( pid, true, true );
    if ( neighborpid.objectID()!=-1 )
    {
	nrres++;
	if ( res ) (*res) += neighborpid;
    }

    neighborpid = getNeighbor( pid, false, true );
    if ( neighborpid.objectID()!=-1 )
    {
	nrres++;
	if ( res ) (*res) += neighborpid;
    }

    return nrres;
}


bool Horizon2DGeometry::isAtEdge( const PosID& pid ) const
{
    return getConnectedPos( pid, 0 ) != 2;
}


Geometry::Horizon2DLine* Horizon2DGeometry::createSectionGeometry() const
{ return new Geometry::Horizon2DLine; }


StepInterval<int> Horizon2DGeometry::colRange( const SectionID& sid,
					       Pos::GeomID geomid ) const
{
    const Geometry::Horizon2DLine* geom = sectionGeometry( sectionID(sid) );
    return geom ? geom->colRangeForGeomID( geomid ) : StepInterval<int>(0,0,0);
}


StepInterval<int> Horizon2DGeometry::colRange( Pos::GeomID geomid) const
{
    StepInterval<int> res(0,0,0);
    bool isset = false;

    for ( int idx=0; idx<nrSections(); idx++ )
    {
	StepInterval<int> sectionrg = colRange( sids_[idx], geomid );
	if ( sectionrg.start>sectionrg.stop )
	    continue;
	if ( !isset ) { res = sectionrg; isset=true; }
	else res.include( sectionrg );
    }

    return res;
}


void Horizon2DGeometry::fillPar( IOPar& iopar ) const
{
    const Geometry::Horizon2DLine* cgeom = sectionGeometry( sectionID(0) );
    if ( !cgeom ) return;

    PtrMan<Geometry::Horizon2DLine> geom = cgeom->clone();
    geom->trimUndefParts();
    const int nrlines = geomids_.size();
    iopar.set( Horizon2DGeometry::sKeyNrLines(), nrlines );
    for ( int idx=0; idx<nrlines; idx++ )
    {
	BufferString key = IOPar::compKey( "Line", idx );
	iopar.set( IOPar::compKey(sKey::GeomID(),idx), geomids_[idx] );
	iopar.set( IOPar::compKey(key,Horizon2DGeometry::sKeyTrcRg()),
		   geom->colRangeForGeomID(geomids_[idx]) );
    }
}


bool Horizon2DGeometry::usePar( const IOPar& par )
{
    geomids_.erase();
    int nrlines = 0;
    if ( par.get(Horizon2DGeometry::sKeyNrLines(),nrlines) )
    {
	for ( int idx=0; idx<nrlines; idx++ )
	{
	    Pos::GeomID geomid;
	    if ( !par.get(IOPar::compKey(sKey::GeomID(),idx),geomid) )
	    {
		BufferString key = IOPar::compKey( "Line", idx );
		BufferString idstr;
		if ( par.get(IOPar::compKey(key,Horizon2DGeometry::sKeyID()),
			     idstr))
		{
		    PosInfo::Line2DKey l2dkey;
		    l2dkey.fromString( idstr );
		    if ( S2DPOS().curLineSetID() != l2dkey.lsID() )
			S2DPOS().setCurLineSet( l2dkey.lsID() );

		    geomid = Survey::GM().getGeomID(
				    S2DPOS().getLineSet(l2dkey.lsID()),
				    S2DPOS().getLineName(l2dkey.lineID()) );
		}
	    }

	    mDynamicCastGet(const Survey::Geometry2D*,geom2d,
			    Survey::GM().getGeometry(geomid));
	    if ( !geom2d )
		continue;

	    geomids_ += geomid;
	    for ( int secidx=sections_.size()-1; secidx>=0; secidx-- )
	    {
		Geometry::Horizon2DLine* section =
		reinterpret_cast<Geometry::Horizon2DLine*>( sections_[secidx] );
		section->syncRow( geomid, geom2d->data() );
	    }
	}

	return true;
    }

    TypeSet<int> lineids;
    if ( !par.get(sKeyLineIDs(),lineids) )
	return false;
    BufferStringSet linenames;
    if ( !par.get(sKeyLineNames(),linenames)  )
	return false;

    for ( int idx=0; idx<lineids.size(); idx++ )
    {
	BufferString linesetkey = sKeyLineSets();
	linesetkey += idx;

	MultiID mid;
	if ( !par.get(linesetkey.buf(),mid) ) continue;

	PtrMan<IOObj> ioobj = IOM().get( mid );
	if ( !ioobj ) continue;

	const Pos::GeomID geomid = Survey::GM().getGeomID( ioobj->name(),
							   linenames.get(idx) );
	mDynamicCastGet(const Survey::Geometry2D*,geom2d,
			Survey::GM().getGeometry(geomid));
	if ( !geom2d )
	    continue;

	geomids_ += geomid;
	for ( int secidx=sections_.size()-1; secidx>=0; secidx-- )
	{
	    Geometry::Horizon2DLine* section =
	    reinterpret_cast<Geometry::Horizon2DLine*>( sections_[secidx] );
	    section->syncRow( geomid, geom2d->data() );
	}
    }

    return true;
}


mImplementEMObjFuncs( Horizon2D, EMHorizon2DTranslatorGroup::sGroupName() )

Horizon2D::Horizon2D( EMManager& emm )
    : Horizon(emm)
    , geometry_(*this)
    , nodesource_( 0 )
{
    geometry_.addSection( "", false );
    trackingsampling.setParam( this, new StepInterval<int>(0,0,0) );
}


Horizon2D::~Horizon2D()
{
    delete trackingsampling.getParam( this );
    trackingsampling.removeParam( this );
}


void Horizon2D::initNodeSourceArray( const TrcKey& tk )
{
    mDynamicCastGet(const Survey::Geometry2D*,geom2d,
		    Survey::GM().getGeometry(tk.geomID()));
    if ( !geom2d || geom2d->data().isEmpty() )
	return;

    const StepInterval<int> trcrg = geom2d->data().trcNrRange();
    const int size = trcrg.nrSteps()+1;
    nodesource_ = new Array1DImpl<char>( size );
    nodesource_->setAll('0');
    *trackingsampling.getParam(this) = trcrg;
}


float Horizon2D::getZ( const TrcKey& tk ) const
{
    const Coord3 pos = getCoord( tk );
    return pos.isDefined() ? mCast(float,pos.z) : mUdf(float);
}


bool Horizon2D::setZ( const TrcKey& tk, float z, bool addtohist )
{
    return setPos( sectionID(0), tk.geomID(), tk.trcNr(), z, addtohist );
}


bool Horizon2D::setZAndNodeSourceType( const TrcKey& tk, float z,
    bool addtohist, NodeSourceType type )
{
    if ( !nodesource_ )
	initNodeSourceArray(tk);

    const bool retval = setPos(
	sectionID(0), tk.geomID(), tk.trcNr(), z, addtohist );
    const NodeSourceType tp = !mIsUdf(z) ? type : None;
    setNodeSourceType( tk, tp );
    return retval;
}


void Horizon2D::setNodeSourceType( const TrcKey& tk, NodeSourceType type )
{
    const StepInterval<int>* sampling = trackingsampling.getParam( this );
    if ( !nodesource_ || !sampling )
	return;

    const int idx = sampling->getIndex( tk.trcNr() );
    if ( nodesource_->info().validPos(idx) )
	nodesource_->getData()[idx] = (char)type;
}


bool Horizon2D::isNodeSourceType( const PosID& posid,
				  NodeSourceType type ) const
{
    const TrcKey tk = geometry_.getTrcKey(posid);
    return !tk.isUdf() ? isNodeSourceType( tk, type ) : false;
}


bool Horizon2D::isNodeSourceType( const TrcKey& tk, NodeSourceType type ) const
{
    const StepInterval<int>* sampling = trackingsampling.getParam( this );
    if ( !nodesource_ || !sampling )
	return false;

    const int idx = sampling->getIndex( tk.trcNr() );
    return nodesource_->info().validPos(idx) ?
	nodesource_->getData()[idx]==(char)type : false;
}


bool Horizon2D::hasZ( const TrcKey& tk ) const
{ return !mIsUdf(getZ(tk)); }


Coord3 Horizon2D::getCoord( const TrcKey& tk ) const
{
    const Geometry::Horizon2DLine* line =
		geometry().sectionGeometry( SectionID(0) );
    const int rowidx = line ? line->getRowIndex( tk.geomID() ) : -1;
    if ( rowidx < 0 )
	return Coord3::udf();

    return line->getKnot( RowCol(rowidx,tk.trcNr()) );
}


void Horizon2D::setAttrib( const TrcKey& tk, int attr, int yn, bool addtohist )
{
    const int lineidx = geometry().lineIndex( tk.geomID() );
    if ( lineidx<0 ) return;

    const BinID bid( lineidx, tk.trcNr() );
    const PosID pid( id(), sectionID(0), bid.toInt64() );
    setPosAttrib( pid, attr, yn, addtohist );
}


bool Horizon2D::isAttrib( const TrcKey& tk, int attr ) const
{
    const int lineidx = geometry().lineIndex( tk.geomID() );
    if ( lineidx<0 ) return false;

    const BinID bid( lineidx, tk.trcNr() );
    const PosID pid( id(), sectionID(0), bid.toInt64() );
    return isPosAttrib( pid, attr );
}


float Horizon2D::getZValue( const Coord& c, bool allow_udf, int nr ) const
{
    const int sectionidx = nr;

    const EM::SectionID sectionid = sectionID( sectionidx );
    const Geometry::Horizon2DLine* line =
	geometry().sectionGeometry( sectionid );

    if ( !line )
	return allow_udf ? mUdf(float) : 0;

    TopList<double,double> closestpoints( 2 );

    const StepInterval<int> rowrg = line->rowRange();
    const int nrrows = rowrg.nrSteps()+1;
    for ( int rowidx=0; rowidx<nrrows; rowidx++ )
    {
	StepInterval<int> colrg = line->colRange( rowidx );
	RowCol rowcol( rowrg.atIndex( rowidx ), 0 );

	for ( rowcol.col()=colrg.start; rowcol.col()<=colrg.stop;
	      rowcol.col()+= colrg.step )
	{
	    const Coord3 knot = line->getKnot( rowcol );
	    if ( !knot.isDefined() )
		continue;

	    const double sqdist = c.sqDistTo( knot );
	    if ( mIsZero(sqdist,1e-3) )
		return (float) knot.z;

	    closestpoints.addValue( -sqdist, knot.z );
	}
    }

    if ( closestpoints.isEmpty() )
	return allow_udf ? mUdf(float) : 0;

    if ( closestpoints.size()==1 )
	return (float) closestpoints.getAssociatedValue( 0 );

    const double z0 = closestpoints.getAssociatedValue( 0 );
    const double dist0 = Math::Sqrt( -closestpoints.getValue( 0 ) );
    const double z1 = closestpoints.getAssociatedValue( 1 );
    const double dist1 = Math::Sqrt( -closestpoints.getValue( 1 ) );

    return (float) ((dist1*z0+dist0*z1)/(dist0+dist1));
}


void Horizon2D::removeAll()
{
    Surface::removeAll();
    geometry_.removeAll();
}


void Horizon2D::removeSelected( const Selector<Coord3>& selector,
				TaskRunner* taskrunner )
{
    if ( !selector.isOK() )
	return;

    removebypolyposbox_.setEmpty();
    insideselremoval_ = true;

    for ( int idx=0; idx<nrSections(); idx++ )
    {
	const Geometry::Element* ge = sectionGeometry( sectionID(idx) );
	if ( !ge ) continue;

	TypeSet<EM::SubID> removallist;

	PtrMan<EM::EMObjectIterator> iterator = createIterator( -1 );
	while ( true )
	{
	    const EM::PosID pid = iterator->next();
	    if ( pid.objectID()==-1 )
		break;

	    const Coord3 pos = getPos(pid);
	    if ( selector.includes(pos) )
		removallist += pid.subID();
	}

	removeListOfSubIDs( removallist, sectionID(idx) );
    }
    insideselremoval_ = false;
}


bool Horizon2D::unSetPos( const PosID& pid, bool addtoundo )
{
    Coord3 pos = getPos( pid );
    pos.z = mUdf(float);
    return EMObject::setPos( pid, pos, addtoundo );
}


bool Horizon2D::unSetPos( const EM::SectionID& sid, const EM::SubID& subid,
			  bool addtoundo )
{
    Coord3 pos = getPos( sid, subid );
    pos.z = mUdf(float);
    return EMObject::setPos( sid, subid, pos, addtoundo );
}


Coord3 Horizon2D::getPos( EM::SectionID sid, Pos::GeomID geomid,
			  int trcnr ) const
{
    const Geometry::Horizon2DLine* geom = geometry_.sectionGeometry( sid );
    if ( !geom || geom->isEmpty() )
	return Coord3::udf();

    const int lineidx = geom->getRowIndex( geomid );
    RowCol rc( lineidx, trcnr );
    return geom->getKnot( rc );
}


bool Horizon2D::setPos( const EM::PosID& posid, const Coord3& pos,
			bool addtohistory )
{
    return EMObject::setPos( posid, pos, addtohistory );
}


bool Horizon2D::setPos( const EM::SectionID& sid, const EM::SubID& subid,
			const Coord3& pos, bool addtohistory )
{
    return EMObject::setPos( sid, subid, pos, addtohistory );
}


bool Horizon2D::setPos( EM::SectionID sid, Pos::GeomID geomid, int trcnr,
			float z, bool addtohistory )
{
  Geometry::Horizon2DLine* geom = geometry_.sectionGeometry( sid );
    if ( !geom || geom->isEmpty() )
	return 0;

    const int lineidx = geom->getRowIndex( geomid );
    if ( mIsUdf(lineidx) || lineidx<0 ) return false;

    EM::SubID subid = BinID( lineidx, trcnr ).toInt64();
    Coord3 newpos = EMObject::getPos( sid, subid );
    newpos.z = z;

    return EMObject::setPos( sid, subid, newpos, addtohistory );
}


Coord3 Horizon2D::getPos( const EM::PosID& pid ) const
{ return EMObject::getPos(pid); }

Coord3 Horizon2D::getPos( const EM::SectionID& sid, const EM::SubID& sub ) const
{ return EMObject::getPos(sid,sub); }


Coord3 Horizon2D::getPosition( EM::SectionID sid, int lineidx, int trcnr ) const
{
    return getPos( sid, RowCol(lineidx,trcnr).toInt64() );
}


TypeSet<Coord3> Horizon2D::getPositions( int lineidx, int trcnr ) const
{
    TypeSet<Coord3> crds;
    for ( int idx=0; idx<nrSections(); idx++ )
	crds += getPos( sectionID(idx), RowCol(lineidx,trcnr).toInt64() );
    return crds;
}


bool Horizon2D::setArray1D( const Array1D<float>& arr,
			    SectionID sid, Pos::GeomID geomid,
			    bool onlyfillundefs )
{
    const StepInterval<int> trcrg = geometry_.colRange( geomid );
    return setArray1D( arr, trcrg, sid, geomid, onlyfillundefs );
}


bool Horizon2D::setArray1D( const Array1D<float>& arr,
			    const StepInterval<int>& trcrg,
			    SectionID sid, Pos::GeomID geomid,
			    bool onlyfillundefs )
{
    Geometry::Horizon2DLine* geom = geometry_.sectionGeometry( sid );
    if ( !geom || geom->isEmpty() )
	return false;

    const int lineidx = geom->getRowIndex( geomid );
    if ( lineidx < 0 )
	return false;

    setBurstAlert( true );
    const StepInterval<int> colrg = geom->colRange( lineidx );
    for ( int col=trcrg.start; col<=trcrg.stop; col+=trcrg.step )
    {
	if ( !colrg.includes(col,false) )
	    continue;

	RowCol rc( lineidx, col );
	Coord3 pos = geom->getKnot( rc );
	if ( pos.isDefined() && onlyfillundefs )
	    continue;

	if ( arr.info().validPos(trcrg.getIndex(col)) )
	{
	    float z = arr.get( trcrg.getIndex(col) );
	    pos.z = z;
	    geom->setKnot( rc, pos );
	}
    }

    setBurstAlert( false );
    return true;
}


Array1D<float>* Horizon2D::createArray1D( SectionID sid, Pos::GeomID geomid,
					  const ZAxisTransform* trans ) const
{
    const Geometry::Horizon2DLine* geom = geometry_.sectionGeometry( sid );
    if ( !geom || geom->isEmpty() )
	return 0;

    Array1DImpl<float>* arr = 0;
    const int lineidx = geom->getRowIndex( geomid );
    if ( lineidx < 0 )
	return 0;

    arr = new Array1DImpl<float>( geom->colRange(lineidx).nrSteps() + 1 );
    if ( !arr || !arr->isOK() )
	return 0;

    const StepInterval<int> colrg = geom->colRange( lineidx );
    for ( int col=colrg.start; col<=colrg.stop; col+=colrg.step )
    {
	Coord3 pos = geom->getKnot( RowCol(lineidx,col) );
	float val = (float)pos.z;
	if ( trans )
	    val = trans->transformTrc( TrcKey(geomid,col), val );

	arr->set( colrg.getIndex(col), val );
    }

    return arr;
}


const IOObjContext& Horizon2D::getIOObjContext() const
{ return EMHorizon2DTranslatorGroup::ioContext(); }


Table::FormatDesc* Horizon2DAscIO::getDesc()
{
    Table::FormatDesc* fd = new Table::FormatDesc( "Horizon2D" );
    fd->headerinfos_ += new Table::TargetInfo( "Undefined Value",
			StringInpSpec(sKey::FloatUdf()), Table::Required );
    BufferStringSet hornms;
    createDescBody( fd, hornms );
    return fd;
}


bool Horizon2DAscIO::isFormatOK(  const Table::FormatDesc& fd,
				  BufferString& msg )
{
    const bool trccoldefined = fd.bodyinfos_[3]->selection_.isInFile( 0 );
    const bool xycolsdefined = fd.bodyinfos_[1]->selection_.isInFile( 0 )
			       &&  fd.bodyinfos_[1]->selection_.isInFile( 1 );
     if ( trccoldefined || xycolsdefined )
	 return true;

     msg = "At least one of 'Trace Nr' and 'X Y' columns need to be defined";
     return false;
}


void Horizon2DAscIO::createDescBody( Table::FormatDesc* fd,
				     const BufferStringSet& hornms )
{
    fd->bodyinfos_ += new Table::TargetInfo( "Line name", Table::Required );
    Table::TargetInfo* ti = new Table::TargetInfo( "Position", DoubleInpSpec(),
					    Table::Optional );
    ti->form(0).add( DoubleInpSpec() ); ti->form(0).setName( "X Y" );
    fd->bodyinfos_ += ti;
    Table::TargetInfo* trcspti = new Table::TargetInfo( "", Table::Optional );
    trcspti->form(0).setName( "Trace Nr" );
    Table::TargetInfo::Form* spform =
			new Table::TargetInfo::Form( "SP Nr", IntInpSpec() );
    trcspti->add( spform );
    fd->bodyinfos_ += trcspti;

    for ( int idx=0; idx<hornms.size(); idx++ )
    {
	BufferString fldname = hornms.get( idx );
	ti = new Table::TargetInfo( fldname.buf(), FloatInpSpec(),
			Table::Required, PropertyRef::surveyZType() );
	ti->selection_.unit_ = UnitOfMeasure::surveyDefZUnit();
	fd->bodyinfos_ += ti;
    }
}


void Horizon2DAscIO::updateDesc( Table::FormatDesc& fd,
				 const BufferStringSet& hornms )
{
    fd.bodyinfos_.erase();
    createDescBody( &fd, hornms );
}


#define mErrRet(s) { if ( s ) errmsg_ = s; return 0; }

int Horizon2DAscIO::getNextLine( BufferString& lnm, Coord& crd, int& nr,
				 TypeSet<float>& data )
{
    data.erase();
    if ( !finishedreadingheader_ )
    {
	if ( !getHdrVals(strm_) )
	    return -1;

	udfval_ = getFValue( 0 );
	finishedreadingheader_ = true;
    }

    int ret = getNextBodyVals( strm_ );
    if ( ret <= 0 ) return ret;

    lnm = text( 0 );
    crd.x = getDValue( 1 );
    crd.y = getDValue( 2 );
    nr = getIntValue( 3 );
    const int nrhors = vals_.size() - 4;
    for ( int idx=0; idx<nrhors; idx++ )
	data += getFValue( idx+4, udfval_ );

    return ret;
}


bool Horizon2DAscIO::isTraceNr() const
{
    return formOf( false, 2 ) == 0;
}

} // namespace EM
