#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bruno
 Date:		July 2011
________________________________________________________________________

-*/

#include "wellattribmod.h"

#include "ailayer.h"
#include "elasticpropsel.h"
#include "stratsynthlevel.h"
#include "synthseis.h"
#include "syntheticdata.h"
#include "uistring.h"

namespace PreStack {  class GatherSetDataPack; }
class SeisTrcBuf;
class TaskRunner;
class TrcKeyZSampling;
class PostStackSyntheticData;
class PreStackSyntheticData;

namespace Strat
{
    class LayerModel; class LayerModelSuite; class Level;
}

namespace StratSynth
{

class PropertyDataSetsCreator;
class StratSeqSplitter;


/*!\brief manages the synthetics for a set of Layer Models.

 The 'key' to adding synthetics is the SynthGenParams object. Adding
 a synthetic means adding one of those. Actually generating the seismics will
 require a call of 'ensureGenerated()' for that DataSet ID. This makes sure
 that the DataSet for the current layer model is available.

 Note that this class manages the DataSet's for the multiple LayerModel's in
 a LayerModelSuite. The LayerModelSuite has the notion of a 'current'
 LayerModel, which is why you can but do not have to specify the LayerModel
 index for all DataSet-related operations (int curlmidx=-1).

*/


mExpClass(WellAttrib) DataMgr : public CallBacker
{ mODTextTranslationClass(DataMgr);
public:
			DataMgr(const Strat::LayerModelSuite&);
			DataMgr(const DataMgr&,int calceach);
			~DataMgr();

    bool		isEmpty() const;

    void		setEmpty()		{ clearData(true,true); }
    void		modelChange()		{ clearData(true,false); }
    void		synthChange()		{ clearData(false,true); }

    void		setCalcEach(int);
    int			calcEach() const	{ return calceach_; }
    DataMgr*		getProdMgr();		//!< may return this
    const DataMgr*	getProdMgr() const;	//!< may return this

    int			nrSequences(int curlmidx=-1) const; //!< all available
    int			nrTraces(int curlmidx=-1) const;  //!< actual calculated
    bool		needSWave() const;

    bool		ensureGenerated(SynthID,TaskRunner* =nullptr,
					int curlmidx=-1) const;
    bool		ensureInstantAttribsDataSet(const TypeSet<SynthID>&,
					TaskRunner* =nullptr,
					int curlmidx=-1) const;
			//!< Generates all attribs from a same input synthetic
    bool		ensurePropertyDataSets(TaskRunner* =nullptr,
					int lmsidx=-1,double zstep=0.001) const;
			//!< Generates all properties, or a subselection

    SynthID		addSynthetic(const SynthGenParams&);
    bool		addPropertySynthetics(TypeSet<SynthID>* =nullptr,
				      const BufferStringSet* propnms =nullptr);
			/* Adds all properties, or a subselection
			     Can be overruled by the environment variable
			     DTECT_SYNTHROCK_TIMEPROPS
			  Example: export DTECT_SYNTHROCK_TIMEPROPS=Density|Phie
			  Set to None to disable them all
			 */
    bool		addInstAttribSynthetics(const SynthID& inpid,
				const TypeSet<Attrib::Instantaneous::OutType>&,
				TypeSet<SynthID>&);
    bool		updateSynthetic(SynthID,const SynthGenParams&);
    bool		updateWavelet(const MultiID& oldwvltid,
				      const MultiID& newwvltid);
    bool		updateSyntheticName(SynthID,const char* newnm);
    bool		removeSynthetic(SynthID);
    bool		disableSynthetic(const TypeSet<SynthID>&);

    SynthID		getIDByIdx(int) const;
    SynthID		find(const char* nm,int* lmsidx =nullptr) const;
    SynthID		find(const PropertyRef&,bool require_generated=false,
			     int lmsidx=-1) const;
    SynthID		first(bool prestack,bool require_generated=false,
			      int lmsidx=-1) const;
    bool		isPS(SynthID) const;
    bool		isAttribute(SynthID) const;
    bool		isStratProp(SynthID) const;

    enum SubSelType	{ NoSubSel, OnlyZO, NoZO, OnlyPS, NoPS, OnlyEI, NoEI,
			  OnlyPSBased, NoPSBased, OnlyAttrib, NoAttrib,
			  OnlyRaw, NoRaw, OnlyInput, OnlyWithInput, NoWithInput,
			  OnlyProps, NoProps };
    void		getNames(BufferStringSet&,SubSelType t=NoSubSel,
				 bool omitempty=false,int lmsidx=-1) const;
    void		getNames(const MultiID& wvltid,BufferStringSet&,
				 bool omitempty=false,int lmsidx=-1) const;
    void		getIDs(TypeSet<SynthID>&,SubSelType t=NoSubSel,
			       bool omitempty=false,int lmsidx=-1) const;
    void		getIDs(const MultiID& wvltid,TypeSet<SynthID>&,
			       bool omitempty=false,int lmsidx=-1) const;
    bool		haveOfType(SynthGenParams::SynthType) const;

    const SynthGenParams* getGenParams(SynthID) const;
    BufferString	nameOf(SynthID) const;
    int			lmsIndexOf(const SyntheticData*) const;
    int			indexOf(const SyntheticData*,int lmsidx=-1) const;
    bool		hasValidDataSet(SynthID,int lmsidx=-1) const;
    ConstRefMan<SyntheticData> getDataSet(SynthID,int lmsidx=-1) const;
    ConstRefMan<SyntheticData> getDataSetByIdx(int,int lmsidx=-1) const;

    const TypeSet<SynthGenParams>& genParams() const	{ return genparams_; }
    const Strat::LayerModelSuite& layerModelSuite() const	{ return lms_; }
    const Strat::LayerModel& layerModel(int lmsidx=-1) const;
    const TypeSet<ElasticModel>& elasticModels(int lmsidx=-1) const;
    const StratSynth::LevelSet& levels(int lmsidx=-1) const;

    void		getLevelDepths(Strat::LevelID,TypeSet<float>& zvals,
				       int lmsidx=-1) const;
    void		setPackLevelTimes(SynthID,Strat::LevelID) const;

    uiRetVal		errMsg() const		{ return errmsg_; }
    uiRetVal		infoMsg() const		{ return infomsg_; }
    void		clearInfoMsg()		{ infomsg_.setOK(); }

    void		fillPar(IOPar&,
			       const ObjectSet<IOPar>* disppars =nullptr) const;
    bool		usePar(const IOPar&);
    bool		setElasticProperties(const IOPar&,
					     uiString* msg=nullptr);
			//!< Will set a default if none found

    CNotifier<DataMgr,SynthID> entryAdded;
    CNotifier<DataMgr,const TypeSet<SynthID>&> entryRenamed;
    CNotifier<DataMgr,const TypeSet<SynthID>&> entryChanged;
    CNotifier<DataMgr,const TypeSet<SynthID>&> entryDisabled;
    CNotifier<DataMgr,const TypeSet<SynthID>&> entryRemoved;
    Notifier<DataMgr>	elPropSelChanged;
    CNotifier<DataMgr,const MultiID&> newWvltUsed;
    CNotifier<DataMgr,const BufferStringSet&>	wvltScalingDone;

    DirtyCountType	dirtyCount() const	{ return dirtycount_; }
    void		touch() const		{ dirtycount_++; }
    void		kick();

    BufferString	getFinalDataSetName(const char* gpnm,bool isprop=false,
					    int lmsidx=-1) const;
    bool		getUnscaledSynthetics(
					RefObjectSet<const SyntheticData>*,
					TypeSet<MultiID>* unscaledwvlts,
					int lmsidx=-1) const;
			//!< return true if unscaled synthetics are found
    bool		checkElasticPropSel(const ElasticPropSelection&,
					    const bool* checkswave =nullptr,
					    uiString* msg=nullptr) const;

    static bool		getAllGenPars(const IOPar&,ObjectSet<SynthGenParams>&);

    static const char*	sKeySynthetics()	{ return "Synthetics"; }
    static const char*	sKeySyntheticNr()	{ return "Synthetics Nr"; }
    static const char*	sKeyNrSynthetics()	{ return "Nr of Synthetics"; }

private:

				// input
    const Strat::LayerModelSuite& lms_;
    int				calceach_	= 1;

				// data
    TypeSet<SynthID>		ids_;
    TypeSet<SynthGenParams>	genparams_;
    mutable DirtyCounter	dirtycount_;

				// generated / 'caches'
    ObjectSet<RefObjectSet<const SyntheticData> > lmdatasets_;
    ObjectSet<TypeSet<ElasticModel> > elasticmodelsets_;
    ObjectSet<StratSynth::LevelSet> levelsets_;

    PtrMan<DataMgr>	prodmgr_;

    void		clearData(bool lmdata,bool synthdata);
    void		ensureLevels(int lmsidx) const;
    void		setElasticPropSel(const ElasticPropSelection&);
    bool		ensureAdequatePropSelection(int lmsidx,
						    bool checkswave) const;
    bool		ensureElasticModels(int lmsidx,bool checkswave,
					    TaskRunner*) const;
    bool		adjustElasticModel(const Strat::LayerModel&,
					   TypeSet<ElasticModel>&,bool chksvel,
					   TaskRunner*) const;
    bool		checkNeedsInput(const SynthGenParams&) const;

    bool		generate(SynthID,int lmsidx,TaskRunner*) const;
    ConstRefMan<SyntheticData> generateDataSet(const SynthGenParams&,int lmsidx,
					       TaskRunner*) const;
    bool		runSynthGen(Seis::RaySynthGenerator&,
				    const SynthGenParams&,TaskRunner*) const;
    ConstRefMan<SyntheticData> genPSPostProcDataSet(
						const PreStackSyntheticData&,
						const SynthGenParams&,
						TaskRunner*) const;
    ConstRefMan<SyntheticData> createAttribute(const PostStackSyntheticData&,
					 const SynthGenParams&,
					 TaskRunner*) const;
    void		createAngleData(PreStackSyntheticData&,
					TaskRunner*) const;

    const ReflectivityModelSet* getRefModels(const SynthGenParams&,
					      int lmsidx) const;
    const Seis::SynthGenDataPack* getSynthGenRes(const SynthGenParams&,
						 int lmsidx) const;
    ConstRefMan<PreStack::GatherSetDataPack>	getRelevantAngleData(
				const Seis::SynthGenDataPack&,int lmsidx) const;

    int			nrLayerModels() const;
    int			curLayerModelIdx() const;
    void		addLayModelSets(bool withmod=true);
    int			gtActualLMIdx(int lmsidx) const;
    ObjectSet<const SyntheticData>& gtDSS(int lmsidx);
    const ObjectSet<const SyntheticData>& gtDSS(int lmsidx) const;
    int			iSeq(int itrc) const;
    int			iTrc(int iseq) const;
    SynthID		addEntry(SynthID,const SynthGenParams&);
    void		getAllNames(const SynthGenParams&,int lmsidx,
				    BufferStringSet&) const;
    int			gtIdx(SynthID) const;
    int			gtGenIdx(SynthID,TaskRunner*) const;
			//<! same as gtIdx, used in ensureGenerated* funcs
    const SyntheticData* gtDS(SynthID,int lmsidx) const;
    const SyntheticData* gtDSByIdx(int idx,int lmsidx) const;
    const SyntheticData* gtDSByName(const char*,int lmsidx) const;
    void		gtIdxs(TypeSet<int>&,SubSelType,bool,int lmsidx) const;
    void		gtIdxs(const MultiID&,TypeSet<int>&,
			       bool,int lmsidx) const;
    void		lmsEdChgCB(CallBacker*);
    bool		haveDS(int,int lmsidx) const;

    friend class	StratSeqSplitter;
    friend class	PropertyDataSetsCreator;
    void		setDataSet(const SynthGenParams&,const SyntheticData*,
				   int lmsidx);

    const Wavelet*	wvlt_ = nullptr;

    mutable uiRetVal	errmsg_;
    mutable uiRetVal	infomsg_;
    mutable bool	swaveinfomsgshown_ = false;

			mOD_DisableCopy( DataMgr );
public:

    void		setWavelet( const Wavelet& wvlt )
			{ wvlt_ = &wvlt; }
			/*<! Mainly for test programs, to use a default wavelet
			     not registered in the database */

};

} // namespace StratSynth
