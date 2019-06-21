#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Satyaki/Bruno / Bert
 Date:		July 2013 / Sep 2018
________________________________________________________________________

-*/

#include "datapack.h"
#include "integerid.h"
#include "synthseisgenerator.h"
#include "synthseisgenparams.h"
#include "timedepthmodel.h"

class PropertyRef;
class SeisTrc;
class SeisTrcBufDataPack;
namespace ColTab { class MapperSetup; }


namespace SynthSeis
{


mExpClass(Seis) DispPars
{
public:

    typedef ColTab::MapperSetup	MapperSetup;

			DispPars();

    BufferString	colseqname_;
    RefMan<MapperSetup>	vdmapsetup_;
    RefMan<MapperSetup>	wvamapsetup_;
    float		overlap_;

    void		fillPar(IOPar&) const;
    void		usePar(const IOPar&);
};


/*! brief the basic synthetic dataset. contains the data cubes */

mExpClass(Seis) DataSet : public ::RefCount::Referenced
			, public ::NamedObject
{
public:

    mDefIntegerIDType(			MgrID );
    typedef TimeDepthModelSet		D2TModelSet;
    mUseType( TimeDepthModelSet,	idx_type );
    mUseType( TimeDepthModelSet,	size_type );
    typedef SynthSeis::SyntheticType	SynthType;
    typedef TypeSet<float>		ZValueSet;
    typedef StepInterval<float>		OffsetDef;

    MgrID		id() const		{ return id_; }
    SynthType		synthType() const	{ return genpars_.type_; }
    virtual bool	isPS() const		{ return true; }
    virtual bool	hasRayModel() const	{ return raymodels_.ptr(); }
    bool		hasOffset() const;
    virtual OffsetDef	offsetDef() const	{ return OffsetDef(0.f,0.f); }

    const GenParams&	genParams() const	{ return genpars_; }
    const IOPar&	rayPars() const		{ return genpars_.raypars_; }
    DispPars&		dispPars()		{ return disppars_; }
    const DispPars&	dispPars() const	{ return disppars_; }

    bool		isEmpty() const		{ return size() < 1; }
    size_type		size() const;
    const SeisTrc*	getTrace(idx_type,float offs=0.f) const;
    ZSampling		zRange() const;

    float		getTime(float dpt,int seqnr) const;
    float		getDepth(float time,int seqnr) const;
    const D2TModelSet&	d2TModels() const	{ return finald2tmodels_; }
    const RayModelSet&	rayModels() const	{ return *raymodels_.ptr(); }

    DataPackMgr::ID	dataPackMgrID() const	{ return dpMgrID(); }
    DataPack::FullID	dataPackID() const;
    DataPack&		dataPack()		{ return *datapack_; }
    const DataPack&	dataPack() const	{ return *datapack_; }
    ConstRefMan<DataPack> getTrcDPAtOffset(float offs=0.f) const;
    ConstRefMan<DataPack> getFlattenedTrcDP(const ZValueSet&,bool istime,
					    float offs=0.f) const;

    void		useDispPars(const IOPar&);
    void		fillDispPars(IOPar&) const;

    DBKey		waveletID() const	{ return genpars_.wvltid_; }
    BufferString	waveletName() const	{ return waveletID().name(); }

protected:

			DataSet(const GenParams&,DataPack&,RayModelSet*);
			DataSet(const DataSet&)		= delete;
			~DataSet();
    DataSet&		operator =(const DataSet&)	= delete;

    MgrID		id_;
    const GenParams	genpars_;
    DispPars		disppars_;

    RefMan<RayModelSet> raymodels_;
    RefMan<DataPack>	datapack_;
    D2TModelSet		finald2tmodels_;

    bool		validIdx( idx_type idx ) const
			{ return finald2tmodels_.validIdx( idx ); }

    virtual const DataPack*	gtTrcBufDP(float) const;
    virtual DataPackMgr::ID	dpMgrID() const				= 0;
    virtual const SeisTrc*	gtTrc(idx_type,float) const		= 0;

public:
			// just don't
    virtual void	setName(const char*)			{}

			// Use if you are a RaySynthGenerator
    RayModelSet&	rayMdls()		{ return *raymodels_.ptr(); }
    void		updateD2TModels();

			// For models directly from RayModel
    void		adjustD2TModelsToSRD(D2TModelSet&);

			// Used for obtaining an ID by a managing object
    static MgrID	getNewID();

			// Used by managing object
    void		setID( MgrID newid )	{ id_ = newid; }
    void		getD2TFrom(const DataSet&);

};



mExpClass(Seis) PostStackDataSet : public DataSet
{
public:

    typedef SeisTrcBufDataPack	DPType;

			PostStackDataSet(const GenParams&,DPType&,RayModelSet*);
			~PostStackDataSet();

    virtual bool	isPS() const		{ return false; }

    DPType&		postStackPack();
    const DPType&	postStackPack() const;

    static const char*	sDataPackCategory();

protected:

    virtual DataPackMgr::ID	dpMgrID() const;
    virtual const SeisTrc*	gtTrc(idx_type,float) const;
    virtual const DataPack*	gtTrcBufDP(float) const;

};


mExpClass(Seis) StratPropDataSet : public PostStackDataSet
{
public:

			StratPropDataSet(const GenParams&,DPType&,
					 const PropertyRef&);

    const PropertyRef&	propRef() const		{ return prop_; }

protected:

    const PropertyRef&		prop_;

};


mExpClass(Seis) PSBasedPostStackDataSet : public PostStackDataSet
{
public:

			PSBasedPostStackDataSet(const GenParams&,DPType&);

};


mExpClass(Seis) AVOGradDataSet : public PSBasedPostStackDataSet
{
public:

			AVOGradDataSet( const GenParams& gp, DPType& dp )
			    : PSBasedPostStackDataSet(gp,dp) {}

};


mExpClass(Seis) AngleStackDataSet : public PSBasedPostStackDataSet
{
public:

			AngleStackDataSet( const GenParams& gp, DPType& dp )
			    : PSBasedPostStackDataSet(gp,dp) {}

};


} // namespace SynthSeis
