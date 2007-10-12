/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Nanne Hemstra
 Date:		September 2006
 RCS:		$Id: horizonattrib.cc,v 1.9 2007-10-12 14:26:51 cvshelene Exp $
________________________________________________________________________

-*/

#include "horizonattrib.h"

#include "attribdataholder.h"
#include "attribdesc.h"
#include "attribfactory.h"
#include "attribparam.h"
#include "emhorizon3d.h"
#include "emmanager.h"
#include "emsurfaceauxdata.h"
#include "emsurfaceiodata.h"
#include "executor.h"
#include "ptrman.h"

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
    
    StringParam* surfidpar = new StringParam( sKeySurfDataName() );
    surfidpar->setEnabled( false );
    desc->addParam( surfidpar );

    desc->addInput( InputSpec("Input data for Horizon",true) );//positioning
    desc->addOutputDataType( Seis::UnknowData );

    mAttrEndInitClass
}


void Horizon::updateDesc( Desc& desc )
{
    BufferString type = desc.getValParam(sKeyType())->getStringValue();
    const bool issurfdata = type==outTypeNamesStr( mOutTypeSurfData );
    desc.setParamEnabled( sKeySurfDataName(), issurfdata );
}


Horizon::Horizon( Desc& dsc )
    : Provider(dsc)
    , inputdata_(0)
    , horizon_(0)
{ 
    BufferString idstr = desc.getValParam( sKeyHorID() )->getStringValue();
    horid_ = MultiID( idstr.buf() );

    mGetEnum( outtype_, sKeyType() );
    if ( outtype_ == mOutTypeSurfData )
	mGetString( surfdatanm_, sKeySurfDataName() );

    if ( !isOK() )
    {
	errmsg = "Selected surface data name does not exist";
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


const char* Horizon::outTypeNamesStr(int type)
{
    if ( type == mOutTypeZ ) return "Z";
    return "Surface Data";
}


bool Horizon::getInputData( const BinID& relpos, int zintv )
{
    inputdata_ = inputs[0]->getData( relpos, zintv );
    dataidx_ = getDataIndex( 0 );
    return inputdata_;
}


void Horizon::prepareForComputeData()
{
    EM::EMManager& em = EM::EMM();
    EM::ObjectID objid = em.getObjectID( horid_ );
    if ( objid > -1 )
    {
	mDynamicCastGet(EM::Horizon3D*,hor,em.getObject(objid))
	if ( hor && hor->isFullyLoaded() 
	     && ( outtype_!=mOutTypeSurfData || hor->auxdata.nrAuxData() ) )
	{
	    horizon_ = hor;
	    horizon_->ref();
	    return;
	}
    }

    EM::SurfaceIOData sd;
    EM::EMM().getSurfaceData( horid_, sd );
    EM::SurfaceIODataSelection sel( sd );
    if ( getDesiredVolume() )
	sel.rg = getDesiredVolume()->hrg;

    int surfdtidx = sd.valnames.indexOf( surfdatanm_ );
    if ( surfdtidx<0 && outtype_==mOutTypeSurfData ) return;
    else if ( surfdtidx >= 0 )
	sel.selvalues += surfdtidx;

    PtrMan<Executor> loader = em.objectLoader( horid_, &sel );
    if ( !loader ) return;

    loader->execute();
    objid = em.getObjectID( horid_ );
    mDynamicCastGet(EM::Horizon3D*,hor,em.getObject(objid))
    if ( hor )
    {
	horizon_ = hor;
	horizon_->ref();
    }

    Provider::prepareForComputeData();
}


bool Horizon::computeData( const DataHolder& output, const BinID& relpos,
			   int z0, int nrsamples, int threadid ) const
{
    if ( !horizon_ ) return false;

    const BinID bid = currentbid + relpos;
    const EM::PosID posid( horizon_->id(), horizon_->sectionID(0),
	    		   bid.getSerialized() );
    const float zval = horizon_->getPos( posid ).z;

    float outputvalue = mUdf(float);
    if ( outtype_ == mOutTypeZ )
	outputvalue = zval;
    else
    {
	int auxindex = horizon_->auxdata.auxDataIndex( surfdatanm_ );
	outputvalue = horizon_->auxdata.getAuxDataVal( auxindex, posid );
    }
    
    for ( int idx=0; idx<nrsamples; idx++ )
	setOutputValue( output, 0, idx, z0, outputvalue );

    return true;
}


} // namespace Attrib
