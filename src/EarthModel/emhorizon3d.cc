/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        K. Tingdahl
 Date:          Oct 1999
________________________________________________________________________

-*/
static const char* rcsID mUnusedVar = "$Id: emhorizon3d.cc,v 1.141 2012-08-08 05:47:54 cvssalil Exp $";

#include "emhorizon3d.h"

#include "array2dinterpol.h"
#include "arrayndimpl.h"
#include "binidsurface.h"
#include "binidvalset.h"
#include "datapointset.h"
#include "emmanager.h"
#include "emrowcoliterator.h"
#include "emsurfaceauxdata.h"
#include "emsurfaceedgeline.h"
#include "emsurfacetr.h"
#include "emundo.h"
#include "executor.h"
#include "ioobj.h"
#include "pickset.h"
#include "posprovider.h"
#include "ptrman.h"
#include "scaler.h"
#include "survinfo.h"
#include "tabledef.h"
#include "trigonometry.h"
#include "unitofmeasure.h"
#include "zaxistransform.h"

#include <math.h>

namespace EM {

class AuxDataImporter : public Executor
{
public:

AuxDataImporter( Horizon3D& hor, const ObjectSet<BinIDValueSet>& sects,
		 const BufferStringSet& attribnames, const int start,
       		 HorSampling hs	)
    : Executor("Data Import")
    , horizon_(hor)
    , bvss_(sects)
    , startidx_(start)
    , totalnr_(0)
    , nrdone_(0)
    , hs_(hs)
    , inl_(0)
    , sectionidx_(-1)
    , nrattribs_(-1)
{
    if ( bvss_.isEmpty() || attribnames.isEmpty() )
	{ msg_ = "Internal error: empty input"; return; }
    nrattribs_ = attribnames.size();
    for ( int idx=0; idx<bvss_.size(); idx++ )
	totalnr_ += bvss_[idx]->nrInls();
    if ( totalnr_ < 1 )
	{ msg_ = "No valid data found"; return; }

    for ( int iattr=0; iattr<nrattribs_; iattr++ )
    {
	BufferString nm = attribnames.get( iattr );
	if ( nm.isEmpty() )
	    { nm = "Imported attribute "; nm += iattr + 1; }
	attrindexes_ += horizon_.auxdata.addAuxData( nm.buf() );
    }

    if ( nrattribs_ == 1 )
	{ msg_ = "Getting "; msg_ += attribnames.get( 0 ); }
    else
	msg_ = "Getting horizon attribute values";
}


int nextStep()
{
    if ( nrattribs_ < 0 ) return ErrorOccurred();

    if ( sectionidx_ == -1 || inl_ > inlrg_.stop )
    {
	sectionidx_++;
	if ( sectionidx_ >= horizon_.geometry().nrSections() )
	    return Finished();

	const SectionID sectionid = horizon_.geometry().sectionID(sectionidx_);
	const Geometry::BinIDSurface* rcgeom =
	    horizon_.geometry().sectionGeometry( sectionid );
	if ( !rcgeom ) return ErrorOccurred();

	inlrg_ = rcgeom->rowRange();
	crlrg_ = rcgeom->colRange();
	inl_ = inlrg_.start;
    }

    PosID posid( horizon_.id(), horizon_.geometry().sectionID(sectionidx_) );
    const BinIDValueSet& bvs = *bvss_[sectionidx_];
    for ( int crl=crlrg_.start; crl<=crlrg_.stop; crl+=crlrg_.step )
    {
	const BinID bid( inl_, crl );
	if ( !hs_.includes(bid) ) continue;

	BinIDValueSet::Pos pos = bvs.findFirst( bid );
	if ( !pos.valid() ) continue;

	const float* vals = bvs.getVals( pos );
	if ( !vals ) continue;
	posid.setSubID( bid.toInt64() );
	for ( int iattr=0; iattr<nrattribs_; iattr++ )
	{
	    const float val = vals[iattr+startidx_];
	    if ( !mIsUdf(val) )
		horizon_.auxdata.setAuxDataVal( attrindexes_[iattr],
						posid, val );
	}
    }

    inl_ += inlrg_.step;
    nrdone_++;
    return MoreToDo();
}


const char*	message() const		{ return msg_; }
od_int64	totalNr() const		{ return totalnr_; }
od_int64	nrDone() const		{ return nrdone_; }
const char*	nrDoneText() const	{ return "Positions handled"; }

protected:

