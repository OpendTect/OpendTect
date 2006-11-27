/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        K. Tingdahl
 Date:          Oct 1999
 RCS:           $Id: emhorizon3d.cc,v 1.84 2006-11-27 14:57:39 cvsnanne Exp $
________________________________________________________________________

-*/

#include "emhorizon.h"

#include "array2dinterpol.h"
#include "arrayndimpl.h"
#include "binidsurface.h"
#include "binidvalset.h"
#include "emmanager.h"
#include "emrowcoliterator.h"
#include "emsurfaceauxdata.h"
#include "emsurfaceedgeline.h"
#include "emsurfacetr.h"
#include "executor.h"
#include "ioobj.h"
#include "ptrman.h"
#include "survinfo.h"
#include "trigonometry.h"

#include <math.h>

namespace EM {

class AuxDataImporter : public Executor
{
public:

AuxDataImporter( Horizon& hor, const ObjectSet<BinIDValueSet>& sects,
		 const BufferStringSet& attribnames,
		 const BoolTypeSet& attrsel )
    : Executor("Data Import")
    , horizon(hor)
    , sections(sects)
    , attribsel(attrsel)
    , totalnr(0)
    , nrdone(0)
{
    for ( int idx=0; idx<sections.size(); idx++ )
    {
	const BinIDValueSet& bvs = *sections[idx];
	totalnr += bvs.nrInls();
    }

    const int nrattribs = sections[0]->nrVals()-1;
    for ( int attribidx=0; attribidx<nrattribs; attribidx++ )
    {
	if ( !attribsel[attribidx] ) continue;

	BufferString nm = attribnames.get( attribidx );
	if ( nm.isEmpty() )
	    { nm = "Imported attribute "; nm += attribidx; }

	horizon.auxdata.addAuxData( nm );
    }

    sectionidx = -1;
}


int nextStep()
{
    if ( sectionidx == -1 || inl > inlrange.stop )
    {
	sectionidx++;
	if ( sectionidx >= horizon.geometry().nrSections() )
	    return Finished;

	SectionID sectionid = horizon.geometry().sectionID(sectionidx);
	inlrange = horizon.geometry().rowRange(sectionid);
	crlrange = horizon.geometry().colRange(sectionid);
	inl = inlrange.start;
    }

    PosID posid( horizon.id(), horizon.geometry().sectionID(sectionidx) );
    const BinIDValueSet& bvs = *sections[sectionidx];
    const int nrattribs = bvs.nrVals()-1;
    for ( int crl=crlrange.start; crl<=crlrange.stop; crl+=crlrange.step )
    {
	const BinID bid(inl,crl);
	BinIDValueSet::Pos pos = bvs.findFirst( bid );
	const float* vals = bvs.getVals( pos );
	if ( !vals ) continue;
	posid.setSubID( bid.getSerialized() );
	int index = 0;
	for ( int attridx=0; attridx<nrattribs; attridx++ )
	{
	    if ( !attribsel[attridx] ) continue;
	    horizon.auxdata.setAuxDataVal( index++, posid, vals[attridx+1] );
	}
    }

    inl += inlrange.step;
    nrdone++;
    return MoreToDo;
}


const char*	message() const		{ return "Importing aux data"; }
int		totalNr() const		{ return totalnr; }
int		nrDone() const		{ return nrdone; }
const char*	nrDoneText() const	{ return "Nr inlines imported"; }

protected:

    const BoolTypeSet&		attribsel;
    const ObjectSet<BinIDValueSet>&	sections;
    Horizon&			horizon;
    StepInterval<int>		inlrange;
    StepInterval<int>		crlrange;

    int				inl;
    int				sectionidx;

