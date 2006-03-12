/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        K. Tingdahl
 Date:          Oct 1999
 RCS:           $Id: emhorizon3d.cc,v 1.69 2006-03-12 13:39:10 cvsbert Exp $
________________________________________________________________________

-*/

#include "emhorizon.h"

#include "array2dinterpol.h"
#include "binidsurface.h"
#include "binidvalset.h"
#include "emmanager.h"
#include "emsurfaceauxdata.h"
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
	if ( !nm.size() )
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
	if ( sectionidx >= horizon.geometry.nrSections() )
	    return Finished;

	SectionID sectionid = horizon.geometry.sectionID(sectionidx);
	inlrange = horizon.geometry.rowRange(sectionid);
	crlrange = horizon.geometry.colRange(sectionid);
	inl = inlrange.start;
    }

    PosID posid( horizon.id(), horizon.geometry.sectionID(sectionidx) );
    const BinIDValueSet& bvs = *sections[sectionidx];
    const int nrattribs = bvs.nrVals()-1;
    for ( int crl=crlrange.start; crl<=crlrange.stop; crl+=crlrange.step )
    {
	const BinID bid(inl,crl);
	BinIDValueSet::Pos pos = bvs.findFirst( bid );
	const float* vals = bvs.getVals( pos );
	if ( !vals ) continue;
	RowCol rc( HorizonGeometry::getRowCol(bid) );
	posid.setSubID( rc.getSerialized() );
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
	horizon_.geometry.setStep( step, step );
	horizon_.geometry.addSection( 0, false );
    }

    horizon_.geometry.checkSupport( false );
    sectionidx_ = 0;
}


const char*	message() const		{ return "Transforming grid data"; }
int		totalNr() const		{ return totalnr_; }
int		nrDone() const		{ return nrdone_; }
const char*	nrDoneText() const	{ return "Nr positions imported"; }