    const ObjectSet<BinIDValueSet>&	bvss_;
    Horizon3D&			horizon_;
    const HorSampling		hs_;
    BufferString		msg_;
    int				nrattribs_;
    int				startidx_;
    TypeSet<int>		attrindexes_;

    int				inl_;
    StepInterval<int>		inlrg_;
    StepInterval<int>		crlrg_;
    int				sectionidx_;

    int				totalnr_;
    int				nrdone_;
};


class HorizonImporter : public Executor
{
public:

HorizonImporter( Horizon3D& hor, const ObjectSet<BinIDValueSet>& sects, 
		 const HorSampling& hs )
    : Executor("Horizon Import")
    , horizon_(hor)
    , bvss_(sects)
    , totalnr_(0)
    , nrdone_(0)
    , hs_(hs)
    , sectionidx_(0)
    , nrvals_(-1)
    , msg_("Adding nodes")
{
    if ( bvss_.isEmpty() ) return;
    nrvals_ = bvss_[0]->nrVals();
    const RowCol step( hs_.step.inl, hs_.step.crl );
    horizon_.geometry().setStep( step, step );

    for ( int idx=0; idx<bvss_.size(); idx++ )
    {
	const BinIDValueSet& bvs = *bvss_[idx];
	if ( bvs.nrVals() != nrvals_ )
	    { msg_ = "Incompatible sections"; return; }

	totalnr_ += bvs.totalSize();
	EM::SectionID sid = horizon_.geometry().addSection( 0, false );

	Geometry::BinIDSurface* geom = horizon_.geometry().sectionGeometry(sid);
	HorSampling sectrg;
	sectrg.set( bvs.inlRange(), bvs.crlRange(-1) );
	sectrg.step = step;
	sectrg.limitTo( hs_ );
	mDeclareAndTryAlloc( Array2D<float>*, arr,
		Array2DImpl<float>( sectrg.nrInl(), sectrg.nrCrl() ) );
	arr->setAll( mUdf(float) );
	geom->setArray( sectrg.start, sectrg.step, arr, true );

    }

    horizon_.enableGeometryChecks( false );
}


const char*	message() const		{ return msg_; }
od_int64	totalNr() const		{ return totalnr_; }
od_int64	nrDone() const		{ return nrdone_; }
const char*	nrDoneText() const	{ return "Positions handled"; }

int nextStep()
{
    if ( nrvals_ == -1 ) return ErrorOccurred();

    if ( sectionidx_ >= bvss_.size() )
    {
	horizon_.enableGeometryChecks( true );
	return Finished();
    }

    const BinIDValueSet& bvs = *bvss_[sectionidx_];
    const EM::SectionID sid = horizon_.sectionID( sectionidx_ );
    Geometry::BinIDSurface* surf = horizon_.geometry().sectionGeometry( sid );
    if ( !surf )
	return ErrorOccurred();

    BinID bid;
    for ( int idx=0; idx<10000; idx++ )
    {
	nrdone_++;
	if ( !bvs.next(pos_) )
	{
	    sectionidx_++;
	    pos_.reset();
	    return MoreToDo();
	}

	bvs.get( pos_, bid );
	if ( !hs_.includes(bid) )
	    continue;

	const float z = bvs.getVals(pos_)[ 0 ];
	surf->setKnot( RowCol(bid), Coord3(0,0,z) );
    }

    return MoreToDo();
}

protected:

    const ObjectSet<BinIDValueSet>&	bvss_;
    Horizon3D&		horizon_;
    BinIDValueSet::Pos	pos_;
    HorSampling		hs_;
    BufferString	msg_;
    int			nrvals_;

