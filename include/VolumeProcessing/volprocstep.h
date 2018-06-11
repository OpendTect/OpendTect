#ifndef volprocstep_h
#define volprocstep_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	K. Tingdahl
 Date:		October 2006
 RCS:		$Id$
________________________________________________________________________


-*/

#include "volumeprocessingmod.h"
#include "factory.h"
#include "survgeom.h"
#include "trckeysampling.h"
#include "uistrings.h"

class Task;
class VelocityDesc;
class ProgressMeter;
class TrcKeyZSampling;
class RegularSeisDataPack;


namespace VolProc
{

class Chain;


/*!\brief An algorithm/calculation/transformation that takes one scalar volume
  as input, processes it, and puts the output in another volume.

  Every step will be part of a Chain, which will give the step its ID.

 */

mExpClass(VolumeProcessing) Step
{
public:
				typedef int ID;
				typedef int InputSlotID;
				typedef int OutputSlotID;
    static ID			cUndefID()		{ return mUdf(int); }
    static int			cUndefSlotID()		{ return mUdf(int); }

				mDefineFactoryInClass( Step, factory );
    virtual			~Step();

    ID				getID() const		{ return id_; }
    Chain&			getChain();
    const Chain&		getChain() const;
    virtual const char*		userName() const;
    virtual void		setUserName(const char* nm);

    void			resetInput();
    virtual bool		needsInput() const		= 0;
    virtual int			getNrInputs() const;
    virtual InputSlotID		getInputSlotID(int idx) const;
    virtual void		getInputSlotName(InputSlotID,
						 BufferString&) const;
    virtual bool		isInputPrevStep() const { return needsInput(); }

    virtual int			getNrOutputs() const		{ return 1; }
    virtual OutputSlotID	getOutputSlotID(int idx) const;
    bool			validInputSlotID(InputSlotID) const;
    bool			validOutputSlotID(OutputSlotID) const;

    virtual TrcKeySampling	getInputHRg(const TrcKeySampling&) const;
				/*!<When computing TrcKeySampling, how
				 big input is needed? */
    virtual StepInterval<int>	getInputZRg(const StepInterval<int>&) const;
    virtual StepInterval<int>	getInputZRgWithGeom(const StepInterval<int>&,
					    Survey::Geometry::ID) const;
				/*!<When computing Z Sampling, how
				 big input is needed? */

    virtual void		setInput(InputSlotID,
					 const RegularSeisDataPack*);
    const RegularSeisDataPack*	getInput(InputSlotID) const;
    virtual void		setOutput(OutputSlotID,RegularSeisDataPack*,
				      const TrcKeySampling&,
				      const StepInterval<int>&);
    const RegularSeisDataPack*	getOutput(OutputSlotID) const;
    RegularSeisDataPack*		getOutput(OutputSlotID);

    int				getOutputIdx(OutputSlotID) const;
    void			enableOutput(OutputSlotID);

    virtual bool		canInputAndOutputBeSame() const { return false;}
    virtual bool		needsFullVolume() const { return true; }
    virtual bool		canHandle2D() const	{ return false; }

    const RegularSeisDataPack*	getOutput() const	{ return output_; }
    RegularSeisDataPack*	getOutput()		{ return output_; }

    virtual const VelocityDesc* getVelDesc() const	{ return 0; } // old

    virtual bool		areSamplesIndependent() const { return true; }
				/*!<returns whether samples in the output
				 are independent from each other.*/

    virtual Task*		createTask();
    virtual Task*		createTaskWithProgMeter(ProgressMeter*);
    virtual bool		needReportProgress()	{ return false; }

    virtual void		fillPar(IOPar&) const;
    virtual bool		usePar(const IOPar&);

    virtual void		releaseData();
    TrcKeyZSampling		getInputSampling(const TrcKeyZSampling&) const;
    /* Given a target sampling, return the ideal sampling for the
       input datapack(s) */

    virtual uiString		errMsg() const
				{ return uiString::emptyString(); }

protected:

			Step();

    virtual bool	prefersBinIDWise() const		{ return false;}
    virtual bool	computeBinID(const BinID&,int threadid) { return false;}
    virtual bool	prepareComp(int nrthreads)		{ return true;}

    Chain*		chain_;

    // The memory needed on top of the 'base' memory usage. Can be 0.
    virtual od_int64	extraMemoryUsage(OutputSlotID,const TrcKeySampling&,
					 const StepInterval<int>&) const
				{ return 0; }

    ObjectSet<const RegularSeisDataPack> inputs_;
    TypeSet<InputSlotID>		inputslotids_;

    BufferString			username_;
    ID					id_;
    uiString				errmsg_;

    TypeSet<OutputSlotID>		outputslotids_; // enabled slotids

    const StepInterval<float>&	getZSampling() const;
    void			setHStep(const BinID&);
    void			setVStep(int);
    void			setInpNrComps(InputSlotID,int);
    void			setOutputNrComps(int);

    int				getNrInputComponents(InputSlotID) const;
    int				getNrOutComponents() const;

private:

    RegularSeisDataPack*	output_;

    void			setChain(Chain&);

    friend class		Chain;
    friend class		ChainExecutor;
    friend class		BinIDWiseTask;

    static od_uint64		getBaseMemoryUsage(const TrcKeyZSampling&);

    StepInterval<float>		getInputZSamp(const StepInterval<float>&) const;


    //For 6.2 only (HiddenParam set):
public:

    mDeprecated virtual od_int64 getOuputMemSize(int) const;
    mDeprecated virtual od_int64 getProcTimeExtraMemory() const { return 0; }

    mDeprecated static od_int64 getBaseMemoryUsage(const TrcKeySampling&,
						   const StepInterval<int>&);
protected:
    od_uint64			getComponentMemory(const TrcKeySampling&,
						   bool input) const;

    /*mDeprecated*/ TrcKeySampling	tks_;
    /*mDeprecated*/ StepInterval<int>	zrg_;

private:
    od_uint64			extraMemoryUsage(OutputSlotID,
						 const TrcKeyZSampling&) const;
    od_uint64			getComponentMemory(const TrcKeyZSampling&,
						   bool input) const;

};

} // namespace VolProc

#endif
