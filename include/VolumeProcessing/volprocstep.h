#ifndef volprocstep_h
#define volprocstep_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	K. Tingdahl
 Date:		October 2006
________________________________________________________________________


-*/

#include "volumeprocessingmod.h"
#include "factory.h"
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

    typedef ConstRefMan<RegularSeisDataPack> CVolRef;
    typedef RefMan<RegularSeisDataPack> VolRef;
    typedef int			ID;
    typedef int			InputSlotID;
    typedef int			OutputSlotID;
    static ID			cUndefID()		{ return mUdf(int); }
    static int			cUndefSlotID()		{ return mUdf(int); }

				mDefineFactoryInClass( Step, factory );
    virtual			~Step();
    virtual void		releaseData();

    ID				getID() const		{ return id_; }
    Chain&			getChain();
    const Chain&		getChain() const;
    virtual const char*		userName() const;
    virtual void		setUserName(const char* nm);

    void			resetInput();
    virtual int			getNrInputs() const;
    bool			validInputSlotID(InputSlotID) const;
    virtual InputSlotID		getInputSlotID(int idx) const;
    virtual void		getInputSlotName(InputSlotID,
						 BufferString&) const;

    virtual int			getNrOutputs() const		{ return 1; }
    bool			validOutputSlotID(OutputSlotID) const;
    virtual OutputSlotID	getOutputSlotID(int idx) const;

    virtual TrcKeySampling	getInputHRg(const TrcKeySampling&) const;
				/*!<When computing TrcKeySampling, how
				     big input is needed? */
    virtual StepInterval<int>	getInputZRg(const StepInterval<int>&) const;
				/*!<When computing Z Sampling, how
				     big input is needed?*/

    virtual void		setInput(InputSlotID,
					 const RegularSeisDataPack*);
    CVolRef			getInput(InputSlotID) const;
    virtual void		setOutput(OutputSlotID,RegularSeisDataPack*,
				      const TrcKeySampling&,
				      const StepInterval<int>&);
    CVolRef			getOutput(OutputSlotID) const;
    VolRef			getOutput(OutputSlotID);

    int				getOutputIdx(OutputSlotID) const;
    void			enableOutput(OutputSlotID);

    CVolRef			getOutput() const	{ return output_; }
    VolRef			getOutput()		{ return output_; }

    static od_int64		getBaseMemoryUsage(const TrcKeySampling&,
						   const StepInterval<int>&);
    od_int64                    getExtraMemoryUsage(const TrcKeySampling&,
					const StepInterval<int>&,
					const TypeSet<OutputSlotID>&
					=TypeSet<OutputSlotID>()) const;
				/*!< returns total amount of bytes needed
				     on top of the base consumption */

    virtual const VelocityDesc*	getVelDesc() const		{ return 0; }

    virtual Task*		createTask();
    virtual Task*		createTaskWithProgMeter(ProgressMeter*);
    				//!< only called when needReportProgress()
    virtual uiString		errMsg() const
				{ return uiString::emptyString(); }

    virtual void		fillPar(IOPar&) const;
    virtual bool		usePar(const IOPar&);

    				// Processing properties
    virtual bool		needsFullVolume() const		= 0;
    virtual bool		canInputAndOutputBeSame() const	= 0;
    virtual bool		areSamplesIndependent() const	= 0;
    virtual bool		needsInput() const		{ return true; }
    virtual bool		prefersBinIDWise() const	{ return false;}
    virtual bool		needReportProgress()		{ return false;}
    virtual bool		isInputPrevStep() const
    				{ return needsInput(); }
protected:

				Step();

    // Only called if prefersBinIDWise() returns true
    virtual bool		prepareComp(int nrthreads)	{ return true;}
    virtual bool		computeBinID(const BinID&,int threadid)
    								{ return false;}

    // The memory needed on top of the 'base' memory usage. Can be 0.
    virtual od_int64		extraMemoryUsage(OutputSlotID,
						 const TrcKeySampling&,
						 const StepInterval<int>&) const
    								= 0;

    ID				id_;
    Chain*			chain_;
    BufferString		username_;
    ObjectSet<const RegularSeisDataPack> inputs_;
    TypeSet<InputSlotID>	inputslotids_;
    uiString			errmsg_;
    TrcKeySampling		tks_;
    StepInterval<int>		zrg_;
    TypeSet<OutputSlotID>	outputslotids_; // enabled slotids

private:

    RefMan<RegularSeisDataPack>	output_;

    void			setChain(Chain&);

    friend class		Chain;
    friend class		ChainExecutor;
    friend class		BinIDWiseTask;

};


} // namespace VolProc

#endif