    int				totalnr;
    int				nrdone;
};


class HorizonImporter : public Executor
{
public:

HorizonImporter( Horizon& hor, const ObjectSet<BinIDValueSet>& sects, 
		 const RowCol& step )
    : Executor("Horizon Import")
    , horizon_(hor)
    , sections_(sects)
    , totalnr_(0)
    , nrdone_(0)

{
    for ( int idx=0; idx<sections_.size(); idx++ )
    {
	const BinIDValueSet& bvs = *sections_[idx];
	totalnr_ += bvs.totalSize();
	horizon_.geometry().setStep( step, step );
	horizon_.geometry().addSection( 0, false );
    }

    horizon_.enableGeometryChecks(false);
    sectionidx_ = 0;
}


const char*	message() const		{ return "Transforming grid data"; }
int		totalNr() const		{ return totalnr_; }
int		nrDone() const		{ return nrdone_; }
const char*	nrDoneText() const	{ return "Nr positions imported"; }

int nextStep()
{
    const BinIDValueSet& bvs = *sections_[sectionidx_];
    bool haspos = bvs.next( pos_ );
    if ( !haspos )
    {
	if ( bvs.isEmpty() )
	    sectionidx_++;

	if ( sectionidx_ >= sections_.size() )
	{
	    horizon_.enableGeometryChecks( true );
	    return Finished;
	}

	pos_.i = pos_.j = -1;
	return MoreToDo;
    }

    const int nrvals = bvs.nrVals();
    TypeSet<float> vals( nrvals, mUdf(float) );
    BinID bid;
    bvs.get( pos_, bid, vals.arr() );

    PosID posid( horizon_.id(), horizon_.sectionID(sectionidx_),
		 bid.getSerialized() );
    bool res = horizon_.setPos( posid, Coord3(0,0,vals[0]), false );
    if ( res )
    {
	const_cast<BinIDValueSet&>(bvs).remove( pos_ );
	pos_.i = pos_.j = -1;
	nrdone_++;
    }

    return MoreToDo;
}

protected:

    const ObjectSet<BinIDValueSet>&	sections_;
    Horizon&		horizon_;
    BinIDValueSet::Pos	pos_;

