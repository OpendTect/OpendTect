#pragma once
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        H.Payraudeau
 Date:          04/2005
________________________________________________________________________

-*/

#include "attribdescid.h"
#include "attribsel.h"
#include "uistringset.h"
#include "fullsubsel.h"

class BinnedValueSet;
class BufferStringSet;
class DataPackMgr;
class DataPointSet;
class Executor;
class NLAModel;
class RegularSeisDataPack;
class SeisTrcBuf;
class SeisTrcInfo;

namespace Attrib
{
class SeisTrcStorOutput;
class Processor;
class Data2DHolder;

/*!\brief The attribute engine manager. */

mExpClass(AttributeEngine) EngineMan
{ mODTextTranslationClass(Attrib::EngineMan);
public:

    mUseType( Pos, GeomID );
    mUseType( Survey, FullSubSel );
    mUseType( Survey, GeomSubSel );

			EngineMan();
    virtual		~EngineMan();

    static Processor*	createProcessor(const DescSet&,const DescID&,
					uiRetVal&,GeomID gid=GeomID());
    static uiRetVal	getPossibleSubSel(DescSet&,const DescID&,FullSubSel&,
					  GeomID gid=GeomID());
    static void		addNLADesc(const char*,DescID&,DescSet&,int,
				   const NLAModel*,uiRetVal&);

    bool		is2D() const;
    Processor*		usePar(const IOPar&,DescSet&,
			       uiRetVal&,int outputidx,GeomID gid=GeomID());

    SeisTrcStorOutput*	createOutput(const IOPar&,uiRetVal&,int outidx=0);

    const DescSet*	attribSet() const	{ return attrset_; }
    const NLAModel*	nlaModel() const	{ return nlamodel_; }
    const FullSubSel&	subSel() const		{ return reqss_; }
    GeomID		geomID() const		{ return reqss_.geomID(0); }
    float		undefValue() const	{ return udfval_; }

    void		setAttribSet(const DescSet*);
    void		setNLAModel(const NLAModel*);
    void		setAttribSpec(const SelSpec&);
    void		setAttribSpecs(const SelSpecList&);
    void		setSubSel(const FullSubSel&);
    void		setSubSel(const GeomSubSel&);
    void		setGeomID(GeomID);	// only actually useful for 2D
    void		setUndefValue( float v )	 { udfval_ = v; }
    DescSet*		createNLAADS(DescID& outid,uiRetVal&,
				     const DescSet* addtoset=0);
    static DescID	createEvaluateADS(DescSet&,const TypeSet<DescID>&,
					  uiRetVal&);

    Processor*		createDataPackOutput(uiRetVal&,
				      const RegularSeisDataPack* cached_data=0);
			//!< Give the previous calculated data in cached data
			//!< and some parts may not have to be recalculated.
			//!< is used for many regular visualization elements.

    RefMan<RegularSeisDataPack> getDataPackOutput(const Processor&);
    RefMan<RegularSeisDataPack> getDataPackOutput(
				   const ObjectSet<const RegularSeisDataPack>&);

    Executor*		createFeatureOutput(const BufferStringSet& inputs,
					    const ObjectSet<BinnedValueSet>&);

    Processor*		createScreenOutput2D(uiRetVal&,Data2DHolder&);
    Processor*		createLocationOutput(uiRetVal&,
					     ObjectSet<BinnedValueSet>&);

    Processor*		createTrcSelOutput(uiRetVal&,
					   const BinnedValueSet& bidvalset,
					   SeisTrcBuf&, float outval=0,
					   const Interval<float>* cubezbounds=0,
					   const TypeSet<BinID>* trueknotspos=0,
					   const TypeSet<BinID>* path=0);
    Processor*		create2DVarZOutput(uiRetVal&,
					   const IOPar& pars,
					   DataPointSet* bidvalset,
					   float outval=0,
					   Interval<float>* cubezbounds = 0);
    Processor*		getTableOutExecutor(DataPointSet& datapointset,
					    uiRetVal& errmsg,
					    int firstcol);
    Executor*		getTableExtractor(DataPointSet&,const Attrib::DescSet&,
					  uiRetVal& errmsg,int firstcol =0,
					  bool needprep=true);
    static bool		ensureDPSAndADSPrepared(DataPointSet&,
						const Attrib::DescSet&,
						uiRetVal& errmsg);
    int			getNrOutputsToBeProcessed(const Processor&) const;

    const char*		getCurUserRef() const;
    void		computeIntersect2D(ObjectSet<BinnedValueSet>&) const;

    bool		hasCache() const		{ return cache_; }

protected:

    const DescSet*	attrset_;
    const NLAModel*	nlamodel_;
    FullSubSel		reqss_;
    float		udfval_;
    DataPackMgr&	dpm_;

    const RegularSeisDataPack*	cache_;

    DescSet*		procattrset_;
    int			curattridx_;
    SelSpecList		attrspecs_;

    Processor*		getProcessor(uiRetVal& err);
    void		setExecutorName(Executor*);

private:

    friend class	AEMFeatureExtracter;
    friend class	AEMTableExtractor;

};

} // namespace Attrib
