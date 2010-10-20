/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        K. Tingdahl
 Date:          Oct 1999
________________________________________________________________________

-*/
static const char* rcsID = "$Id: emhorizon2d.cc,v 1.42 2010-10-20 06:19:59 cvsnanne Exp $";

#include "emhorizon2d.h"

#include "arrayndimpl.h"
#include "emsurfacetr.h"
#include "emmanager.h"
#include "emrowcoliterator.h"
#include "errh.h"
#include "horizon2dline.h"
#include "ioman.h"
#include "selector.h"
#include "survinfo.h"
#include "surv2dgeom.h"
#include "tabledef.h"
#include "unitofmeasure.h"
#include "zaxistransform.h"

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


int Horizon2DGeometry::nrLines() const
{ return geomids_.size(); }


int Horizon2DGeometry::lineIndex( int lineid ) const
{ return lineids_.indexOf( lineid ); }


int Horizon2DGeometry::lineIndex( const char* linenm ) const
{
    const int lineid = PosInfo::POS2DAdmin().getLineNameID( linenm );
    for ( int idx=0; idx<geomids_.size(); idx++ )
    {
	if ( geomids_[idx].lineid_ == lineid )
	    return idx;
    }

    return -1;
}


int Horizon2DGeometry::lineID( int idx ) const
{ return idx>=0 && idx<nrLines() ? lineids_[idx] : -1; }


const char* Horizon2DGeometry::lineName( int lid ) const
{
    const PosInfo::GeomID* geomid = lineGeomID( lid );
    if ( !geomid ) return 0;

    PosInfo::POS2DAdmin().setCurLineSet( geomid->lsid_ );
    return PosInfo::POS2DAdmin().getLineName( geomid->lineid_ );
}


const char* Horizon2DGeometry::lineSet( int lid ) const
{
    const PosInfo::GeomID* geomid = lineGeomID( lid );
    return geomid ? PosInfo::POS2DAdmin().getLineSet( geomid->lsid_ ) : 0;
}


const PosInfo::GeomID* Horizon2DGeometry::lineGeomID( int lid ) const
{
    const int idx = lineids_.indexOf( lid );
    return geomids_.validIdx(idx) ? &geomids_[idx] : 0;
}


int Horizon2DGeometry::addLine( const PosInfo::GeomID& geomid, int step )
{ return addLine( geomid, StepInterval<int>(0,0,step) ); }


int Horizon2DGeometry::addLine( const PosInfo::GeomID& geomid,
				const StepInterval<int>& trcrg )
{
    if ( !geomid.isOK() ) return -1;

    geomids_ += geomid;
    PosInfo::POS2DAdmin().setCurLineSet( geomid.lsid_ );
    PosInfo::Line2DData linegeom(
	PosInfo::POS2DAdmin().getLineName(geomid.lineid_) );
    if ( !PosInfo::POS2DAdmin().getGeometry(linegeom) )
	return -1;

    for ( int idx=sections_.size()-1; idx>=0; idx-- )
    {
	Geometry::Horizon2DLine* section =
		reinterpret_cast<Geometry::Horizon2DLine*>( sections_[idx] );
	const int lineid =
	    section->addUdfRow( trcrg.start, trcrg.stop, trcrg.step );
	if ( idx )
	    continue;
	section->syncRow( lineid, linegeom );

	lineids_ += lineid;
    }

    return lineids_[lineids_.size()-1];
}


void Horizon2DGeometry::removeLine( int lid )
{
    const int lidx = lineids_.indexOf( lid );
    if ( lidx<0 || lidx>=geomids_.size() )
	return;

    lineids_.remove( lidx );
    geomids_.remove( lidx );
    for ( int idx=sections_.size()-1; idx>=0; idx-- )
    {
	Geometry::Horizon2DLine* section =
	    reinterpret_cast<Geometry::Horizon2DLine*>(sections_[idx]);
	section->removeRow( lid );
    }
}


