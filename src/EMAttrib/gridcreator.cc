/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nanne Hemstra
 Date:		January 2010
________________________________________________________________________

-*/



#include "gridcreator.h"

#include "trckeyzsampling.h"
#include "emmanager.h"
#include "emhorizon3d.h"
#include "emhorizon2d.h"
#include "grid2d.h"
#include "hor2dfrom3dcreator.h"
#include "ioobj.h"
#include "iopar.h"
#include "keystrs.h"
#include "uistrings.h"
#include "dbkey.h"
#include "randomlinegeom.h"
#include "seis2ddata.h"
#include "seisrandlineto2d.h"
#include "seisprovider.h"
#include "seisrangeseldata.h"
#include "seistrc.h"
#include "seisstorer.h"
#include "separstr.h"
#include "survgeom2d.h"
#include "survgeommgr.h"


const char* Seis2DGridCreator::sKeyOverWrite()	{ return "Do Overwrite"; }
const char* Seis2DGridCreator::sKeyInput()	{ return "Input ID"; }
const char* Seis2DGridCreator::sKeyOutput()	{ return "Output ID"; }
const char* Seis2DGridCreator::sKeySelType()	{ return "Selection Type"; }
const char* Seis2DGridCreator::sKeyOutpAttrib() { return "Output Attribute"; }
const char* Seis2DGridCreator::sKeyInlSelType() { return "Inl Sel Type"; }
const char* Seis2DGridCreator::sKeyCrlSelType() { return "Crl Sel Type"; }

const char* Seis2DGridCreator::sKeyBaseLine()	{ return "BaseLine"; }
const char* Seis2DGridCreator::sKeyStartBinID() { return "Start BinID"; }
const char* Seis2DGridCreator::sKeyStopBinID()	{ return "Stop BinID"; }
const char* Seis2DGridCreator::sKeyInlSpacing() { return "Inl Spacing"; }
const char* Seis2DGridCreator::sKeyCrlSpacing() { return "Crl Spacing"; }

const char* Seis2DGridCreator::sKeyInlPrefix()	{ return "Inl Prefix"; }
const char* Seis2DGridCreator::sKeyCrlPrefix()	{ return "Crl Prefix"; }

class Seis2DLineCreator : public Executor
{ mODTextTranslationClass(Seis2DLineCreator);
public:
			Seis2DLineCreator(const IOObj& input,
					  const TrcKeyZSampling&,
					  const IOObj& output,
					  Pos::GeomID gepmid);
			~Seis2DLineCreator();

    uiString		message() const;
    uiString		nrDoneText() const;
    virtual od_int64	nrDone() const		{ return nrdone_; }
    virtual od_int64	totalNr() const		{ return totalnr_; }
    virtual int		nextStep();

protected:
    od_int64		nrdone_;
    od_int64		totalnr_;
    uiString		errmsg_;

    Seis::Provider*	prov_;
    Seis::Storer*	storer_;
};


Seis2DLineCreator::Seis2DLineCreator( const IOObj& input,
    const TrcKeyZSampling& cs, const IOObj& output, Pos::GeomID geomid )
    : Executor("Creating 2D line")
    , nrdone_(0)
    , totalnr_(cs.hsamp_.totalNr())
{
    uiRetVal uirv;
    prov_ = Seis::Provider::create( input, &uirv );
    if ( prov_ )
	prov_->setSelData( new Seis::RangeSelData(cs) );
    else
	errmsg_ = uirv;

    storer_ = new Seis::Storer( output );
    storer_->setFixedGeomID( geomid );
}


Seis2DLineCreator::~Seis2DLineCreator()
{
    delete prov_;
    delete storer_;
}

uiString Seis2DLineCreator::message() const
{ return errmsg_.isEmpty() ? uiStrings::phrHandling(uiStrings::sTrace(mPlural))
       : errmsg_; }
uiString Seis2DLineCreator::nrDoneText() const
{ return uiStrings::phrHandled(uiStrings::sTrace(mPlural)); }


int Seis2DLineCreator::nextStep()
{
    if ( !prov_ )
	return ErrorOccurred();

    SeisTrc trc;
    const uiRetVal uirv = prov_->getNext( trc );
    if ( !uirv.isOK() )
    {
	if ( isFinished(uirv) )
	    return Finished();

	errmsg_ = uirv;
	return ErrorOccurred();
    }

    errmsg_ = storer_->put( trc );
    if ( !errmsg_.isEmpty() )
	return ErrorOccurred();

    nrdone_++;
    return MoreToDo();
}



Seis2DGridCreator::Seis2DGridCreator( const IOPar& par )
    : ExecutorGroup("Creating Seis 2D Grid")
{
    init( par );
}


Seis2DGridCreator::~Seis2DGridCreator()
{}

uiString Seis2DGridCreator::nrDoneText() const
{ return tr("Traces done"); }

