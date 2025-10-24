/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "tutexternalattribrandom.h"

#include "arraynd.h"
#include "attribdesc.h"
#include "attribsel.h"
#include "randomlinegeom.h"
#include "statrand.h"
#include "survinfo.h"
#include "zdomain.h"


// ExternalAttrib::Random

ExternalAttrib::Random::Random()
    : gen_(*new Stats::RandGen())
    , zdom_(&SI().zDomainInfo())
{}


ExternalAttrib::Random::~Random()
{
    delete &gen_;
}


bool ExternalAttrib::Random::setTargetSelSpec( const Attrib::SelSpec& spec )
{
    if ( !checkSelSpec(spec) )
	return false;

    dpname_ = spec.userRef();
    const ZDomain::Info& zdomain = ZDomain::Info::getFrom( spec.zDomainKey(),
							   spec.zDomainUnit() );
    zdom_ = &zdomain;

    return true;
}


ConstRefMan<RegularSeisDataPack>
ExternalAttrib::Random::createAttrib( const TrcKeyZSampling& tkzs,
		      const RegularSeisDataPack* /*cachedp*/,
		      TaskRunner* /* taskrun */ )
{
    RefMan<RegularSeisDataPack> regsdp = new RegularSeisDataPack(
					    VolumeDataPack::categoryStr(tkzs) );
    if ( !regsdp )
	return nullptr;

    regsdp->setName( dpname_.buf() );
    regsdp->setSampling( tkzs );
    regsdp->setZDomain( *zdom_ );
    if ( !regsdp->addComponent(dpname_) )
	return nullptr;

    Array3D<float>& arr = regsdp->data();
    for ( int idx=0; idx<arr.getSize(0); idx++ )
    {
	for ( int idy=0; idy<arr.getSize(1); idy++ )
	{
	    for ( int idz=0; idz<arr.getSize(2); idz++ )
	    {
		arr.set( idx, idy, idz, gen_.get() );
	    }
	}
    }

    return regsdp;
}


ConstRefMan<RegularSeisDataPack>
ExternalAttrib::Random::createAttrib( const TrcKeyZSampling& tkzs,
				      TaskRunner* taskr )
{
    return createAttrib( tkzs, nullptr, taskr );
}


bool ExternalAttrib::Random::createAttrib( ObjectSet<BinIDValueSet>& o,
					   TaskRunner* taskrun )
{
    return Attrib::ExtAttribCalc::createAttrib( o, taskrun );
}


bool ExternalAttrib::Random::createAttrib( const BinIDValueSet& b,
					   SeisTrcBuf& tb, TaskRunner* taskrun )
{
    return Attrib::ExtAttribCalc::createAttrib( b, tb, taskrun );
}


ConstRefMan<RandomSeisDataPack>
 ExternalAttrib::Random::createRdmTrcAttrib( const ZGate& zrg,
				const RandomLineID& rdlid, TaskRunner* taskrun )
{
    ConstRefMan<Geometry::RandomLine> rdmline = Geometry::RLM().get( rdlid );
    if ( !rdmline )
	return nullptr;

    TrcKeySet knots, trckeys;
    rdmline->allNodePositions( knots );
    rdmline->getPathBids( knots, trckeys );
    if ( trckeys.isEmpty() )
	return nullptr;

    RefMan<RandomSeisDataPack> randsdp =
	    new RandomSeisDataPack( VolumeDataPack::categoryStr(true,false) );
    if ( !randsdp )
	return nullptr;

    const ZSampling zsamp( zrg, zdom_->getReasonableZSampling(false).step_ );

    randsdp->setName( dpname_ );
    randsdp->setRandomLineID( rdlid );
    randsdp->setPath( trckeys );
    randsdp->setZRange( zsamp );
    randsdp->setZDomain( *zdom_ );
    if ( !randsdp->addComponent(dpname_) )
	return nullptr;

    Array3D<float>& arr = randsdp->data();
    for ( int idx=0; idx<arr.getSize(0); idx++ )
    {
	for ( int idy=0; idy<arr.getSize(1); idy++ )
	{
	    for ( int idz=0; idz<arr.getSize(2); idz++ )
	    {
		arr.set( idx, idy, idz, gen_.get() );
	    }
	}
    }

    return randsdp;
}


BufferString ExternalAttrib::Random::createDefinition()
{
    return sFactoryKeyword();
}


uiString ExternalAttrib::Random::createDisplayName()
{
    return sFactoryDisplayName();
}


bool ExternalAttrib::Random::sCheckSelSpec( const Attrib::SelSpec& spec )
{
    return Attrib::ExtAttribCalc::sCheckSelSpec( spec, sFactoryKeyword() );
}