int nextStep()
{
    BinIDValueSet& bvs = *sections_[sectionidx_];
    bool haspos = bvs.next( pos_ );
    if ( !haspos )
    {
	if ( bvs.isEmpty() )
	    sectionidx_++;

	if ( sectionidx_ >= sections_.size() )
	{
	    horizon_.geometry.checkSupport( true );
	    return Finished;
	}

	pos_.i = pos_.j = -1;
	return MoreToDo;
    }

    const int nrvals = bvs.nrVals();
    TypeSet<float> vals( nrvals, mUdf(float) );
    BinID bid;
    bvs.get( pos_, bid, vals.arr() );

    PosID posid( horizon_.id(), horizon_.geometry.sectionID(sectionidx_),
		 bid.getSerialized() );
    bool res = horizon_.setPos( posid, Coord3(0,0,vals[0]), false );
    if ( res )
    {
	bvs.remove( pos_ );
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
    : Surface(man,*new HorizonGeometry(*this))
{
    geometry.addSection( "", false );
}


const char* Horizon::typeStr() { return EMHorizonTranslatorGroup::keyword; }

void Horizon::initClass(EMManager& emm)
{
    emm.addFactory( new ObjectFactory( create,
				       EMHorizonTranslatorGroup::ioContext(),
				       typeStr()) );
}


void Horizon::interpolateHoles( int aperture )
{
    for ( int idx=0; idx<geometry.nrSections(); idx++ )
    {
	SectionID sectionid = geometry.sectionID( idx );
	const StepInterval<int> rowrg = geometry.rowRange(sectionid);
	const StepInterval<int> colrg = geometry.colRange(sectionid);
	if ( rowrg.width(false)<1 || colrg.width(false)<1 )
	    continue;

	Array2DImpl<float>* arr =
		new Array2DImpl<float>( rowrg.nrSteps()+1, colrg.nrSteps()+1 );
	if ( !arr ) return;

	for ( int row=rowrg.start; row<=rowrg.stop; row+=rowrg.step )
	{
	    for ( int col=colrg.start; col<=colrg.stop; col+=colrg.step )
	    {
		const Coord3 pos =
		    getPos( sectionid, RowCol(row,col).getSerialized() );
		arr->set( rowrg.getIndex(row), colrg.getIndex(col), pos.z );
	    }
	}

	Array2DInterpolator<float> interpolator( arr );
	interpolator.setInverseDistance( true );
	interpolator.setAperture( aperture );
	while ( true )
	{
	    const int res = interpolator.nextStep();
	    if ( !res ) break;
	    else if ( res == -1 ) return;
	}

	for ( int row=rowrg.start; row<=rowrg.stop; row+=rowrg.step )
	{
	    for ( int col=colrg.start; col<=colrg.stop; col+=colrg.step )
	    {
		const RowCol rc( row, col );
		Coord3 pos = getPos( sectionid, rc.getSerialized() );
		if ( !pos.isDefined() )
		{
		    pos.z = arr->get( rowrg.getIndex(row), colrg.getIndex(col));
		    setPos( sectionid, rc.getSerialized(), pos, true );
		}
	    }
	}

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
    cleanUp();
    return new HorizonImporter( *this, sections, step );
}


Executor* Horizon::auxDataImporter( const ObjectSet<BinIDValueSet>& sections,
				    const BufferStringSet& attribnms,
				    const BoolTypeSet& attribsel )
{
    return new AuxDataImporter( *this, sections, attribnms, attribsel );
}



HorizonGeometry::HorizonGeometry( Surface& surf )
    : SurfaceGeometry( surf ) 
{}


bool HorizonGeometry::createFromStick( const TypeSet<Coord3>& stick,
				       SectionID sid, float velocity )
{
/*
    SectionID sectionid = sid;
    if ( !nrSections() || !hasSection(sid) ) 
	sectionid = addSection( "", true );

    const float idealdistance = 25; // TODO set this in some intelligent way

    for ( int idx=0; idx<stick.size()-1; idx++ )
    {
	const Coord3 startpos( stick[idx], stick[idx].z*velocity/2 );
	const Coord3 stoppos( stick[idx+1], stick[idx+1].z*velocity/2 );
	if ( !startpos.isDefined() || !stoppos.isDefined() )
	    break;

	const BinID startbid = SI().transform( startpos );
	const BinID stopbid = SI().transform( stoppos );

	const RowCol startrc = getRowCol(startbid);
	const RowCol stoprc = getRowCol(stopbid);
	const Line3 line( Coord3(startrc.row,startrc.col,0),
	     Coord3(stoprc.row-startrc.row,stoprc.col-startrc.col,0) );

	RowCol iterstep( step_ );
	if ( startrc.row==stoprc.row ) iterstep.row = 0;
	else if ( startrc.row>stoprc.row ) iterstep.row = -iterstep.row;

	if ( startrc.col==stoprc.col ) iterstep.col = 0;
	else if ( startrc.col>stoprc.col ) iterstep.col = -iterstep.col;

	RowCol rowcol = startrc;
	while ( rowcol != stoprc )
	{
	    const float rowdistbefore = rowcol.row-startrc.row;
	    const float coldistbefore = rowcol.col-startrc.col;
	    const float distbefore = sqrt(rowdistbefore*rowdistbefore+
					  coldistbefore*coldistbefore );
	    const float rowdistafter = rowcol.row-stoprc.row;
	    const float coldistafter = rowcol.col-stoprc.col;
	    const float distafter = sqrt(rowdistafter*rowdistafter+
					  coldistafter*coldistafter );

	    const Coord3 newpos(SI().transform(getBinID(rowcol)),
		    (startpos.z*distafter+stoppos.z*distbefore)/
		    (distbefore+distafter)/(velocity/2));

	    const PosID posid( surface.id(), sectionid,
				   rowcol.getSerialized() );
	    setPos( posid, newpos, true );
	    if ( rowcol == startrc )
		surface.setPosAttrib( posid, EMObject::sPermanentControlNode,
				      true);

	    float distance = mUdf(float);
	    RowCol nextstep;
	    if ( iterstep.row )
	    {
		distance = line.distanceToPoint(
			Coord3(rowcol.row+iterstep.row, rowcol.col,0) );
		nextstep = rowcol;
		nextstep.row += iterstep.row;
	    }

	    if ( iterstep.col )
	    {
		float nd = line.distanceToPoint(
			    Coord3(rowcol.row, rowcol.col+iterstep.col,0) );
		if ( nd<distance )
		{
		    nextstep = rowcol;
		    nextstep.col += iterstep.col;	
		}
	    }

	    rowcol = nextstep;
	}

	if ( idx==stick.size()-2 )
	{
	    const PosID posid( surface.id(), sectionid,
				    rowcol.getSerialized() );
	    setPos( posid, Coord3( stoppos, stoppos.z/(velocity/2)), true);

	    surface.setPosAttrib( posid, EMObject::sPermanentControlNode, true);
	}
    }
*/

    return true;
}


BinID HorizonGeometry::getBinID( const SubID& subid )
{
    return getBinID( RowCol(subid) );
}


BinID HorizonGeometry::getBinID( const RowCol& rc )
{
    return BinID(rc.row, rc.col);
}


RowCol HorizonGeometry::getRowCol( const BinID& bid )
{
    return RowCol( bid.inl, bid.crl );
}


SubID HorizonGeometry::getSubID( const BinID& bid )
{
    return bid.getSerialized();
}


Geometry::ParametricSurface* HorizonGeometry::createSectionSurface() const
{
    return new Geometry::BinIDSurface( loadedstep_ );
}

/*
void HorizonGeometry::setTransform( const SectionID& sectionid ) const
{
    const int sectionidx = sectionNr( sectionid );
    if ( sectionidx < 0 || sectionidx >= meshsurfaces.size() )
	return;
    mDynamicCastGet(Geometry::GridSurface*,gridsurf,meshsurfaces[sectionidx])
    if ( !gridsurf ) return;

    const RowCol rc00( 0, 0 );
    const RowCol rc10( 1, 0 );
    const RowCol rc11( 1, 1 );

    const RowCol surfrc00( getSurfSubID(rc00,sectionid) );
    const RowCol surfrc10( getSurfSubID(rc10,sectionid) );
    const RowCol surfrc11( getSurfSubID(rc11,sectionid) );

    const Coord pos00 = SI().transform(BinID(surfrc00.row,surfrc00.col));
    const Coord pos10 = SI().transform(BinID(surfrc10.row,surfrc10.col));
    const Coord pos11 = SI().transform(BinID(surfrc11.row,surfrc11.col));
    
    gridsurf->setTransform(  pos00.x, pos00.y, rc00.row, rc00.col,
			     pos10.x, pos10.y, rc10.row, rc10.col,
			     pos11.x, pos11.y, rc11.row, rc11.col );
}

*/


}; //namespace