bool Seis2DGridCreator::init( const IOPar& par )
{
    DBKey key;
    par.get( Seis2DGridCreator::sKeyInput(), key );
    PtrMan<IOObj> input = key.getIOObj();

    par.get( Seis2DGridCreator::sKeyOutput(), key );
    PtrMan<IOObj> output = key.getIOObj();
    if ( !input || !output )
	return false;

    TrcKeyZSampling bbox;
    PtrMan<IOPar> subselpar = par.subselect( sKey::Subsel() );
    if ( subselpar )
	bbox.usePar( *subselpar );

    BufferString seltype;
    par.get( sKeySelType(), seltype );
    return seltype == "InlCrl" ? initFromInlCrl(par,*input,*output,bbox)
			       : initFromRandomLine(par,*input,*output,bbox);
}

#define mHandleLineGeom \
uiString errmsg; \
Pos::GeomID geomid = SurvGeom::getGeomID( linenm ); \
const bool islinepresent = geomid.isValid(); \
if ( !islinepresent ) \
{ \
    PosInfo::Line2DData* l2d = new PosInfo::Line2DData( linenm ); \
    SurvGeom2D* newgoem2d = new SurvGeom2D( l2d ); \
    newgoem2d->ref(); \
    Survey::GMAdmin().addEntry( newgoem2d, geomid, errmsg ); \
    newgoem2d->unRef(); \
    if ( !geomid.isValid() ) \
    { \
	failedlines_.add( linenm ); \
	continue; \
    } \
} \
else if ( islinepresent && dooverwrite ) \
{ \
    const auto& geom = SurvGeom::get2D( geomid ); \
    const_cast<SurvGeom2D&>(geom).data().setEmpty(); \
}

bool Seis2DGridCreator::initFromInlCrl( const IOPar& par,
					const IOObj& input, const IOObj& output,
					const TrcKeyZSampling& bbox )
{
    BufferString attribname;
    par.get( sKey::Attribute(), attribname );

    TypeSet<int> inlines;
    BufferString mode;
    par.get( Seis2DGridCreator::sKeyInlSelType(), mode );
    if ( mode == sKey::Range() )
    {
	StepInterval<int> range;
	par.get( sKey::InlRange(), range );
	for ( int idx=0; idx<=range.nrSteps(); idx++ )
	    inlines += range.atIndex( idx );
    }
    else
    {
	SeparString str( par.find(sKey::InlRange()) );
	const int nrinls = str.size();
	for ( int idx=0; idx<nrinls; idx++ )
	    inlines += str.getIValue(idx);
    }

    bool dooverwrite = false;
    if ( par.find(sKeyOverWrite()) )
	par.getYN( sKeyOverWrite(), dooverwrite );
    FixedString inlstr = par.find( sKeyInlPrefix() );
    failedlines_.setEmpty();
    for ( int idx=0; idx<inlines.size(); idx++ )
    {
	TrcKeyZSampling cs = bbox;
	cs.hsamp_.start_.inl() = cs.hsamp_.stop_.inl() = inlines[idx];
	BufferString linenm( inlstr.str() );
	linenm.add( inlines[idx] );

	mHandleLineGeom
	add( new Seis2DLineCreator(input,cs,output,geomid) );
    }

    TypeSet<int> crosslines;
    par.get( Seis2DGridCreator::sKeyCrlSelType(), mode );
    if ( mode == sKey::Range() )
    {
	StepInterval<int> range;
	par.get( sKey::CrlRange(), range );
	for ( int idx=0; idx<=range.nrSteps(); idx++ )
	    crosslines += range.atIndex( idx );
    }
    else
    {
	SeparString str( par.find(sKey::CrlRange()) );
	for ( int idx=0; idx<str.size(); idx++ )
	    crosslines += str.getIValue(idx);
    }

    FixedString crlstr = par.find( sKeyCrlPrefix() );
    for ( int idx=0; idx<crosslines.size(); idx++ )
    {
	TrcKeyZSampling cs = bbox;
	cs.hsamp_.start_.crl() = cs.hsamp_.stop_.crl() = crosslines[idx];
	BufferString linenm( crlstr.str() );
	linenm.add( crosslines[idx] );
	mHandleLineGeom
	add( new Seis2DLineCreator(input,cs,output,geomid) );
    }

    return true;
}


