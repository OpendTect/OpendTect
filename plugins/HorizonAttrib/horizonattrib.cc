/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nanne Hemstra
 Date:		September 2006
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "horizonattrib.h"

#include "attribdataholder.h"
#include "attribdesc.h"
#include "attribfactory.h"
#include "attribparam.h"
#include "emhorizon3d.h"
#include "emhorizon2d.h"
#include "emmanager.h"
#include "emsurfaceauxdata.h"
#include "emsurfaceiodata.h"
#include "executor.h"
#include "ptrman.h"
#include "ioman.h"
#include "ioobj.h"

#define mOutTypeZ		0
#define mOutTypeSurfData	1


namespace Attrib
{

mAttrDefCreateInstance(Horizon)
    
void Horizon::initClass()
{
    mAttrStartInitClassWithUpdate

    desc->addParam( new StringParam(sKeyHorID()) );
    
    EnumParam* type = new EnumParam( sKeyType() );
    //Note: Ordering must be the same as numbering!
    type->addEnum( outTypeNamesStr(mOutTypeZ) );
    type->addEnum( outTypeNamesStr(mOutTypeSurfData) );
    desc->addParam( type );

    desc->addParam( new BoolParam( sKeyRelZ(), false, false ) );
    
    StringParam* surfidpar = new StringParam( sKeySurfDataName() );
    surfidpar->setEnabled( false );
    desc->addParam( surfidpar );

    desc->addInput( InputSpec("Input data for Horizon",true) );//positioning
    desc->addOutputDataType( Seis::UnknowData );

    desc->setLocality( Desc::SingleTrace );
    desc->setUsesTrcPos( true );
    mAttrEndInitClass
}


void Horizon::updateDesc( Desc& desc )
{
    BufferString type = desc.getValParam(sKeyType())->getStringValue();
    const bool issurfdata = type==outTypeNamesStr( mOutTypeSurfData );
    desc.setParamEnabled( sKeySurfDataName(), issurfdata );
    desc.setParamEnabled( sKeyRelZ(), !issurfdata );
}


Horizon::Horizon( Desc& dsc )
    : Provider(dsc)
    , inputdata_(0)
    , horizon_(0)
    , horizon2dlineid_( mUdf(int) )
    , relz_(false)
{ 
    BufferString idstr = desc_.getValParam( sKeyHorID() )->getStringValue();
    horid_ = MultiID( idstr.buf() );

    mGetEnum( outtype_, sKeyType() );
    if ( outtype_ == mOutTypeSurfData )
	{ mGetString( surfdatanm_, sKeySurfDataName() ); }
    else
	{ mGetBool( relz_, sKeyRelZ() ); }

    if ( !isOK() )
    {
	errmsg_ = "Selected Horizon Data name does not exist";
	return;
    }
}


Horizon::~Horizon()
{
    if ( horizon_ ) horizon_->unRef();
}


bool Horizon::isOK() const
{
    if ( outtype_==mOutTypeSurfData )
    {
	EM::SurfaceIOData sd;
	EM::EMM().getSurfaceData( horid_, sd );
	int surfdtidx = sd.valnames.indexOf( surfdatanm_ );
	if ( surfdtidx<0 ) return false;
    }

    return true;
}


const char* Horizon::outTypeNamesStr( int type )
{
    return type == mOutTypeZ ? "Z" : "Horizon Data";
}


bool Horizon::getInputData( const BinID& relpos, int zintv )
{
    inputdata_ = inputs_[0]->getData( relpos, zintv );
    dataidx_ = getDataIndex( 0 );
    return inputdata_;
}

#define mRet \
{ Provider::prepareForComputeData(); return; }

void Horizon::prepareForComputeData()
{
    EM::EMManager& em = EM::EMM();
    EM::SurfaceIOData sd;
    em.getSurfaceData( horid_, sd );
    const int surfdtidx = sd.valnames.indexOf( surfdatanm_ );
    if ( surfdtidx<0 && outtype_==mOutTypeSurfData ) mRet

    EM::ObjectID objid = em.getObjectID( horid_ );
    EM::SurfaceIODataSelection sel( sd );
    PtrMan<Executor> loader = 0;
    if ( objid < 0 )
    {
	if ( getDesiredVolume() )
	    sel.rg = getDesiredVolume()->hrg;

	loader = em.objectLoader( horid_, &sel );
	if ( !loader ) mRet
	
	loader->execute();	
	objid = em.getObjectID( horid_ );
    }

    mDynamicCastGet(EM::Horizon*,hor,em.getObject(objid))
    if ( !hor ) mRet

    horizon_ = hor;
    horizon_->ref();
    if ( desc_.is2D() )
	fillLineID();

    if ( outtype_ == mOutTypeZ || desc_.is2D() )
	mRet

    mDynamicCastGet(EM::Horizon3D*,hor3d,hor)
    const int auxdataidx = hor3d ? hor3d->auxdata.auxDataIndex(surfdatanm_) :-1;
    if ( auxdataidx != -1 ) mRet

    PtrMan<Executor> adl = hor3d ? hor3d->auxdata.auxDataLoader(surfdtidx) : 0;
    if ( !adl || !adl->execute() )
    {
	BufferString msg = "Loading Horizon Data ";
	msg += surfdatanm_;
	msg += " failed.";
	errmsg_ =  msg;
	horizon_->unRef();
	mRet
    }

    Provider::prepareForComputeData();
}


void Horizon::fillLineID()
{
    mDynamicCastGet(EM::Horizon2D*,hor2d,horizon_);
    MultiID lsid( desc_.getStoredID(true) );
    PtrMan<IOObj> lsobj = IOM().get( lsid );
    if ( !lsobj ) return;
    PosInfo::GeomID geomid = S2DPOS().getGeomID( lsobj->name(),
	    					  curlinekey_.lineName() );
    const int lineidx = hor2d->geometry().lineIndex( geomid );
    horizon2dlineid_ = lineidx==-1 ? mUdf(int) : lineidx;
}


bool Horizon::computeData( const DataHolder& output, const BinID& relpos,
			   int z0, int nrsamples, int threadid ) const
{
    if ( !horizon_ ) return false;

    RowCol rc = currentbid_ + relpos;
    if ( desc_.is2D() )
    {
	if ( mIsUdf(horizon2dlineid_) )
	    return false;

	rc = RowCol( horizon2dlineid_, currentbid_.crl+relpos.crl );
    }

    const EM::PosID posid( horizon_->id(), horizon_->sectionID(0),
	    		   rc.toInt64() );
    const float zval = (float) horizon_->getPos( posid ).z;

    const bool isz = outtype_ == mOutTypeZ;
    if ( relz_ && isz )
    {
	for ( int iz=0; iz<nrsamples; iz++ )
	{
	    const float ziz = (z0 + iz) * refstep_;
	    setOutputValue( output, 0, iz, z0, ziz - zval );
	}
    }
    else
    {
	float outputvalue = mUdf(float);
	if ( isz )
	    outputvalue = zval;
	else if ( !desc_.is2D() )
	{
	    mDynamicCastGet(EM::Horizon3D*,hor3d,horizon_)
	    if ( hor3d )
	    {
		int auxindex = hor3d->auxdata.auxDataIndex( surfdatanm_ );
		outputvalue = hor3d->auxdata.getAuxDataVal( auxindex, posid );
	    }
	}
	for ( int iz=0; iz<nrsamples; iz++ )
	    setOutputValue( output, 0, iz, z0, outputvalue );
    }

    return true;
}


} // namespace Attrib
