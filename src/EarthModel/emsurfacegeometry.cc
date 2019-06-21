/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        K. Tingdahl
 Date:          Nov 2002
________________________________________________________________________

-*/

#include "emsurfacegeometry.h"

#include "emrowcoliterator.h"

#include "emsurface.h"
#include "emsurfacetr.h"
#include "emsurfaceauxdata.h"

#include "undo.h"
#include "emmanager.h"
#include "parametricsurface.h"
#include "mathfunc.h"
#include "ioobj.h"
#include "iopar.h"
#include "pca.h"
#include "toplist.h"
#include "survinfo.h"
#include "uistrings.h"

namespace EM {


// ***** SurfaceGeometry *****

SurfaceGeometry::SurfaceGeometry( Surface& surf )
    : changed_( false )
    , surface_( surf )
{
    surface_.objectChanged().notify( mCB(this,SurfaceGeometry,geomChangeCB) );
}


SurfaceGeometry::~SurfaceGeometry()
{
    surface_.objectChanged().remove( mCB(this,SurfaceGeometry,geomChangeCB) );
    removeAll();
}


SurfaceGeometry& SurfaceGeometry::operator=( const SurfaceGeometry& oth )
{
    deepCopyClone( sections_, oth.sections_ );
    return *this;
}


void SurfaceGeometry::removeAll()
{
}


void SurfaceGeometry::geomChangeCB( CallBacker* cb )
{
    mCBCapsuleUnpack(ObjectCallbackData,cbdata,cb);
    if ( cbdata.changeType() == Object::cPositionChange() )
       changed_ = true;
}


bool SurfaceGeometry::enableChecks( bool yn )		{ return false; }
bool SurfaceGeometry::isChecksEnabled() const		{ return false; }
bool SurfaceGeometry::isNodeOK( const PosID& ) const	{ return true; }


const Geometry::Element* SurfaceGeometry::geometryElement() const
{
    if ( sections_.isEmpty() )
    {
	Geometry::Element* geomelem = createGeometryElement();
	const_cast<SurfaceGeometry*> (this)->sections_ += geomelem;
    }

    return sections_[0];
}


Geometry::Element* SurfaceGeometry::geometryElement()
{
    if ( sections_.isEmpty() )
    {
	Geometry::Element* geomelem = createGeometryElement();
	sections_ += geomelem;
    }

    return sections_[0];
}


bool SurfaceGeometry::isAtEdge(EM::PosID const&) const
{ return false; }


int SurfaceGeometry::getConnectedPos( const PosID& posid,
				      TypeSet<PosID>* res) const
{
    return 0;
}


int SurfaceGeometry::findPos( const Interval<float>& x,
			      const Interval<float>& y,
			      const Interval<float>& z,
			      TypeSet<PosID>* res ) const
{
    return 0;
}


int SurfaceGeometry::findPos( const TrcKeyZSampling& cs,
			  TypeSet<PosID>* res ) const
{
    Coord xypos = SI().transform(cs.hsamp_.start_);
    Interval<float> xinterval( (float) xypos.x_, (float) xypos.x_ );
    Interval<float> yinterval( (float) xypos.y_, (float) xypos.y_ );

    xypos = SI().transform(cs.hsamp_.stop_);
    xinterval.include( (float) xypos.x_ );
    yinterval.include( (float) xypos.y_ );

    xypos = SI().transform(
	BinID(cs.hsamp_.start_.inl(),cs.hsamp_.stop_.crl()) );
    xinterval.include( (float) xypos.x_ );
    yinterval.include( (float) xypos.y_ );

    xypos = SI().transform(
	BinID(cs.hsamp_.stop_.inl(),cs.hsamp_.start_.crl()) );
    xinterval.include( (float) xypos.x_ );
    yinterval.include( (float) xypos.y_ );

    TypeSet<PosID> posids;
    findPos( xinterval, yinterval, cs.zsamp_, &posids );

    for ( int idx=0; idx<posids.size(); idx++ )
    {
	const PosID& posid = posids[idx];
	const BinID nodebid = SI().transform(surface_.getPos(posid).getXY());

	if ( nodebid.inl()<cs.hsamp_.start_.inl() ||
	     nodebid.inl()>cs.hsamp_.stop_.inl() ||
	     nodebid.crl()<cs.hsamp_.start_.crl() ||
	     nodebid.crl()>cs.hsamp_.stop_.crl() )
	{
	    posids.removeSingle( idx--, false );
	    continue;
	}
    }

    if ( res ) res->append(posids);
    return posids.size();
}


void SurfaceGeometry::getLinkedPos( const PosID& posid,
				TypeSet<PosID>& res ) const
{ res.erase(); }


bool SurfaceGeometry::isLoaded() const
{
    return !sections_.isEmpty() && sections_[0];
}


bool SurfaceGeometry::isFullResolution() const
{
    return true;
}


Executor* SurfaceGeometry::loader( const SurfaceIODataSelection* newsel )
{
    PtrMan<IOObj> ioobj = surface_.dbKey().getIOObj();
    if ( !ioobj )
	{ surface_.errmsg_ = uiStrings::phrCannotFindObjInDB(); return 0; }

    PtrMan<EMSurfaceTranslator> trans =
			(EMSurfaceTranslator*)ioobj->createTranslator();
    if ( !trans || !trans->startRead(*ioobj) )
	{ surface_.errmsg_ = trans ? trans->errMsg() :
	    tr("Cannot find Translator"); return 0; }

    SurfaceIODataSelection& sel = trans->selections();
    if ( newsel && !sel.rg.isEmpty() )
    {
	sel.sellinenames = newsel->sellinenames;
	sel.seltrcranges = newsel->seltrcranges;

	sel.rg.start_.inl() = sel.rg.inlRange().limitValue(
		sel.rg.inlRange().snap( newsel->rg.inlRange().start ) );
	sel.rg.start_.crl() = sel.rg.crlRange().limitValue(
		sel.rg.crlRange().snap( newsel->rg.crlRange().start ) );
	sel.rg.stop_.inl() = sel.rg.inlRange().limitValue(
		sel.rg.inlRange().snap( newsel->rg.inlRange().stop ) );
	sel.rg.stop_.crl() = sel.rg.crlRange().limitValue(
		sel.rg.crlRange().snap( newsel->rg.crlRange().stop ) );
	int stepfactorinl = mNINT32(((float)newsel->rg.step_.inl()
			  / sel.rg.step_.inl()));
	if ( stepfactorinl<1 ) stepfactorinl = 1;
	sel.rg.step_.inl() *= stepfactorinl;
	int stepfactorcrl = mNINT32(((float)newsel->rg.step_.crl()
			  / sel.rg.step_.crl()));
	if ( stepfactorcrl<1 ) stepfactorcrl = 1;
	sel.rg.step_.crl() *= stepfactorcrl;

	sel.selvalues = newsel->selvalues;
    }
    else
	sel.selvalues.erase();

    Executor* exec = trans->reader( surface_ );
    surface_.errmsg_ = trans->errMsg();
    return exec;
}


Executor* SurfaceGeometry::saver( const SurfaceIODataSelection* newsel,
			          const DBKey* key )
{
    const DBKey& dbky = key && key->isValid() ? *key : surface_.dbKey();
    PtrMan<IOObj> ioobj = dbky.getIOObj();
    if ( !ioobj )
	{ surface_.errmsg_ = uiStrings::phrCannotFindObjInDB(); return 0; }

    PtrMan<EMSurfaceTranslator> trans =
			(EMSurfaceTranslator*)ioobj->createTranslator();
    if ( !trans )
	{ surface_.errmsg_ = toUiString("Internal: No Translator"); return 0; }
    if ( !trans->startWrite(surface_) )
	{ surface_.errmsg_ = trans->errMsg(); return 0; }


    SurfaceIODataSelection& sel = trans->selections();
    if ( newsel )
    {
	sel.rg = newsel->rg;
	sel.selvalues = newsel->selvalues;
    }

    Executor* exec = trans->writer( *ioobj, changed_ );
    surface_.errmsg_ = trans->errMsg();
    return exec;
}


bool SurfaceGeometry::usePar( const IOPar& par )
{ return true; }


void SurfaceGeometry::fillPar( IOPar& par ) const
{ }


ObjectIterator* SurfaceGeometry::createIterator( const TrcKeyZSampling*) const
{ return 0; }


RowColSurfaceGeometry::RowColSurfaceGeometry( Surface& s )
    : SurfaceGeometry( s )
{}


RowColSurfaceGeometry::~RowColSurfaceGeometry()
{}


Geometry::RowColSurface*
RowColSurfaceGeometry::geometryElement()
{
    Geometry::Element* res = SurfaceGeometry::geometryElement();
    return reinterpret_cast<Geometry::RowColSurface*>( res );
}

const Geometry::RowColSurface* RowColSurfaceGeometry::geometryElement() const
{
    return const_cast<RowColSurfaceGeometry*>(this)->geometryElement();
}


StepInterval<int> RowColSurfaceGeometry::rowRange() const
{
    const Geometry::RowColSurface* elem = geometryElement();
    return elem ? elem->rowRange() : StepInterval<int>::udf();
}


StepInterval<int> RowColSurfaceGeometry::colRange( int row ) const
{
    const Geometry::RowColSurface* elem = geometryElement();
    if ( !elem )
	return StepInterval<int>::udf();

    return row<0 ? elem->colRange() : elem->colRange( row );
}


StepInterval<int> RowColSurfaceGeometry::colRange() const
{
    return colRange( -1 );
}


ObjectIterator* RowColSurfaceGeometry::createIterator(
					const TrcKeyZSampling* cs ) const
{
    return new RowColIterator( surface_, cs );
}

}; //namespace
