/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        K. Tingdahl
 Date:          Oct 1999
________________________________________________________________________

-*/

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
{
    const Geometry::Horizon2DLine* hor2dline = sectionGeometry( SectionID(0) );
    return hor2dline ? hor2dline->nrLines() : 0;
}


int Horizon2DGeometry::lineIndex( Pos::GeomID geomid ) const
{
    const Geometry::Horizon2DLine* hor2dline = sectionGeometry( SectionID(0) );
    return hor2dline ? hor2dline->getRowIndex( geomid ) : -1;
}


int Horizon2DGeometry::lineIndex( const char* linenm ) const
{
    return lineIndex( Survey::GM().getGeomID(linenm) );
}


const char* Horizon2DGeometry::lineName( int lidx ) const
{ return Survey::GM().getName( geomID(lidx) ); }

Pos::GeomID Horizon2DGeometry::geomID( int lineidx ) const
{
    const Geometry::Horizon2DLine* hor2dline = sectionGeometry( SectionID(0) );
    return hor2dline ? hor2dline->geomID( lineidx ) : mUdfGeomID;
}


void Horizon2DGeometry::getGeomIDs( TypeSet<Pos::GeomID>& geomids ) const
{
    const Geometry::Horizon2DLine* hor2dline = sectionGeometry( SectionID(0) );
    if ( hor2dline )
	hor2dline->getGeomIDs( geomids );
}


PosID Horizon2DGeometry::getPosID( const TrcKey& trckey ) const
{
    mDynamicCastGet(const EM::Horizon*, hor, &surface_ );

    if ( trckey.survID()!=hor->getSurveyID() )
	return PosID::udf();

    const int lineidx = lineIndex( trckey.geomID() );
    if ( lineidx < 0 )
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
    if ( lineIndex(geomid) >= 0 )
	return false;

    mDynamicCastGet( const Survey::Geometry2D*, geom2d,
		     Survey::GM().getGeometry(geomid) );
    if ( !geom2d || geom2d->data().isEmpty() )
	return false;

    StepInterval<int> trcrg = inptrg.isUdf() ? geom2d->data().trcNrRange()
					     : inptrg;
    Geometry::Horizon2DLine* h2dl =
		    reinterpret_cast<Geometry::Horizon2DLine*>( sections_[0] );
    int oldlineidx = -1;
    for ( int lineidx=0; mergewithdouble && lineidx<nrLines(); lineidx++ )
    {
	StepInterval<int> trg = h2dl->colRange( lineidx );
	trg.limitTo( trcrg );

	const Coord cur0 = h2dl->getKnot( RowCol(lineidx,trg.start) );
	const Coord cur1 = h2dl->getKnot( RowCol(lineidx,trg.stop) );
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

	oldlineidx = lineidx;
    }

    if ( oldlineidx < 0 )
	h2dl->addUdfRow( geomid, trcrg.start, trcrg.stop, trcrg.step );
    else
	h2dl->reassignRow( geomID(oldlineidx), geomid );

    h2dl->syncRow( geomid, geom2d->data() );

    return true;
}


void Horizon2DGeometry::removeLine( Pos::GeomID geomid )
{
    Geometry::Horizon2DLine* h2dline = sectionGeometry( SectionID(0) );
    h2dline->removeRow( geomid );
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
    return colRange( SectionID(0), geomid );
}


void Horizon2DGeometry::fillPar( IOPar& iopar ) const
{
    const Geometry::Horizon2DLine* cgeom = sectionGeometry( sectionID(0) );
    if ( !cgeom ) return;

    PtrMan<Geometry::Horizon2DLine> geom = cgeom->clone();
    geom->trimUndefParts();
    const int nrlines = nrLines();
    iopar.set( Horizon2DGeometry::sKeyNrLines(), nrlines );
    for ( int idx=0; idx<nrlines; idx++ )
    {
	BufferString key = IOPar::compKey( "Line", idx );
	iopar.set( IOPar::compKey(sKey::GeomID(),idx), geomID(idx) );
	iopar.set( IOPar::compKey(key,Horizon2DGeometry::sKeyTrcRg()),
		   geom->colRange(idx) );
    }
}


bool Horizon2DGeometry::usePar( const IOPar& par )
{
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
    , trackingsampling_(0,0,0)
{
    geometry_.addSection( "", false );
}


Horizon2D::~Horizon2D()
{
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
    trackingsampling_ = trcrg;
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
    if ( !nodesource_ )
	return;

    const int idx = trackingsampling_.getIndex( tk.trcNr() );
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
    if ( !nodesource_ )
	return false;

    const int idx = trackingsampling_.getIndex( tk.trcNr() );
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

    const Geometry::Element* ge = sectionGeometry( sectionID(0) );
    if ( !ge ) return;

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

    removeListOfSubIDs( removallist, sectionID(0) );
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
    crds += getPos( sectionID(0), RowCol(lineidx,trcnr).toInt64() );
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
    Table::TargetInfo* ti = Table::TargetInfo::mkHorPosition( false, false );
    fd->bodyinfos_ += ti;
    Table::TargetInfo* trcspti = new Table::TargetInfo( "", IntInpSpec(),
							Table::Optional );
    trcspti->form(0).setName( "Trace Nr" );
    Table::TargetInfo::Form* spform =
			new Table::TargetInfo::Form( "SP Nr", FloatInpSpec() );
    trcspti->add( spform );
    fd->bodyinfos_ += trcspti;

    for ( int idx=0; idx<hornms.size(); idx++ )
    {
	BufferString fldname = hornms.get( idx );
	ti = new Table::TargetInfo( fldname.buf(), FloatInpSpec(),
			Table::Required, Mnemonic::surveyZType() );
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
				 float& spnr, TypeSet<float>& data )
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

    lnm = getText( 0 );
    crd = getPos( 1, 2 );
    if ( isTraceNr() )
	nr = getIntValue( 3 );
    else
	spnr = getFValue( 3 );

    const int nrhors = vals_.size() - 4;
    for ( int idx=0; idx<nrhors; idx++ )
	data += getFValue( idx+4, udfval_ );

    return ret;
}


bool Horizon2DAscIO::isTraceNr() const
{
    return formOf(false,2) == 0;
}

} // namespace EM
