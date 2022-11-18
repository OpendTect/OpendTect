#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "attributeenginemod.h"

#include "attribdescid.h"
#include "attribsel.h"
#include "bufstring.h"
#include "posgeomid.h"
#include "ranges.h"
#include "sets.h"
#include "uistring.h"

class BinIDValueSet;
class BufferStringSet;
class DataPackMgr;
class TrcKeyZSampling;
class DataPointSet;
class Executor;
class LineKey;
class NLAModel;
class RegularSeisDataPack;
class SeisTrcBuf;
class SeisTrcInfo;

namespace Attrib
{
class SeisTrcStorOutput;
class Desc;
class DescSet;
class Processor;
class Data2DHolder;

/*!
\brief The attribute engine manager.
*/

mExpClass(AttributeEngine) EngineMan
{ mODTextTranslationClass(Attrib::EngineMan);
public:
			EngineMan();
    virtual		~EngineMan();

    Processor*		usePar(const IOPar&,DescSet&,
			       const char* linename,uiString&);

    static Processor*	createProcessor(const DescSet&,const char*,
					const DescID&,uiString& errmsg);
    static bool		getPossibleVolume(DescSet&,TrcKeyZSampling&,
					  const char* linename,const DescID&);
    static void		addNLADesc(const char*,DescID&,DescSet&,int,
				   const NLAModel*,uiString&);

    SeisTrcStorOutput*	createOutput(const IOPar&,const LineKey&,uiString&);

    const DescSet*	attribSet() const	{ return inpattrset_; }
    const NLAModel*	nlaModel() const	{ return nlamodel_; }
    const TrcKeyZSampling&	cubeSampling() const	{ return tkzs_; }
    Pos::GeomID		getGeomID() const	{ return geomid_; }
    float		undefValue() const	{ return udfval_; }

    void		setAttribSet(const DescSet*);
    void		setNLAModel(const NLAModel*);
    void		setAttribSpec(const SelSpec&);
    void		setAttribSpecs(const TypeSet<SelSpec>&);
    void		setTrcKeyZSampling(const TrcKeyZSampling&);
    void		setGeomID( const Pos::GeomID geomid )
			{ geomid_ = geomid; }
    void		setUndefValue( float v )	{ udfval_ = v; }
    DescSet*		createNLAADS(DescID& outid,uiString& errmsg,
				     const DescSet* addtoset=0);
    static DescID	createEvaluateADS(DescSet&, const TypeSet<DescID>&,
					  uiString&);

    Processor*		createDataPackOutput(uiString& errmsg,
				      const RegularSeisDataPack* cached_data=0);
			//!< Give the previous calculated data in cached data
			//!< and some parts may not be recalculated.

    RefMan<RegularSeisDataPack> getDataPackOutput(const Processor&);
    RefMan<RegularSeisDataPack> getDataPackOutput(
				   const ObjectSet<const RegularSeisDataPack>&);

    Executor*		createFeatureOutput(const BufferStringSet& inputs,
					    const ObjectSet<BinIDValueSet>&);

    Processor*		createScreenOutput2D(uiString& errmsg,
					     Data2DHolder&);
    Processor*		createLocationOutput(uiString& errmsg,
					     ObjectSet<BinIDValueSet>&);

    Processor*		createTrcSelOutput(uiString& errmsg,
				   const BinIDValueSet& bidvalset,
				   SeisTrcBuf&, float outval=0.f,
				   const Interval<float>* cubezbounds=nullptr,
				   const TypeSet<BinID>* trueknotspos=nullptr,
				   const TypeSet<BinID>* path=nullptr);
    Processor*		create2DVarZOutput(uiString& errmsg,
				   const IOPar& pars,
				   DataPointSet* bidvalset,
				   float outval=0.f,
				   const Interval<float>* cubezbounds =nullptr);
    Processor*		getTableOutExecutor(DataPointSet& datapointset,
					    uiString& errmsg,
					    int firstcol);
    Executor*		getTableExtractor(DataPointSet&,const Attrib::DescSet&,
					  uiString& errmsg,int firstcol =0,
					  bool needprep=true);
    static bool		ensureDPSAndADSPrepared(DataPointSet&,
						const Attrib::DescSet&,
						uiString& errmsg);
    int			getNrOutputsToBeProcessed(const Processor&) const;

    const char*		getCurUserRef() const;
    void		computeIntersect2D(ObjectSet<BinIDValueSet>&) const;

protected:

    const DescSet*	inpattrset_ = nullptr;
    const NLAModel*	nlamodel_ = nullptr;
    TrcKeyZSampling&	tkzs_;
    float		udfval_ = mUdf(float);
    Pos::GeomID		geomid_;
    DataPackMgr&	dpm_;

    ConstRefMan<RegularSeisDataPack>	cache_;

    DescSet*		procattrset_ = nullptr;
    int			curattridx_ = 0;
    TypeSet<SelSpec>	attrspecs_;

    Processor*		getProcessor(uiString& err);
    void		setExecutorName(Executor*);

private:

    friend class	AEMFeatureExtracter;//TODO will soon be removed
    friend class	AEMTableExtractor;

public:
    bool		hasCache() const		{ return cache_; }
};

} // namespace Attrib
