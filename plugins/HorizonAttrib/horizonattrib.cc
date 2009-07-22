/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nanne Hemstra
 Date:		September 2006
________________________________________________________________________

-*/
static const char* rcsID = "$Id: horizonattrib.cc,v 1.16 2009-07-22 16:01:27 cvsbert Exp $";

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
    BufferString idstr = desc.getValParam( sKeyHorID() )->getStringValue();
    horid_ = MultiID( idstr.buf() );

    mGetEnum( outtype_, sKeyType() );
    if ( outtype_ == mOutTypeSurfData )
	{ mGetString( surfdatanm_, sKeySurfDataName() ); }
    else
	{ mGetBool( relz_, sKeyRelZ() ); }

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


const char* Horizon::outTypeNamesStr( int type )
{
    return type == mOutTypeZ ? "Z" : "Surface Data";
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
    BufferStringSet loadedauxdatanms;
    if ( objid > -1 )
    {
	mDynamicCastGet(EM::Horizon*,hor,em.getObject(objid))
	if ( hor && hor->isFullyLoaded() )
	{
	    if ( outtype_!=mOutTypeSurfData )
	    {
		horizon_ = hor;
		horizon_->ref();
		if ( desc.is2D() )
		    fillLineID();
		return;
	    }
	    else if ( !desc.is2D() )
	    {
		mDynamicCastGet(EM::Horizon3D*,hor3d,hor)
		for ( int idx=0; hor3d && idx<hor3d->auxdata.nrAuxData(); idx++)
		{
		    const char* auxnm = hor3d->auxdata.auxDataName(idx);
		    if ( !strcmp( auxnm, surfdatanm_ ) )
		    {
			horizon_ = hor;
			horizon_->ref();
			return;
		    }
		    
		    loadedauxdatanms.add( auxnm );
		}
	    }
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

    for ( int idx=0; idx<loadedauxdatanms.size(); idx++ )
    {
	int tmpsurfdtidx = sd.valnames.indexOf( loadedauxdatanms.get(idx) );
	if ( tmpsurfdtidx >= 0 )
	    sel.selvalues += tmpsurfdtidx;
    }
    
    PtrMan<Executor> loader = em.objectLoader( horid_, &sel );
    if ( !loader ) return;

    loader->execute();
    objid = em.getObjectID( horid_ );
    mDynamicCastGet(EM::Horizon*,hor,em.getObject(objid))
    if ( hor )
    {
	horizon_ = hor;
	horizon_->ref();
    }

    if ( desc.is2D() )
	fillLineID();

    Provider::prepareForComputeData();
}


void Horizon::fillLineID()
{
    mDynamicCastGet(EM::Horizon2D*,hor2d,horizon_);
    const int lineidx = hor2d->geometry().lineIndex( curlinekey_.lineName() );
    horizon2dlineid_ = lineidx==-1 ? mUdf(int)
				  : hor2d->geometry().lineID( lineidx );
}


bool Horizon::computeData( const DataHolder& output, const BinID& relpos,
			   int z0, int nrsamples, int threadid ) const
{
    if ( !horizon_ ) return false;

    RowCol rc = currentbid + relpos;
    if ( desc.is2D() )
    {
	if ( mIsUdf(horizon2dlineid_) )
	    return false;

	rc = RowCol( horizon2dlineid_, currentbid.crl+relpos.crl );
    }

    const EM::PosID posid( horizon_->id(), horizon_->sectionID(0),
	    		   rc.getSerialized() );
    const float zval = horizon_->getPos( posid ).z;

    const bool isz = outtype_ == mOutTypeZ;
    if ( relz_ && isz )
    {
	for ( int iz=0; iz<nrsamples; iz++ )
	{
	    const float ziz = (z0 + iz) * refstep;
	    setOutputValue( output, 0, iz, z0, ziz - zval );
	}
    }
    else
    {
	float outputvalue = mUdf(float);
	if ( isz )
	    outputvalue = zval;
	else if ( !desc.is2D() )
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
