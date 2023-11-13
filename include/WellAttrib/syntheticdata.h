#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "wellattribmod.h"

#include "datapack.h"
#include "flatview.h"
#include "sharedobject.h"
#include "stratsynthgenparams.h"
#include "synthseis.h"

class FlatDataPack;


/*! brief the basic synthetic dataset. contains the data cubes*/
mExpClass(WellAttrib) SyntheticData : public SharedObject
{
public:

    static ConstRefMan<SyntheticData>	get(const SynthGenParams&,
					    Seis::RaySynthGenerator&);

    void				setName(const char*) override;

    bool				isOK() const;
    virtual const SeisTrc*		getTrace(int trcnr) const	= 0;
    virtual TrcKey			getTrcKey(int trcnr) const	= 0;
    virtual int				nrPositions() const		= 0;
    virtual ZSampling			zRange() const			= 0;

    float				getTime(float dpt,int trcnr) const;
    float				getDepth(float time,int trcnr) const;

    const DataPack&			getPack() const { return *datapack_; }
    DataPack&				getPack()	{ return *datapack_; }
    virtual DataPack::FullID		fullID() const			= 0;

    const Seis::SynthGenDataPack&	synthGenDP() const;
    const ReflectivityModelSet&		getRefModels() const;
    const ReflectivityModelBase*	getRefModel(int itrc) const;
    const TimeDepthModel*		getTDModel(int itrc) const;
    const TimeDepthModel*		getTDModel(int itrc,int ioff) const;

    SynthID				id() const		{ return id_; }

    virtual bool			isPS() const		= 0;
    virtual bool			hasOffset() const	= 0;
    virtual bool			isAngleStack() const  { return false; }
    virtual bool			isAVOGradient() const { return false; }
    virtual bool			isStratProp() const   { return false; }
    virtual bool			isAttribute() const   { return false; }
    virtual bool			isFilter() const      { return false; }
    virtual SynthGenParams::SynthType	synthType() const	= 0;

    const SynthGenParams&		getGenParams() const	{ return sgp_; }
    void				useGenParams(const SynthGenParams&);
    const char*				waveletName() const;

    virtual ConstRefMan<FlatDataPack>	getTrcDP() const		= 0;
				//<! display datapack for a FlatViewer only
    virtual ConstRefMan<FlatDataPack>	getFlattenedTrcDP(
						const TypeSet<float>& zvals,
						bool istime) const = 0;
				//<! display datapack for a FlatViewer only

protected:
				SyntheticData(const SynthGenParams&,
					      const Seis::SynthGenDataPack&,
					      DataPack&);
				~SyntheticData();

    SynthID			id_;
    SynthGenParams		sgp_;
    RefMan<DataPack>		datapack_;
    ConstRefMan<Seis::SynthGenDataPack> synthgendp_;

public:
				// Used for obtaining an ID by a managing object
    static SynthID		getNewID();

				// Used by managing object
    void			setID( SynthID newid )	{ id_ = newid; }

};
