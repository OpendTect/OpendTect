/*+
* (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
* AUTHOR   : Salil Agarwal
* DATE     : Dec 2012
-*/


#include "survgeometryio.h"

#include "bendpointfinder.h"
#include "ioobjctxt.h"
#include "dbdir.h"
#include "survgeom2d.h"
#include "posinfo2d.h"
#include "survgeometrytransl.h"
#include "survinfo.h"
#include "executor.h"
#include "keystrs.h"
#include "od_iostream.h"

namespace Survey
{

#define mReturn return ++nrdone_ < totalNr() ? MoreToDo() : Finished();

class OD2DGeometryFileReader : public Executor
{ mODTextTranslationClass(Survey::OD2DGeometryFileReader);
public:

OD2DGeometryFileReader( ObjectSet<Geometry2D>& geometries, bool updateonly )
    : Executor( "Loading Files" )
    , dbdir_(new DBDir(IOObjContext::Geom))
    , geometries_(geometries)
    , nrdone_(0)
    , updateonly_(updateonly)
{
}

uiString message() const override    { return tr("Reading line geometries"); }
uiString nrDoneText() const override { return tr("Geometries read"); }
od_int64 nrDone() const override     { return nrdone_; }
od_int64 totalNr() const override    { return dbdir_->size(); }

int nextStep() override
{
    if ( nrdone_ >= dbdir_->size() )
	return Finished();

    PtrMan<IOObj> ioobj = dbdir_->getEntryByIdx( mCast(int,nrdone_) );
    const GeomID geomid = SurvGeom2DTranslator::getGeomID( *ioobj );
    bool doupdate = false;
    const int geomidx = indexOf( geomid );
    if ( updateonly_ && geomidx!=-1 )
    {
	auto* geom2d = geometries_[geomidx];
	if ( geom2d && geom2d->data().isEmpty() )
	    doupdate = true;
	else
	    mReturn
    }

    PtrMan<Translator> transl = ioobj->createTranslator();
    mDynamicCastGet(SurvGeom2DTranslator*,geomtransl,transl.ptr());
    if ( !geomtransl )
	mReturn

    uiString errmsg;
    Geometry2D* geom = geomtransl->readGeometry( *ioobj, errmsg );
    if ( geom )
    {
	geom->ref();
	if ( doupdate )
	{
	    Geometry* prevgeom = geometries_.replace( geomidx, geom );
	    if ( prevgeom )
		prevgeom->unRef();
	}
	else
	    geometries_ += geom;
	calcBendPoints( geom->data() );
    }


    mReturn
}

protected:

bool calcBendPoints( PosInfo::Line2DData& l2d ) const
{
    BendPointFinder2DGeom bpf( l2d.positions(), 10.0 );
    if ( !bpf.execute() || bpf.bendPoints().isEmpty() )
	return false;

    l2d.setBendPoints( bpf.bendPoints() );
    return true;
}

int indexOf( GeomID geomid ) const
{
    for ( int idx=0; idx<geometries_.size(); idx++ )
	if ( geometries_[idx]->geomID() == geomid )
	    return idx;
    return -1;
}

    RefMan<DBDir>		dbdir_;
    ObjectSet<Geometry2D>&	geometries_;
    od_int64			nrdone_;
    bool			updateonly_;

};


bool ODGeometry2DWriter::write( const Geometry2D& geom, uiString& errmsg,
			      const char* createfromstr ) const
{
    const auto* geom2d = geom.as2D();
    if ( !geom2d )
	return true;

    const BufferString linenm( geom2d->data().lineName() );
    if ( linenm.isEmpty() )
	{ errmsg = uiStrings::phrEnterValidName(); return false; }
    bool needcommit = getGeomIDFor( linenm ).isValid();

    PtrMan<IOObj> ioobj = getEntry( geom2d->data().lineName() );
    if ( !ioobj || !ioobj->key().hasValidObjID() )
    {
	errmsg = uiStrings::phrCannotWrite( toUiString("%1 [%2]")
			.arg(uiStrings::sGeometry()).arg(linenm) );
	return false;
    }

    PtrMan<Translator> transl = ioobj->createTranslator();
    mDynamicCastGet(SurvGeom2DTranslator*,geomtransl,transl.ptr());
    if ( !geomtransl )
	{ errmsg = mINTERNAL("Translator is not available"); return false; }

    if ( !FixedString(createfromstr).isEmpty() )
    {
	ioobj->pars().set( sKey::CrFrom(), createfromstr );
	needcommit = true;
    }

    const bool isok = geomtransl->writeGeometry( *ioobj, geom, errmsg );
    if ( isok && needcommit )
	ioobj->commitChanges();

    return isok;
}


GeomID ODGeometry2DWriter::getGeomIDFor( const char* name ) const
{
    PtrMan<IOObj> geomobj = getEntry( name );
    if ( !geomobj )
	return GeomID();
    return SurvGeom2DTranslator::getGeomID( *geomobj );
}


IOObj* ODGeometry2DWriter::getEntry( const char* name ) const
{
    return SurvGeom2DTranslator::getEntry( name,
					dgbSurvGeom2DTranslator::translKey() );
}


bool ODGeometry2DReader::read( ObjectSet<Geometry2D>& geometries,
			     const TaskRunnerProvider& trprov ) const
{
    OD2DGeometryFileReader rdr( geometries, false );
    return trprov.execute( rdr );
}


bool ODGeometry2DReader::updateGeometries( ObjectSet<Geometry2D>& geometries,
				 const TaskRunnerProvider& trprov ) const
{
    OD2DGeometryFileReader rdr( geometries, true );
    return trprov.execute( rdr );
}


void GeometryIO_init2DGeometry()
{
    Survey::Geometry2DReader::factory().addCreator(
	    Survey::ODGeometry2DReader::createNew, sKey::TwoD() );
    Survey::Geometry2DWriter::factory().addCreator(
	    Survey::ODGeometry2DWriter::createNew, sKey::TwoD() );
    SurvGeom2DTranslatorGroup::initClass();
    dgbSurvGeom2DTranslator::initClass();
}



} // namespace Survey