    int			sectionidx_;
    int			totalnr_;
    int			nrdone_;
};


Horizon::Horizon( EMManager& man )
    : Surface(man)
    , geometry_( *this )
    , auxdata( *new SurfaceAuxData(*this) )
    , edgelinesets( *new EdgeLineManager(*this) )
{
    geometry_.addSection( "", false );
}


Horizon::~Horizon()
{
    delete &auxdata;
    delete &edgelinesets;
}


void Horizon::removeAll()
{
    Surface::removeAll();
    geometry_.removeAll();
    auxdata.removeAll();
    edgelinesets.removeAll();
}


void Horizon::fillPar( IOPar& par ) const
{
    Surface::fillPar( par );
    auxdata.fillPar( par );
    edgelinesets.fillPar( par );
}


bool Horizon::usePar( const IOPar& par )
{
    return Surface::usePar( par ) &&
	auxdata.usePar( par ) &&
	edgelinesets.usePar( par );
}


HorizonGeometry& Horizon::geometry()
{ return geometry_; }


const HorizonGeometry& Horizon::geometry() const
{ return geometry_; }


const char* Horizon::typeStr() { return EMHorizonTranslatorGroup::keyword; }

void Horizon::initClass( EMManager& emm )
{
    emm.addFactory( new ObjectFactory( create,
				       EMHorizonTranslatorGroup::ioContext(),
				       typeStr()) );
}


Array2D<float>* Horizon::createArray2D( SectionID sid )
{
    const StepInterval<int> rowrg = geometry_.rowRange( sid );
    const StepInterval<int> colrg = geometry_.colRange( sid );
    if ( rowrg.width(false)<1 || colrg.width(false)<1 )
	return 0;

    Array2DImpl<float>* arr =
	    new Array2DImpl<float>( rowrg.nrSteps()+1, colrg.nrSteps()+1 );
    for ( int row=rowrg.start; row<=rowrg.stop; row+=rowrg.step )
    {
	for ( int col=colrg.start; col<=colrg.stop; col+=colrg.step )
	{
	    const Coord3 pos = getPos( sid, RowCol(row,col).getSerialized() );
	    arr->set( rowrg.getIndex(row), colrg.getIndex(col), pos.z );
	}
    }

    return arr;
}


bool Horizon::setArray2D( const Array2D<float>& arr, SectionID sid, 
			  bool onlyfillundefs )
{
    const StepInterval<int> rowrg = geometry_.rowRange( sid );
    const StepInterval<int> colrg = geometry_.colRange( sid );
    if ( rowrg.width(false)<1 || colrg.width(false)<1 )
	return false;

    geometry().sectionGeometry( sid )->blockCallBacks( true, false );
    const bool didcheck = geometry().enableChecks( false );
    for ( int row=rowrg.start; row<=rowrg.stop; row+=rowrg.step )
    {
	for ( int col=colrg.start; col<=colrg.stop; col+=colrg.step )
	{
	    const RowCol rc( row, col );
	    Coord3 pos = getPos( sid, rc.getSerialized() );
	    if ( pos.isDefined() && onlyfillundefs )
		continue;

	    pos.z = arr.get( rowrg.getIndex(row), colrg.getIndex(col));
	    setPos( sid, rc.getSerialized(), pos, false );
	}
    }

    geometry().sectionGeometry( sid )->blockCallBacks( false, true );
    geometry().enableChecks( didcheck );
    return true;
}


void Horizon::interpolateHoles( int aperture, bool extrapolate )
{
    for ( int idx=0; idx<geometry_.nrSections(); idx++ )
    {
	SectionID sid = geometry_.sectionID( idx );
	const StepInterval<int> rowrg = geometry_.rowRange( sid );
	const StepInterval<int> colrg = geometry_.colRange( sid );

	Array2D<float>* arr = createArray2D( sid );
	Array2DInterpolator<float> interpolator( *arr );
	interpolator.pars().maxholesize_ = aperture;
	interpolator.pars().extrapolate_ = extrapolate;
	interpolator.setStepRatio( rowrg.step, colrg.step );
	if ( !interpolator.execute() )
	{
	    delete arr;
	    return;
	}

	setArray2D( *arr, sid, true );
	delete arr;
    }
}


EMObject* Horizon::create( EMManager& emm )
{ return new Horizon( emm ); }


const IOObjContext& Horizon::getIOObjContext() const
{ return EMHorizonTranslatorGroup::ioContext(); }


Executor* Horizon::importer( const ObjectSet<BinIDValueSet>& sections, 
			     const RowCol& step )
{
    removeAll();
    return new HorizonImporter( *this, sections, step );
}


Executor* Horizon::auxDataImporter( const ObjectSet<BinIDValueSet>& sections,
				    const BufferStringSet& attribnms,
				    const BoolTypeSet& attribsel )
{
    return new AuxDataImporter( *this, sections, attribnms, attribsel );
}



HorizonGeometry::HorizonGeometry( Surface& surf )
    : RowColSurfaceGeometry( surf ) 
    , step_( SI().inlStep(), SI().crlStep() )
    , loadedstep_( SI().inlStep(), SI().crlStep() )
    , shift_( 0 )
    , checksupport_( true )
{}


const Geometry::BinIDSurface*
HorizonGeometry::sectionGeometry( const SectionID& sid ) const
{
    return (const Geometry::BinIDSurface*) SurfaceGeometry::sectionGeometry(sid);
}


Geometry::BinIDSurface*
HorizonGeometry::sectionGeometry( const SectionID& sid )
{
    return (Geometry::BinIDSurface*) SurfaceGeometry::sectionGeometry(sid);
}


RowCol HorizonGeometry::loadedStep() const
{
    return loadedstep_;
}


RowCol HorizonGeometry::step() const
{
    return step_;
}


void HorizonGeometry::setStep( const RowCol& ns, const RowCol& loadedstep )
{
    step_ = ns;
    loadedstep_ = loadedstep;
    if ( nrSections() )
        pErrMsg("Hey, this can only be done without sections.");
}


bool HorizonGeometry::isFullResolution() const
{
    return loadedstep_ == step_;
}


bool HorizonGeometry::removeSection( const SectionID& sid, bool addtohistory )
{
    int idx=sids_.indexOf(sid);
    if ( idx==-1 ) return false;

    ((Horizon&) surface_).edgelinesets.removeSection( sid );
    ((Horizon&) surface_).auxdata.removeSection( sid );
    return SurfaceGeometry::removeSection( sid, addtohistory );
}


SectionID HorizonGeometry::cloneSection( const SectionID& sid )
{
    const SectionID res = SurfaceGeometry::cloneSection(sid);
    if ( res!=-1 )
	((Horizon&) surface_).edgelinesets.cloneEdgeLineSet( sid, res );

    return res;
}


void HorizonGeometry::setShift( float ns )
{ shift_ = ns; }


float HorizonGeometry::getShift() const
{ return shift_; }


bool HorizonGeometry::enableChecks( bool yn )
{
    const bool res = checksupport_;
    for ( int idx=0; idx<sections_.size(); idx++ )
	((Geometry::BinIDSurface*)sections_[idx])->checkSupport(yn);

    checksupport_ = yn;

    return res;
}


bool HorizonGeometry::isChecksEnabled() const
{
    return checksupport_;
}


bool HorizonGeometry::isNodeOK( const PosID& pid ) const
{
    const Geometry::BinIDSurface* surf = sectionGeometry( pid.sectionID() );
    return surf ? surf->hasSupport( RowCol(pid.subID()) ) : false;
}


bool HorizonGeometry::isAtEdge( const PosID& pid ) const
{
    if ( !surface_.isDefined( pid ) )
	return false;

    return !surface_.isDefined(getNeighbor(pid,RowCol(0,1))) ||
	   !surface_.isDefined(getNeighbor(pid,RowCol(1,0))) ||
	   !surface_.isDefined(getNeighbor(pid,RowCol(0,-1))) ||
	   !surface_.isDefined(getNeighbor(pid,RowCol(-1,0)));
}


PosID HorizonGeometry::getNeighbor( const PosID& posid,
				    const RowCol& dir ) const
{
    const RowCol rc( posid.subID() );
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
	const RowCol ownrc(aliases[idx].subID());
	const RowCol neigborrc = ownrc+diff;
	if ( surface_.isDefined(aliases[idx].sectionID(),
	     neigborrc.getSerialized()) )
	{
	    return PosID( surface_.id(), aliases[idx].sectionID(),
			  neigborrc.getSerialized() );
	}
    }

    const RowCol ownrc(posid.subID());
    const RowCol neigborrc = ownrc+diff;

    return PosID( surface_.id(), posid.sectionID(), neigborrc.getSerialized());
}


int HorizonGeometry::getConnectedPos( const PosID& posid,
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


Geometry::BinIDSurface* HorizonGeometry::createSectionGeometry() const
{
    Geometry::BinIDSurface* res = new Geometry::BinIDSurface( loadedstep_ );
    res->checkSupport( checksupport_ );

    return res;
}


void HorizonGeometry::fillBinIDValueSet( const SectionID& sid,
					 BinIDValueSet& bivs ) const
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
	    bid.setSerialized( pid.subID() );
	    bivs.add( bid, crd.z );
	}
    }
}


EMObjectIterator* HorizonGeometry::createIterator( const SectionID& sid, 
						   const CubeSampling* cs) const
{
    if ( !cs )
        return new RowColIterator( surface_, sid, cs );

    const StepInterval<int> rowrg = cs->hrg.inlRange();
    const StepInterval<int> colrg = cs->hrg.crlRange();
    return new RowColIterator( surface_, sid, rowrg, colrg );
}


} // namespace EM