bool Seis2DGridCreator::initFromRandomLine( const IOPar& par,
					const IOObj& input, const IOObj& output,
					const TrcKeyZSampling& bbox )
{
    PtrMan<IOPar> baselinepar = par.subselect( sKeyBaseLine() );
    if ( !baselinepar )
	return false;

    BinID start, stop;
    if ( !baselinepar->get(sKeyStartBinID(),start)
	    || !baselinepar->get(sKeyStopBinID(),stop) )
	return false;

    Grid2D::Line baseline( start, stop );
    double pardist, perdist;
    if ( !par.get(sKeyInlSpacing(),pardist)
	    || !par.get(sKeyCrlSpacing(),perdist) )
	return false;

    Grid2D grid;
    grid.set( baseline, pardist, perdist, bbox.hsamp_ );
    if ( !grid.totalSize() )
	return false;

    failedlines_.setEmpty();
    BufferString attribname;
    par.get( sKey::Attribute(), attribname );
    bool dooverwrite = false;
    if ( par.find(sKeyOverWrite()) )
	par.getYN( sKeyOverWrite(), dooverwrite );

    FixedString parstr = par.find( sKeyInlPrefix() );
    for ( int idx=0; idx<grid.size(true); idx++ )
    {
	const Grid2D::Line* line = grid.getLine( idx, true );
	if ( !line )
	    continue;

	BufferString linenm( parstr.str() );
	linenm.add( idx );
	mHandleLineGeom

	RefMan<Geometry::RandomLine> rdl = new Geometry::RandomLine;
	rdl->addNode( line->start_ );
	rdl->addNode( line->stop_ );
	add( new SeisRandLineTo2D(input,output,geomid,1,*rdl) );
    }

    FixedString perstr = par.find( sKeyCrlPrefix() );
    for ( int idx=0; idx<grid.size(false); idx++ )
    {
	const Grid2D::Line* line = grid.getLine( idx, false );
	if ( !line )
	    continue;

	BufferString linenm( perstr.str() );
	linenm.add( idx );
	mHandleLineGeom
	RefMan<Geometry::RandomLine> rdl = new Geometry::RandomLine;
	rdl->addNode( line->start_ );
	rdl->addNode( line->stop_ );
	add( new SeisRandLineTo2D(input,output,geomid,1,*rdl) );
    }

    return true;
}



bool Seis2DGridCreator::hasWarning( BufferString& msg ) const
{
    msg.setEmpty();
    if ( failedlines_.isEmpty() )
	return false;
    msg.set( "Warning : Following lines could not be created " );
    msg += failedlines_.getDispString();
    return true;
}


// ---- Horizon2DGridCreator ----
const char* Horizon2DGridCreator::sKeyInputIDs() { return "Input IDs"; }
const char* Horizon2DGridCreator::sKeySeisID()	 { return "Seis ID"; }
const char* Horizon2DGridCreator::sKeyPrefix()	 { return "Name Prefix"; }

Horizon2DGridCreator::Horizon2DGridCreator()
    : ExecutorGroup("Extracting 2D horizons")
{}

Horizon2DGridCreator::~Horizon2DGridCreator()
{
    deepUnRef ( horizons_ );
}

od_int64 Horizon2DGridCreator::totalNr() const
{ return totalnr_; }

od_int64 Horizon2DGridCreator::nrDone() const
{ return nrdone_; }

uiString Horizon2DGridCreator::nrDoneText() const
{ return uiStrings::sPositionsDone(); }


bool Horizon2DGridCreator::init( const IOPar& par,
				 const TaskRunnerProvider& trprov )
{
    BufferString prefix;
    par.get( Horizon2DGridCreator::sKeyPrefix(), prefix );

    DBKey dsid;
    par.get( Horizon2DGridCreator::sKeySeisID(), dsid );
    PtrMan<IOObj> dsioobj = dsid.getIOObj();
    if ( !dsioobj )
	return false;

    GeomIDSet geomids;
    Seis2DDataSet ds( *dsioobj );
    ds.getGeomIDs( geomids );
    BufferStringSet horids;

    par.get( Horizon2DGridCreator::sKeyInputIDs(), horids );
    for ( int idx=0; idx<horids.size(); idx++ )
    {
	const DBKey dbky( horids.get(idx) );
	RefMan<EM::Object> emobj =
			EM::Hor3DMan().loadIfNotFullyLoaded( dbky, trprov );

	mDynamicCastGet(EM::Horizon3D*,horizon3d,emobj.ptr());
	if ( !horizon3d ) continue;

	horizon3d->ref();
	BufferString hornm( prefix, " ", horizon3d->name() );
	EM::Object* emobj2d =
		EM::Hor2DMan().createObject( EM::Horizon2D::typeStr(),hornm );
	mDynamicCastGet(EM::Horizon2D*,horizon2d,emobj2d)
	if ( !horizon2d ) continue;

	horizon2d->ref();
	horizons_ += horizon2d;
	Hor2DFrom3DCreatorGrp* creator =
	    new Hor2DFrom3DCreatorGrp( *horizon3d, *horizon2d );
	creator->init( geomids );
	add( creator );
	horizon3d->unRef();
    }

    return true;
}


bool Horizon2DGridCreator::finish( const TaskRunnerProvider& trprov )
{
    bool allok = true;
    for ( int idx=0; idx<horizons_.size(); idx++ )
    {
	PtrMan<Executor> saver = horizons_[idx]->saver();
	allok = trprov.execute( *saver ) && allok;
    }
    return allok;
}
