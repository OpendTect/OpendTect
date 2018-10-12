#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bruno
 Date:		July 2011
________________________________________________________________________

-*/

#include "wellattribmod.h"

#include "atomic.h"
#include "elasticpropsel.h"
#include "raysynthgenerator.h"
#include "stratsynthlevel.h"
#include "uistring.h"

class GatherSetDataPack;
class RaySynthGenerator;
class RayTracer1D;
class SeisTrcBuf;
class TaskRunnerProvider;
class TimeDepthModelSet;
class TrcKeyZSampling;
class Wavelet;
namespace Strat { class LayerModel; class LayerModelSuite; }
namespace SynthSeis { class PreStackDataSet; class PostStackDataSet; }


namespace StratSynth
{

class PropertyDataSetsCreator;


/*!\brief manages the synthetics for a set of Layer Models.

 The 'key' to adding synthetics is the SynthSeis::GenParams object. Adding
 a synthetic means adding one of those. Actually generating the seismics will
 require a call of 'ensureGenerated()' for that DataSet ID. This makes sure
 that the DataSet for the current layer model is available.

 Note that this class manages the DataSet's for the multiple LayerModel's in
 a LayerModelSuite. The LayerModelSuite has the notion of a 'current'
 LayerModel, which is why you can but do not have to specify the LayerModel
 index for all DataSet-related operations - see mCurLMIdx.

  */

mExpClass(WellAttrib) DataMgr	: public RefCount::Referenced
				, public CallBacker
{ mODTextTranslationClass(StratSynth::DataMgr);
public:

    typedef SynthSeis::DataSet		DataSet;
    typedef SynthSeis::SyntheticType	SynthType;
    typedef DataSet::MgrID		SynthID;
    typedef TypeSet<SynthID>		SynthIDSet;
    typedef Strat::LayerModel		LayerModel;
    typedef Strat::LayerModelSuite	LayerModelSuite;
    typedef Level::ID			LevelID;
    typedef Level::ZValueSet		ZValueSet;
    typedef SynthSeis::GenParams	GenParams;
    typedef ObjectSet<DataSet>		DataSetSet;
    typedef TypeSet<GenParams>		GenParamSet;
    typedef TaskRunnerProvider		TRProv;
    typedef SynthIDSet::size_type	size_type;
    typedef SynthIDSet::idx_type	idx_type;
    typedef idx_type			lms_idx_type;

#   define mCurLMIdx	lms_idx_type lmsidx=-1
#   define mAllLMIdxs	lms_idx_type lmsidx=-1

			DataMgr(const LayerModelSuite&);
			DataMgr(const DataMgr&);
    bool		haveEdited() const;

    void		setEmpty()		{ clearData(true); }
    void		modelChange()		{ clearData(false); }

    void		setCalcEach(size_type);
    size_type		calcEach() const	{ return calceach_; }
    RefMan<DataMgr>	getProdMgr();		//!< may return this
    ConstRefMan<DataMgr> getProdMgr() const;	//!< may return this

    size_type		nrSequences(mCurLMIdx) const; //!< all available
    size_type		nrTraces(mCurLMIdx) const;    //!< actual calculated

    bool		ensureGenerated(SynthID,const TRProv&,mCurLMIdx) const;

    size_type		nrSynthetics() const	{ return ids_.size(); }
    SynthID		addSynthetic(const GenParams&);
    SynthID		setSynthetic(SynthID,const GenParams&);
    void		removeSynthetic(SynthID);

				// convenience: shortcuts into GenParams
    void		setWavelet(SynthID,const DBKey&);
				//!< renames dataset, returns like setSynthetic
    DBKey		waveletID(SynthID) const;

    SynthID		getIDByIdx(idx_type) const;
    SynthID		find(const char* nm) const;
    SynthID		find(const PropertyRef&,mCurLMIdx) const;
    SynthID		first(bool prestack,bool require_generated=false,
				mCurLMIdx) const;
    bool		isPS(SynthID) const;
    bool		isStratProp(SynthID) const;

    enum SubSelType	{ NoSubSel, OnlyZO, NoZO,
			  NoPS, OnlyPS, NoProps, OnlyProps };
    void		getNames(BufferStringSet&,SubSelType t=NoSubSel,
				 bool omitempty=false,mCurLMIdx) const;
    void		getIDs(SynthIDSet&,SubSelType t=NoSubSel,
			       bool omitempty=false,mCurLMIdx) const;
    bool		haveOfType(SynthType) const;

    const GenParams&	getGenParams(SynthID) const;
    BufferString	nameOf(SynthID) const;
    lms_idx_type	lmsIndexOf(const DataSet*) const;
    idx_type		indexOf(const DataSet*,mCurLMIdx) const;
    bool		hasValidDataSet(SynthID,mCurLMIdx) const;
    DataSet*		getDataSet(SynthID,mCurLMIdx);
    const DataSet*	getDataSet(SynthID,mCurLMIdx) const;
    DataSet*		getDataSetByIdx(idx_type,mCurLMIdx);
    const DataSet*	getDataSetByIdx(idx_type,mCurLMIdx) const;

    const GenParamSet&	genParams() const	{ return genparams_; }
    const LayerModelSuite& layerModelSuite() const { return lms_; }
    const LayerModel&	layerModel(mCurLMIdx) const;
    const ElasticModelSet& elasticModels(mCurLMIdx) const;
    const TimeDepthModelSet& d2TModels(mCurLMIdx) const;
    const LevelSet&	levels(mCurLMIdx) const;
    const DataSetSet&	synthetics(mCurLMIdx) const { return gtDSS(lmsidx); }

    void		getLevelDepths(LevelID,ZValueSet&,mCurLMIdx) const;
    void		setPackLevelTimes(SynthID,LevelID) const;

    BufferString	getFinalDataSetName(const char* gpnm,
					    bool isprop=false) const;

    void		fillPar(IOPar&) const;
    void		usePar(const IOPar&);

    mutable CNotifier<DataMgr,SynthID>	    entryChanged;

    DirtyCountType	dirtyCount() const  { return dirtycount_; }
    void		touch() const	    { dirtycount_++; }
    void		kick()
			{ dirtycount_++; entryChanged.trigger(SynthID()); }

protected:

			~DataMgr();

				// input
    const LayerModelSuite&	lms_;
    size_type			calceach_		= 1;

				// data
    SynthIDSet			ids_;
    GenParamSet			genparams_;
    mutable DirtyCounter	dirtycount_;

				// generated / 'caches'
    ObjectSet<DataSetSet>	lmdatasets_;
    ObjectSet<ElasticModelSet>	elasticmodelsets_;
    ObjectSet<LevelSet>		levelsets_;

    void		clearData(bool);
    void		ensureLevels(lms_idx_type) const;
    bool		ensureElasticModels(const TRProv&,lms_idx_type) const;
    void		ensurePropertyDataSets(const TRProv&,
						lms_idx_type) const;
    bool		generate(SynthID,const TRProv&,lms_idx_type) const;
    DataSet*		generateDataSet(const GenParams&,const TRProv&,
					lms_idx_type) const;
    DataSet*		genRaySynthDataSet(const GenParams&,const TRProv&,
					   lms_idx_type) const;
    DataSet*		genPSPostProcDataSet(const GenParams&,const DataSet&,
				const TrcKeyZSampling&,const TRProv&) const;

    size_type		nrLayerModels() const;
    idx_type		curLayerModelIdx() const;
    void		addLayModelSets();
    void		addStratPropSynths();
    lms_idx_type	gtActualLMIdx(lms_idx_type) const;
    DataSetSet&		gtDSS(lms_idx_type);
    const DataSetSet&	gtDSS(lms_idx_type) const;
    idx_type		iSeq(idx_type itrc) const;
    idx_type		iTrc(idx_type iseq) const;
    SynthID		addEntry(SynthID,const GenParams&);
    idx_type		gtIdx(SynthID) const;
    DataSet*		gtDS(SynthID,lms_idx_type) const;
    DataSet*		gtDSByIdx(idx_type,lms_idx_type) const;
    DataSet*		gtDSByName(const char*,lms_idx_type) const;
    idx_type		gtGenIdx(SynthID,const TRProv&) const;
    void		gtIdxs(TypeSet<idx_type>&,SubSelType,bool,
				lms_idx_type) const;
    void		lmsEdChgCB(CallBacker*);
    bool		haveDS(idx_type,lms_idx_type) const;

    friend class	PropertyDataSetsCreator;
    void		setDataSet(const GenParams&,DataSet*,lms_idx_type);

private:

    DataMgr&		operator =(const DataMgr&)  = delete;

};

} // namespace StratSynth