PosID Horizon2DGeometry::getNeighbor( const PosID& pid, bool nextcol,
				      bool retundef ) const
{
    const RowCol rc( pid.subID() );
    TypeSet<PosID> aliases;
    getLinkedPos( pid, aliases );
    aliases += pid;

    const int nraliases = aliases.size();
    for ( int idx=0; idx<nraliases; idx++ )
    {
	const SectionID sid = aliases[idx].sectionID();
	const RowCol ownrc( aliases[idx].subID() );
	const int colstep = colRange( sid, ownrc.row ).step;
	const RowCol neighborrc( ownrc.row,
		nextcol ? ownrc.col+colstep : ownrc.col-colstep );

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


void Horizon2DGeometry::fillPar( IOPar& par ) const
{
    for ( int idx=0; idx<geomids_.size(); idx++ )
    {
	SeparString linekey( "Line", '.' );
	linekey.add( idx );
	
	SeparString lineidkey( linekey.buf(), '.' );
	lineidkey.add( Horizon2DGeometry::sKeyID() );
	par.set( lineidkey.buf(), geomids_[idx].lsid_, geomids_[idx].lineid_ );

	SeparString linetrcrgkey( linekey.buf(), '.' );
	linetrcrgkey.add( Horizon2DGeometry::sKeyTrcRg() );
	Geometry::Horizon2DLine geom = *sectionGeometry( sectionID(0) );
	par.set( linetrcrgkey.buf(), geom.colRange(lineID(idx)) );

	/*BufferString linesetkey = sKeyLineSets();
	linesetkey += idx;
	par.set( linesetkey.buf(), linesets_[idx] );
	geom.trimUndefParts();
	BufferString trcrangekey = sKeyTraceRange();
	trcrangekey += idx;
	par.set( trcrangekey, geom.colRange(lineID(idx)) );*/
    }

    par.set( Horizon2DGeometry::sKeyNrLines(), geomids_.size() );
}


bool Horizon2DGeometry::usePar( const IOPar& par )
{
    lineids_.erase(); geomids_.erase();
    if ( par.find(Horizon2DGeometry::sKeyNrLines()) )
    {
	int nrlines = 0;
	par.get( Horizon2DGeometry::sKeyNrLines(), nrlines );
	for ( int idx=0; idx<nrlines; idx++ )
	{
	    SeparString geomstr( "Line", '.' );
	    geomstr.add( idx );
	    geomstr.add( Horizon2DGeometry::sKeyID() );
	    int linesetid =-1;
	    int lineid = -1;
	    if ( !par.get(geomstr.buf(),linesetid,lineid) ||
		 linesetid < 0 || lineid < 0 )
		continue;
	    
	    PosInfo::GeomID geomid( linesetid, lineid );
	    geomids_ += geomid;
	    
	    PosInfo::POS2DAdmin().setCurLineSet( geomid.lsid_ );
	    PosInfo::Line2DData linegeom(
		    PosInfo::POS2DAdmin().getLineName(geomid.lineid_) );
	    if ( !PosInfo::POS2DAdmin().getGeometry(linegeom) )
		continue;

	    for ( int secidx=sections_.size()-1; secidx>=0; secidx-- )
	    {
		Geometry::Horizon2DLine* section =
		reinterpret_cast<Geometry::Horizon2DLine*>( sections_[secidx] );
		section->syncRow( lineids_[idx], linegeom );
	    }
	}

	return true;
    }

    if ( !par.get(sKeyLineIDs(),lineids_) )
	return false;
    BufferStringSet linenames;
    if ( !par.get(sKeyLineNames(),linenames)  )
     	return false;	

    for ( int idx=0; idx<lineids_.size(); idx++ )
    {
	BufferString linesetkey = sKeyLineSets();
	linesetkey += idx;

	MultiID mid;
	if ( !par.get(linesetkey.buf(),mid) ) continue;
	
	PtrMan<IOObj> ioobj = IOM().get( mid );
	if ( !ioobj ) continue;

	PosInfo::GeomID geomid(
		PosInfo::POS2DAdmin().getLineSetID(ioobj->name()),
		PosInfo::POS2DAdmin().getLineNameID(linenames[idx]->buf()) );
	if ( !geomid.isOK() ) continue;
	geomids_ += geomid;

	PosInfo::Line2DData linegeom( linenames[idx]->buf() );
	if ( !PosInfo::POS2DAdmin().getGeometry(linegeom) )
	    continue;

	for ( int secidx=sections_.size()-1; secidx>=0; secidx-- )
	{
	    Geometry::Horizon2DLine* section =
		reinterpret_cast<Geometry::Horizon2DLine*>( sections_[secidx] );
	    section->syncRow( lineids_[idx], linegeom );
	}
    }

    return true;
}


mImplementEMObjFuncs( Horizon2D, EMHorizon2DTranslatorGroup::keyword() )


Horizon2D::Horizon2D( EMManager& emm )
    : Horizon(emm)
    , geometry_(*this)
{
    geometry_.addSection( "", false );
}


Horizon2D::~Horizon2D()
{}


void Horizon2D::removeAll()
{
    Surface::removeAll();
    geometry_.removeAll();
}


void Horizon2D::removeSelected( const Selector<Coord3>& selector,
				TaskRunner* tr )
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
    return setPos( pid, pos, addtoundo );
}


bool Horizon2D::unSetPos( const EM::SectionID& sid, const EM::SubID& subid,
			  bool addtoundo )
{
    Coord3 pos = getPos( sid, subid );
    pos.z = mUdf(float);
    return setPos( sid, subid, pos, addtoundo );
}

Coord3 Horizon2D::getPos( const EM::PosID& pid ) const
{ return EMObject::getPos(pid); }

Coord3 Horizon2D::getPos( const EM::SectionID& sid, const EM::SubID& sub ) const
{ return EMObject::getPos(sid,sub); }


Coord3 Horizon2D::getPos( EM::SectionID sid, int lineidx, int trcnr ) const
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


bool Horizon2D::setArray1D( const Array1D<float>& arr, SectionID sid, int lid,
       			    bool onlyfillundefs	)
{
    Geometry::Horizon2DLine* geom = geometry_.sectionGeometry( sid );
    if ( !geom || geom->isEmpty() )
	return 0;

    const StepInterval<int> colrg = geom->colRange( lid );
    for ( int col=colrg.start; col<=colrg.stop; col+=colrg.step )
    {
	RowCol rc( lid, col );
	Coord3 pos = geom->getKnot( rc );
	if ( pos.isDefined() && onlyfillundefs )
	    continue;

	if ( arr.info().validPos(colrg.getIndex(col)) )
	{
	    float z = arr.get( colrg.getIndex(col) );
	    pos.z = z;
	    geom->setKnot( rc, pos );
	}
    }

    return true;
}


Array1D<float>* Horizon2D::createArray1D( SectionID sid, int lineid,
					  const ZAxisTransform* trans ) const
{
    const Geometry::Horizon2DLine* geom = geometry_.sectionGeometry( sid );
    if ( !geom || geom->isEmpty() )
	return 0;

    Array1DImpl<float>* arr = 0;
    arr = new Array1DImpl<float>( geom->colRange(lineid).nrSteps() + 1 );

    if ( !arr && !arr->isOK() )
	return 0;

    const StepInterval<int> colrg = geom->colRange( lineid );
    for ( int col=colrg.start; col<=colrg.stop; col+=colrg.step )
    {
	Coord3 pos = geom->getKnot( RowCol(lineid,col) );
	if ( trans )
	    pos.z = trans->transform( pos );

	arr->set( colrg.getIndex(col), pos.z );
    }

    return arr;
}


const IOObjContext& Horizon2D::getIOObjContext() const
{ return EMHorizon2DTranslatorGroup::ioContext(); }


Table::FormatDesc* Horizon2DAscIO::getDesc() 
{
    Table::FormatDesc* fd = new Table::FormatDesc( "Horizon2D" );
    fd->headerinfos_ += new Table::TargetInfo( "Undefined Value",
	    		StringInpSpec(sKey::FloatUdf), Table::Required );
    BufferStringSet hornms;
    createDescBody( fd, hornms );
    return fd;
}


bool Horizon2DAscIO::isFormatOK(  const Table::FormatDesc& fd,
				  BufferString& msg )
{
    const bool trccoldefined = fd.bodyinfos_[2]->selection_.isInFile( 0 );
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
    fd->bodyinfos_ += new Table::TargetInfo( "Trace nr", IntInpSpec(),
	    				     Table::Optional );
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

int Horizon2DAscIO::getNextLine( BufferString& lnm, Coord& crd, int& trcnr,
     				 TypeSet<float>& data )
{
    data.erase();
    if ( !finishedreadingheader_ )
    {
	if ( !getHdrVals(strm_) )
	    return -1;

	udfval_ = getfValue( 0 );
	finishedreadingheader_ = true;
    }

    int ret = getNextBodyVals( strm_ );
    if ( ret <= 0 ) return ret;

    lnm = text( 0 );
    crd.x = getdValue( 1 );
    crd.y = getdValue( 2 );
    trcnr = getIntValue( 3 );
    const int nrhors = vals_.size() - 4;
    for ( int idx=0; idx<nrhors; idx++ )
	data += getfValue( idx+4, udfval_ );

    return ret;
}

} // namespace EM
