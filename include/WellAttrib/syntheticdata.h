#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Satyaki/Bruno
 Date:		July 2013
________________________________________________________________________

-*/

#include "wellattribmod.h"

#include "datapack.h"
#include "flatview.h"
#include "stratsynthgenparams.h"

class ReflectivityModelBase;
class ReflectivityModelSet;
class SeisTrc;
class TimeDepthModel;
namespace Seis { class RaySynthGenerator; class SynthGenDataPack; }


mStruct(WellAttrib) SynthFVSpecificDispPars
{
			SynthFVSpecificDispPars()
			: overlap_(1)	{}
    ColTab::MapperSetup	vdmapper_;
    ColTab::MapperSetup	wvamapper_;
    BufferString	ctab_;
    float		overlap_;
    void		fillPar(IOPar&) const;
    void		usePar(const IOPar&);
};


/*! brief the basic synthetic dataset. contains the data cubes*/
mExpClass(WellAttrib) SyntheticData : public NamedCallBacker
{
public:

    typedef int SynthID;
					~SyntheticData();

    static SyntheticData*		get(const SynthGenParams&,
					    Seis::RaySynthGenerator&);

    void				setName(const char*);

    virtual const SeisTrc*		getTrace(int seqnr) const	= 0;
    virtual int				nrPositions() const		= 0;

    float				getTime(float dpt,int seqnr) const;
    float				getDepth(float time,int seqnr) const;

    const DataPack&			getPack() const {return datapack_;}
    DataPack&				getPack()	{return datapack_;}

    const Seis::SynthGenDataPack&	synthGenDP() const;
    const ReflectivityModelSet&		getRefModels() const;
    const ReflectivityModelBase*	getRefModel(int imdl) const;
    const TimeDepthModel*		getTDModel(int imdl) const;
    const TimeDepthModel*		getTDModel(int imdl,int ioff) const;

    DataPack::FullID			datapackid_;

    SynthID				id_ = -1;
    virtual bool			isPS() const		= 0;
    virtual bool			hasOffset() const	= 0;
    virtual bool			isAngleStack() const  { return false; }
    virtual bool			isAVOGradient() const { return false; }
    virtual bool			isStratProp() const   { return false; }
    virtual bool			isAttribute() const   { return false; }
    virtual SynthGenParams::SynthType	synthType() const	= 0;

    const SynthGenParams&		getGenParams() const	{ return sgp_; }
    void				useGenParams(const SynthGenParams&);
    void				useDispPar(const IOPar&);
    void				fillDispPar(IOPar&) const;
    const char*				waveletName() const;
    SynthFVSpecificDispPars&		dispPars()	{ return disppars_; }
    const SynthFVSpecificDispPars&	dispPars() const
							{ return disppars_; }

protected:
					SyntheticData(const SynthGenParams&,
						  const Seis::SynthGenDataPack&,
						  DataPack&);

    SynthFVSpecificDispPars		disppars_;

    void				removePack();

    SynthGenParams			sgp_;
    DataPack&				datapack_;
    ConstRefMan<Seis::SynthGenDataPack> synthgendp_;
};


