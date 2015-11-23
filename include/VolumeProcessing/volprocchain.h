#ifndef volprocchain_h
#define volprocchain_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	K. Tingdahl
 Date:		October 2006
 RCS:		$Id$
________________________________________________________________________


-*/

#include "volumeprocessingmod.h"
#include "multiid.h"
#include "executor.h"
#include "factory.h"
#include "refcount.h"
#include "samplingdata.h"
#include "threadlock.h"
#include "trckeysampling.h"


class RegularSeisDataPack;
class VelocityDesc;
class Executor;
template <class T> class ValueSeries;

namespace VolProc
{

class Chain;
class StepExecutor;
class StepTask;

/*!
 \brief An algorithm/calculation/transformation that takes one scalar volume as
 input, processes it, and puts the output in another volume.
 */

mExpClass(VolumeProcessing) Step
{
public:
				typedef int ID;
				typedef int InputSlotID;
				typedef int OutputSlotID;

				mDefineFactoryInClass( Step, factory );
    virtual			~Step();

    static ID			cUndefID()		{ return mUdf(int); }
    static int			cUndefSlotID()		{ return mUdf(int); }
    ID				getID() const		{ return id_; }

    Chain&			getChain();
    const Chain&		getChain() const;
    void			setChain(Chain&);

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
				/*!<When computing TrcKeySampling, how
				 big input is needed?*/

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
    const RegularSeisDataPack*	getOutput() const	{ return output_; }
    RegularSeisDataPack*	getOutput()		{ return output_; }

    virtual const VelocityDesc*	getVelDesc() const	{ return 0; } // old

    virtual bool		areSamplesIndependent() const { return true; }
				/*!<returns whether samples in the output
				 are independent from each other.*/

    virtual Task*		createTask();

    virtual void		fillPar(IOPar&) const;
    virtual bool		usePar(const IOPar&);

    virtual void		releaseData();

    virtual uiString		errMsg() const
				{ return uiString::emptyString(); }

    void			setNeedToReportProgress( bool );
    bool			needToReportProgress() const;
    void			setProgressMeter(ProgressMeter*);
    ProgressMeter*		getProgressMeter();

protected:
				Step();

    friend		class BinIDWiseTask;
    virtual bool	prefersBinIDWise() const		{ return false;}
    virtual bool	computeBinID(const BinID&,int threadid)	{ return false;}
    virtual bool	prepareComp(int nrthreads)		{ return true;}

    Chain*				chain_;

    ObjectSet<const RegularSeisDataPack>	inputs_;
    TypeSet<InputSlotID>		inputslotids_;

    BufferString			username_;
    ID					id_;

    TrcKeySampling			tks_;
    StepInterval<int>			zrg_;
    TypeSet<OutputSlotID>		outputslotids_; // enabled slotids

private:
    RegularSeisDataPack*		output_;
};



/*!
\brief A chain of Steps that can be applied to a volume of scalars.
*/

mExpClass(VolumeProcessing) Chain
{ mRefCountImpl(Chain); mODTextTranslationClass(Chain);
public:
				Chain();

    mExpClass(VolumeProcessing) Connection
    {
    public:
			Connection( Step::ID outpstep=Step::cUndefID(),
			    Step::OutputSlotID outpslot=Step::cUndefSlotID(),
			    Step::ID inpstep=Step::cUndefID(),
			    Step::InputSlotID inpslot=Step::cUndefSlotID());

	bool		isUdf() const;
	bool		operator==(const Connection&) const;
	bool		operator!=(const Connection&) const;

	void		fillPar(IOPar&,const char* key) const;
	bool		usePar(const IOPar&,const char* key);

				//!Step that is receiving data
	Step::ID		inputstepid_;
	Step::InputSlotID	inputslotid_;

				//!Step that is sending data
	Step::ID		outputstepid_;
	Step::OutputSlotID	outputslotid_;
    };

    mExpClass(VolumeProcessing) Web
    {
    public:
	bool			addConnection(const Connection&);
	void			removeConnection(const Connection&);
	void			getConnections(Step::ID,bool input,
					       TypeSet<Connection>&) const;
				/*!Gets all connection that has step as either
				   input or output. */

	TypeSet<Connection>&		getConnections()
					{ return connections_; }
	const TypeSet<Connection>&	getConnections() const
					{ return connections_; }
    private:
	TypeSet<Connection>	connections_;
    };

    bool			addConnection(const Connection&);
    void			removeConnection(const Connection&);
    void			updateConnections();
    const Web&			getWeb() const	{ return web_; }

    void			setZStep( float z, bool zist )
				{ zstep_=z; zist_ = zist; }
    float			getZStep() const	{ return zstep_; }
    bool			zIsT() const		{ return zist_; }

    int				nrSteps() const;
    Step*			getStep(int);
    Step*			getStepFromName(const char*);
    const Step*			getStepFromName(const char*) const;
    Step*			getStepFromID(Step::ID);
    const Step*			getStepFromID(Step::ID) const;
    int				indexOf(const Step*) const;
    void			addStep(Step*);
    void			insertStep(int,Step*);
    void			swapSteps(int,int);
    void			removeStep(int);
    const ObjectSet<Step>&	getSteps() const	{ return steps_; }

    bool			setOutputSlot(Step::ID,Step::OutputSlotID);

    const VelocityDesc*		getVelDesc() const;

    void			fillPar(IOPar&) const;
    bool			usePar(const IOPar&);

    void			setStorageID(const MultiID& mid);
    const MultiID&		storageID() const { return storageid_; }
    uiString			name() const;

    bool			areSamplesIndependent() const;
    bool			needsFullVolume() const;
    uiString			errMsg() const;

    Step::ID			getNewStepID() { return freeid_++; }

private:
    friend			class ChainExecutor;

    bool			validConnection(const Connection&) const;

    static const char*		sKeyNrSteps()	    { return "Nr Steps"; }
    static const char*		sKeyStepType()	    { return "Type"; }
    static const char*		sKeyNrConnections() { return "Nr Connections"; }
    static const char*		sKeyConnection(int idx,BufferString&);

    Step::ID			outputstepid_;
    Step::OutputSlotID		outputslotid_;

    MultiID			storageid_;
    ObjectSet<Step>		steps_;
    Web				web_;

    float			zstep_;
    bool			zist_;

    uiString			errmsg_;
    Threads::Atomic<int>	freeid_;
};



/*!
\brief Chain Executor
*/

mExpClass(VolumeProcessing) ChainExecutor : public Executor
{ mODTextTranslationClass(ChainExecutor);
public:
				ChainExecutor(Chain&);
				~ChainExecutor();

    uiString			errMsg() const;
    uiString			uiNrDoneText() const;

    bool			setCalculationScope(const TrcKeySampling&,
						    const StepInterval<int>&);

    const RegularSeisDataPack*	getOutput() const;
    int				nextStep();

private:
    class Epoch
    {
    public:
				Epoch(const ChainExecutor& c)
				    : taskgroup_( *new TaskGroup )
				    , chainexec_( c )
				{
				    taskgroup_.setParallel(true);
				    taskgroup_.setName( c.name() );
				}

				~Epoch()		{ delete &taskgroup_; }

	void			addStep(Step* s)	{ steps_ += s; }

	bool			doPrepare();
	bool			doPrepareWithProgressMeter(ProgressMeter*);
	Task&			getTask()		{ return taskgroup_; }

	bool			needsStepOutput(Step::ID) const;
	const RegularSeisDataPack* getOutput() const;

    private:

	BufferString		errmsg_;
	const ChainExecutor&	chainexec_;
	TaskGroup&		taskgroup_;
	ObjectSet<Step>		steps_;
    };

    bool			scheduleWork();
    int				computeLatestEpoch(Step::ID) const;
    void			computeComputationScope(Step::ID stepid,
				    TrcKeySampling& stepoutputhrg,
				    StepInterval<int>& stepoutputzrg ) const;

    void			controlWork(Task::Control);
    od_int64			nrDone() const;
    od_int64			totalNr() const;
    uiString			uiMessage() const;

    void			releaseMemory();

    Epoch*			curepoch_;

    bool			isok_;
    Chain&			chain_;

    TrcKeySampling			outputhrg_;
    StepInterval<int>		outputzrg_;

    mutable uiString		errmsg_;
    ObjectSet<Step>		scheduledsteps_;
    ObjectSet<Epoch>		epochs_;
    Chain::Web			web_;
    int				totalnrepochs_;

    const RegularSeisDataPack*	outputvolume_;
};

} // namespace VolProc

#endif

