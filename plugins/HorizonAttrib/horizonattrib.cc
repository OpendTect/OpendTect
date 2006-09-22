/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Nanne Hemstra
 Date:		September 2006
 RCS:		$Id: horizonattrib.cc,v 1.2 2006-09-22 15:12:44 cvsnanne Exp $
________________________________________________________________________

-*/

#include "horizonattrib.h"

#include "attribdataholder.h"
#include "attribdesc.h"
#include "attribfactory.h"
#include "attribparam.h"
#include "emhorizon.h"
#include "emmanager.h"
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
    EM::SurfaceIODataSelection sel( sd );
    if ( getDesiredVolume() )
	sel.rg = getDesiredVolume()->hrg;
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
			   int z0, int nrsamples ) const
{
    if ( !horizon_ ) return false;

    const BinID bid = currentbid + relpos;
    EM::SectionID sid = horizon_->sectionID( 0 );
    const float zval = horizon_->getPos( sid, bid.getSerialized() ).z;
    for ( int idx=0; idx<nrsamples; idx++ )
    {
	if ( isOutputEnabled(0) )
	    setOutputValue( output, 0, idx, z0, zval );
	if ( isOutputEnabled(1) )
	{
	    const float val = getInputValue( *inputdata_, dataidx_, idx, z0 );
	    setOutputValue( output, 1, idx, z0, val );
	}
    }

    return true;
}


} // namespace Attrib
