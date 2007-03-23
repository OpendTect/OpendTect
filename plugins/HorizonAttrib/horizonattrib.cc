/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Nanne Hemstra
 Date:		September 2006
 RCS:		$Id: horizonattrib.cc,v 1.5 2007-03-23 13:51:52 cvshelene Exp $
________________________________________________________________________

-*/

#include "horizonattrib.h"

#include "attribdataholder.h"
#include "attribdesc.h"
#include "attribfactory.h"
#include "attribparam.h"
#include "emhorizon.h"
#include "emmanager.h"
#include "emsurfaceauxdata.h"
#include "emsurfaceiodata.h"
#include "executor.h"
#include "ptrman.h"


namespace Attrib
{

mAttrDefCreateInstance(Horizon)
    
void Horizon::initClass()
{
    Desc* desc = new Desc( attribName(), updateDesc );
    desc->ref();

    desc->addParam( new StringParam(sKeyHorID()) );
    StringParam* fnmpar = new StringParam( sKeyFileName() );
    fnmpar->setRequired( false );
    desc->addParam( fnmpar );

    desc->addInput( InputSpec("Input data for Horizon",true) );
    desc->setNrOutputs( Seis::UnknowData, 2 );

    mAttrEndInitClass
}


void Horizon::updateDesc( Desc& desc )
{
    BufferString idstr = desc.getValParam( sKeyHorID() )->getStringValue();
    MultiID horid( idstr.buf() );
    EM::SurfaceIOData iodata;
    const char* err = EM::EMM().getSurfaceData( horid, iodata );
    if ( err ) return;

    desc.setNrOutputs( Seis::UnknowData, iodata.valnames.size()+2 );
}


Horizon::Horizon( Desc& dsc )
    : Provider(dsc)
    , inputdata_(0)
    , horizon_(0)
{ 
    if ( !isOK() ) return;

    BufferString idstr = desc.getValParam( sKeyHorID() )->getStringValue();
    horid_ = MultiID( idstr.buf() );
}


Horizon::~Horizon()
{
    if ( horizon_ ) horizon_->unRef();
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
	mDynamicCastGet(EM::Horizon*,hor,em.getObject(objid))
	if ( hor && hor->isFullyLoaded() )
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

    for ( int idx=0; idx<sd.valnames.size(); idx++ )
	sel.selvalues += idx;

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
}


bool Horizon::computeData( const DataHolder& output, const BinID& relpos,
			   int z0, int nrsamples, int threadid ) const
{
    if ( !horizon_ ) return false;

    const BinID bid = currentbid + relpos;
    const EM::PosID posid( horizon_->id(), horizon_->sectionID(0),
	    		   bid.getSerialized() );
    const float zval = horizon_->getPos( posid ).z;

    TypeSet<float> outputvalues( nrOutputs(), mUdf(float) );
    for ( int idx=0; idx<nrOutputs(); idx++ )
    {
	if ( idx==0 && isOutputEnabled(0) )
	    outputvalues[0] = zval;
	else if ( idx==1 && isOutputEnabled(1) )
	    outputvalues[1] =
	    		getInterpolInputValue( *inputdata_, dataidx_, zval );
	else if ( isOutputEnabled(idx) ) // surface data start at idx=2
	    outputvalues[idx] = horizon_->auxdata.getAuxDataVal( idx-2, posid );
    }

    for ( int idx=0; idx<nrsamples; idx++ )
    {
	for ( int outidx=0; outidx<nrOutputs(); outidx++ )
	{
	    if ( isOutputEnabled(outidx) )
		setOutputValue( output, outidx, idx, z0, outputvalues[outidx] );
	}
    }

    return true;
}


} // namespace Attrib