    int			sectionidx_;
    int			totalnr_;
    int			nrdone_;
};

// EM::Horizon3D

Horizon3D::Horizon3D( EMManager& man )
    : Horizon(man)
    , geometry_( *this )
    , auxdata( *new SurfaceAuxData(*this) )
    , edgelinesets( *new EdgeLineManager(*this) )
{
    geometry_.addSection( "", false );
}


Horizon3D::~Horizon3D()
{
    delete &auxdata;
    delete &edgelinesets;
}


void Horizon3D::removeAll()
{
    Surface::removeAll();
    geometry_.removeAll();
    auxdata.removeAll();
    edgelinesets.removeAll();
}


void Horizon3D::fillPar( IOPar& par ) const
{
    Surface::fillPar( par );
    Horizon::fillPar( par );
    auxdata.fillPar( par );
    edgelinesets.fillPar( par );
}


bool Horizon3D::usePar( const IOPar& par )
{
    return Surface::usePar( par ) &&
	auxdata.usePar( par ) &&
	edgelinesets.usePar( par ) &&
	Horizon::usePar( par );
}


Horizon3DGeometry& Horizon3D::geometry()
{ return geometry_; }


const Horizon3DGeometry& Horizon3D::geometry() const
{ return geometry_; }


mImplementEMObjFuncs( Horizon3D, EMHorizon3DTranslatorGroup::keyword() );

bool Horizon3D::setZ( const BinID& bid, float z, bool addtohist )
{ return setPos( sectionID(0), bid.toInt64(), Coord3(0,0,z), addtohist ); }

float Horizon3D::getZ( const BinID& bid ) const
{ return (float) getPos( sectionID(0), bid.toInt64() ).z; }


float Horizon3D::getZValue( const Coord& c, bool allow_udf, int nr ) const
{
    //TODO support the parameters
    return getZ( SI().transform(c) );
}


HorSampling Horizon3D::range( SectionID sid ) const
{
    HorSampling hs( false );
    hs.set( geometry().rowRange(sid), geometry().colRange(sid,-1) );
    return hs;
}


Array2D<float>* Horizon3D::createArray2D( 
		    SectionID sid, const ZAxisTransform* zaxistransform ) const
{
    const Geometry::BinIDSurface* geom = geometry_.sectionGeometry( sid );
    if ( !geom || geom->isEmpty() )
	return 0;

    Array2DImpl<float>* arr = 0;
    if ( zaxistransform || !geom->getArray() )
    {
	const StepInterval<int> rowrg = geom->rowRange();
	const StepInterval<int> colrg = geom->colRange();

	arr = new Array2DImpl<float>( rowrg.nrSteps()+1, colrg.nrSteps()+1 );
	if ( arr && arr->isOK() )
	{
	    for ( int row=rowrg.start; row<=rowrg.stop; row+=rowrg.step )
	    {
		for ( int col=colrg.start; col<=colrg.stop; col+=colrg.step )
		{
		    Coord3 pos = geom->getKnot( RowCol(row,col), false );
		    pos.z = zaxistransform->transform( pos );

		    arr->set( rowrg.getIndex(row), colrg.getIndex(col), (float) pos.z );
		}
	    }
	}
    }
    else
	arr = new Array2DImpl<float>( *geom->getArray() );

    if ( arr && !arr->isOK() )
    {
	delete arr;
	arr = 0;
    }

    return arr;
}


bool Horizon3D::setArray2D( const Array2D<float>& arr, SectionID sid, 
			    bool onlyfillundefs, const char* undodesc )
{
    const Geometry::BinIDSurface* geom = geometry_.sectionGeometry( sid );
    if ( !geom || geom->isEmpty() )
	return 0;

    const StepInterval<int> rowrg = geom->rowRange();
    const StepInterval<int> colrg = geom->colRange();

    const RowCol startrc( rowrg.start, colrg.start );
    const RowCol stoprc( rowrg.stop, colrg.stop );
    geometry().sectionGeometry( sid )->expandWithUdf( startrc, stoprc );
    
    int poscount = 0;
    geometry().sectionGeometry( sid )->blockCallBacks( true, false );
    const bool didcheck = geometry().enableChecks( false );
    setBurstAlert( true );

    Array2D<float>* oldarr = undodesc ? createArray2D( sid, 0 ) : 0;

    const int arrnrrows = arr.info().getSize( 0 );
    const int arrnrcols = arr.info().getSize( 1 );

    for ( int row=rowrg.start; row<=rowrg.stop; row+=rowrg.step )
    {
	for ( int col=colrg.start; col<=colrg.stop; col+=colrg.step )
	{
	    const RowCol rc( row, col );
	    Coord3 pos = getPos( sid, rc.toInt64() );
	    if ( pos.isDefined() && onlyfillundefs )
		continue;

	    const int rowidx = rowrg.nearestIndex(row);
	    const int colidx = colrg.nearestIndex(col);

	    const double val = rowidx>=0 && rowidx<arrnrrows &&
			       colidx>=0 && colidx<arrnrcols
		   ? arr.get(rowrg.getIndex(row),colrg.getIndex(col))
		   : mUdf(float);

	    if ( pos.z == val || (mIsUdf(pos.z) && mIsUdf(val)) )
		continue;

	    pos.z = val;
	    setPos( sid, rc.toInt64(), pos, false );

	    if ( ++poscount >= 10000 ) 
	    {
		geometry().sectionGeometry( sid )->blockCallBacks( true, true );
		poscount = 0;
	    }
	}
    }

    setBurstAlert(false);
    geometry().sectionGeometry( sid )->blockCallBacks( false, true );
    geometry().enableChecks( didcheck );
    geometry().sectionGeometry( sid )->trimUndefParts();

    if ( oldarr )
    {
	UndoEvent* undo = new  SetAllHor3DPosUndoEvent( this, sid, oldarr );
	EMM().undo().addEvent( undo, undodesc );
    }

    return true;
}


const IOObjContext& Horizon3D::getIOObjContext() const
{ return EMHorizon3DTranslatorGroup::ioContext(); }


Executor* Horizon3D::importer( const ObjectSet<BinIDValueSet>& sections, 
			   const HorSampling& hs )
{
    removeAll();
    return new HorizonImporter( *this, sections, hs );
}


Executor* Horizon3D::auxDataImporter( const ObjectSet<BinIDValueSet>& sections,
				      const BufferStringSet& attribnms,
       				      const int start, const HorSampling& hs )
{
    return new AuxDataImporter( *this, sections, attribnms, start, hs );
}



Horizon3DGeometry::Horizon3DGeometry( Surface& surf )
    : HorizonGeometry( surf ) 
    , step_( SI().inlStep(), SI().crlStep() )
    , loadedstep_( SI().inlStep(), SI().crlStep() )
    , checksupport_( true )
{}


const Geometry::BinIDSurface*
Horizon3DGeometry::sectionGeometry( const SectionID& sid ) const
{
    return (const Geometry::BinIDSurface*) SurfaceGeometry::sectionGeometry(sid);
}


Geometry::BinIDSurface*
Horizon3DGeometry::sectionGeometry( const SectionID& sid )
{
    return (Geometry::BinIDSurface*) SurfaceGeometry::sectionGeometry(sid);
}


RowCol Horizon3DGeometry::loadedStep() const
{
    return loadedstep_;
}


RowCol Horizon3DGeometry::step() const
{
    return step_;
}


void Horizon3DGeometry::setStep( const RowCol& ns, const RowCol& loadedstep )
{
    if ( ns.row && ns.col )
    {
	step_.row = abs( ns.row ); step_.col = abs( ns.col );
    }

    if ( loadedstep.row && loadedstep.col )
    {
	loadedstep_.row = abs( loadedstep.row );
	loadedstep_.col = abs( loadedstep.col );
    }

    if ( nrSections() )
        pErrMsg("Hey, this can only be done without sections.");
}


bool Horizon3DGeometry::isFullResolution() const
{
    return loadedstep_ == step_;
}


bool Horizon3DGeometry::removeSection( const SectionID& sid, bool addtoundo )
{
    int idx=sids_.indexOf(sid);
    if ( idx==-1 ) return false;

    ((Horizon3D&) surface_).edgelinesets.removeSection( sid );
    ((Horizon3D&) surface_).auxdata.removeSection( sid );
    return SurfaceGeometry::removeSection( sid, addtoundo );
}


SectionID Horizon3DGeometry::cloneSection( const SectionID& sid )
{
    const SectionID res = SurfaceGeometry::cloneSection(sid);
    if ( res!=-1 )
	((Horizon3D&) surface_).edgelinesets.cloneEdgeLineSet( sid, res );

    return res;
}


bool Horizon3DGeometry::enableChecks( bool yn )
{
    const bool res = checksupport_;
    for ( int idx=0; idx<sections_.size(); idx++ )
	((Geometry::BinIDSurface*)sections_[idx])->checkSupport(yn);

    checksupport_ = yn;

    return res;
}


bool Horizon3DGeometry::isChecksEnabled() const
{
    return checksupport_;
}


bool Horizon3DGeometry::isNodeOK( const PosID& pid ) const
{
    const Geometry::BinIDSurface* surf = sectionGeometry( pid.sectionID() );
    return surf ? surf->hasSupport( pid.getRowCol() ) : false;
}


bool Horizon3DGeometry::isAtEdge( const PosID& pid ) const
{
    if ( !surface_.isDefined( pid ) )
	return false;

    return !surface_.isDefined(getNeighbor(pid,RowCol(0,1))) ||
	   !surface_.isDefined(getNeighbor(pid,RowCol(1,0))) ||
	   !surface_.isDefined(getNeighbor(pid,RowCol(0,-1))) ||
	   !surface_.isDefined(getNeighbor(pid,RowCol(-1,0)));
}


PosID Horizon3DGeometry::getNeighbor( const PosID& posid,
				      const RowCol& dir ) const
{
    const RowCol rc = posid.getRowCol();
    const SectionID sid = posid.sectionID();

    const StepInterval<int> rowrg = rowRange( sid );
    const StepInterval<int> colrg = colRange( sid, rc.row );

    RowCol diff(0,0);
    if ( dir.row>0 ) diff.row = rowrg.step;
    else if ( dir.row<0 ) diff.row = -rowrg.step;

    if ( dir.col>0 ) diff.col = colrg.step;
    else if ( dir.col<0 ) diff.col = -colrg.step;

    TypeSet<PosID> aliases;
    getLinkedPos( posid, aliases );
    aliases += posid;

    const int nraliases = aliases.size();
    for ( int idx=0; idx<nraliases; idx++ )
    {
	const RowCol ownrc = aliases[idx].getRowCol();
	const RowCol neigborrc = ownrc+diff;
	if ( surface_.isDefined(aliases[idx].sectionID(),
	     neigborrc.toInt64()) )
	{
	    return PosID( surface_.id(), aliases[idx].sectionID(),
			  neigborrc.toInt64() );
	}
    }

    const RowCol ownrc = posid.getRowCol();
    const RowCol neigborrc = ownrc+diff;

    return PosID( surface_.id(), posid.sectionID(), neigborrc.toInt64());
}


int Horizon3DGeometry::getConnectedPos( const PosID& posid,
				        TypeSet<PosID>* res ) const
{
    int rescount = 0;
    const TypeSet<RowCol>& dirs = RowCol::clockWiseSequence();
    for ( int idx=dirs.size()-1; idx>=0; idx-- )
    {
	const PosID neighbor = getNeighbor( posid, dirs[idx] );
	if ( surface_.isDefined( neighbor ) )
	{
	    rescount++;
	    if ( res ) (*res) += neighbor;
	}
    }

    return rescount;
}


Geometry::BinIDSurface* Horizon3DGeometry::createSectionGeometry() const
{
    Geometry::BinIDSurface* res = new Geometry::BinIDSurface( loadedstep_ );
    res->checkSupport( checksupport_ );

    return res;
}


void Horizon3DGeometry::getDataPointSet( const SectionID& sid,
					 DataPointSet& dps, float shift ) const
{
    BinIDValueSet& bidvalset = dps.bivSet();
    bidvalset.setNrVals( 1 );
    const int nrknots = sectionGeometry(sid)->nrKnots();
    for ( int idx=0; idx<nrknots; idx++ )
    {
	const RowCol bid = sectionGeometry( sid )->getKnotRowCol( idx );
	Coord3 coord = sectionGeometry( sid )->getKnot( bid, false );
	bidvalset.add( bid, (float) coord.z + shift );
    }
    dps.dataChanged();
}


bool Horizon3DGeometry::getBoundingPolygon( const SectionID& sid,
					    Pick::Set& set ) const
{
    set.erase();
    const Geometry::BinIDSurface* surf = sectionGeometry( sid );
    if ( !surf ) return false;

    StepInterval<int> rowrg = rowRange( sid );
    StepInterval<int> colrg = colRange( sid, rowrg.start );
    SubID subid; PosID posid;
    bool nodefound = false;
    for ( int row=rowrg.start; row<=rowrg.stop; row+=rowrg.step )
    {
	for ( int col=colrg.start; col<=colrg.stop; col+=colrg.step )
	{
	    subid = RowCol( row, col ).toInt64();
	    posid = PosID( surface_.id(), sid, subid );
	    if ( isNodeOK(posid) && isAtEdge(posid) )
	    {
		nodefound = true;
		break;
	    }
	}

	if ( nodefound ) break;
    }

    if ( !nodefound ) return false;

    const PosID firstposid = posid;
    while ( true )
    {
	Coord3 pos = surf->getKnot( posid.getRowCol(), false );
	set += Pick::Location( pos );

	nodefound = false;
	const TypeSet<RowCol>& dirs = RowCol::clockWiseSequence();
	if ( dirs.size() != 8 ) return false;

	for ( int idx=0; idx<dirs.size(); idx++ )
	{
	    PosID curposid, leftposid, rightposid;
	    curposid = leftposid = rightposid = posid;
	    RowCol rcol = curposid.getRowCol();
	    RowCol currcol = rcol + dirs[idx];
	    int leftidx = idx ? idx - 1 : 7;
	    int rightidx = idx < 7 ? idx + 1 : 0;
	    RowCol leftrcol = rcol + dirs[leftidx];
	    RowCol rightrcol = rcol + dirs[rightidx];
	    curposid.setSubID( currcol.toInt64() );
	    leftposid.setSubID( leftrcol.toInt64() );
	    rightposid.setSubID( rightrcol.toInt64() );
	    if ( !isAtEdge(curposid) )
		continue;

	    if ( !surface_.isDefined(leftposid) || !isNodeOK(leftposid) )
		continue;

	    if ( !surface_.isDefined(rightposid) || !isNodeOK(rightposid) )
	    {
		posid = curposid;
		nodefound = true;
		break;
	    }
	}
	
	if ( posid == firstposid || !nodefound || set.size() > 100000 )
	    break;
    }

    set.disp_.connect_ = Pick::Set::Disp::Close;
    return set.size() ? true : false;
}


void Horizon3DGeometry::fillBinIDValueSet( const SectionID& sid,
					   BinIDValueSet& bivs,
       					   Pos::Provider3D* prov ) const
{
    PtrMan<EMObjectIterator> it = createIterator( sid );
    if ( !it ) return;

    BinID bid;
    while ( true )
    {
	const PosID pid = it->next();
	if ( pid.objectID()==-1 )
	    break;

	const Coord3 crd = surface_.getPos( pid );
	if ( crd.isDefined() )
	{
	    bid.fromInt64( pid.subID() );
	    const bool isinside = prov ? prov->includes( bid ) : true;
	    if ( isinside )
		bivs.add( bid, (float) crd.z );
	}
    }
}


EMObjectIterator* Horizon3DGeometry::createIterator(
			const SectionID& sid, const CubeSampling* cs) const
{
    if ( !cs )
        return new RowColIterator( surface_, sid, cs );

    const StepInterval<int> rowrg = cs->hrg.inlRange();
    const StepInterval<int> colrg = cs->hrg.crlRange();
    return new RowColIterator( surface_, sid, rowrg, colrg );
}


const char* Horizon3DAscIO::sKeyFormatStr()
{ return "Horizon3D"; }

const char* Horizon3DAscIO::sKeyAttribFormatStr()
{ return "Horizon3DAttributes"; }

Table::FormatDesc* Horizon3DAscIO::getDesc() 
{
    Table::FormatDesc* fd = new Table::FormatDesc( "Horizon3D" );
    fd->headerinfos_ += new Table::TargetInfo( "Undefined Value",
	    		StringInpSpec(sKey::FloatUdf()), Table::Required );
    BufferStringSet attrnms;
    createDescBody( fd, attrnms );
    return fd;
}


void Horizon3DAscIO::createDescBody( Table::FormatDesc* fd,
				     const BufferStringSet& attrnms )
{
    fd->bodyinfos_ += Table::TargetInfo::mkHorPosition( true );

    for ( int idx=0; idx<attrnms.size(); idx++ )
    {
	const BufferString fldname = attrnms.get( idx );
	Table::TargetInfo* ti;
	if ( fldname == "Z values" )
	    ti = Table::TargetInfo::mkZPosition( true );
	else
	    ti = new Table::TargetInfo( fldname.buf(), FloatInpSpec(),
		    			Table::Required );
	fd->bodyinfos_ += ti;
    }
}


void Horizon3DAscIO::updateDesc( Table::FormatDesc& fd,
				 const BufferStringSet& attrnms )
{
    fd.bodyinfos_.erase();
    createDescBody( &fd, attrnms );
}


#define mErrRet(s) { if ( s ) errmsg_ = s; return 0; }

bool Horizon3DAscIO::isXY() const
{
    return formOf( false, 0 ) == 0;
}


int Horizon3DAscIO::getNextLine( Coord& pos, TypeSet<float>& data )
{
    data.erase();
    if ( !finishedreadingheader_ )
    {
	if ( !getHdrVals(strm_) )
	    return -1;
	
	udfval_ = getfValue( 0 );
	finishedreadingheader_ = true;
    }

    const int ret = getNextBodyVals( strm_ );
    const int nrattribs = fd_.bodyinfos_.size() - 1;
    if ( ret <= 0 || nrattribs < 1 )
	return ret;

    pos.x = getdValue( 0, udfval_ );
    pos.y = getdValue( 1, udfval_ );
    for ( int idx=0; idx<nrattribs; idx++ )
	data += getfValue( idx+2, udfval_ );

    return ret;
}


} // namespace EM
