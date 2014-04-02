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
#include "attribdatacubes.h"
#include "multiid.h"
#include "executor.h"
#include "factory.h"
#include "refcount.h"
#include "samplingdata.h"
#include "threadlock.h"

namespace Attrib { class DataCubes; }

class VelocityDesc;
class Executor;
class HorSampling;
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

    Chain&			getChain()		{ return *chain_; }
    void			setChain(Chain&);

    virtual const char*		userName() const;
    virtual void		setUserName(const char* nm);

    void			resetInput();
    virtual bool		needsInput() const		= 0;
    virtual int			getNrInputs() const;
    virtual InputSlotID		getInputSlotID(int idx) const;
    virtual void		getInputSlotName(InputSlotID,
						 BufferString&) const;

    virtual int			getNrOutputs() const		{ return 1; }
    virtual OutputSlotID	getOutputSlotID(int idx) const;
    bool			validInputSlotID(InputSlotID) const;
    bool			validOutputSlotID(OutputSlotID) const;

    virtual HorSampling		getInputHRg(const HorSampling&) const;
				/*!<When computing HorSampling, how
				 big input is needed? */
    virtual StepInterval<int>	getInputZRg(const StepInterval<int>&) const;
				/*!<When computing HorSampling, how
				 big input is needed?*/

    virtual void		setInput(InputSlotID,const Attrib::DataCubes*);
    const Attrib::DataCubes*	getInput(InputSlotID) const;
    virtual void		setOutput(OutputSlotID,Attrib::DataCubes*,
				      const HorSampling&,
				      const StepInterval<int>&);
    const Attrib::DataCubes*	getOutput(OutputSlotID) const;
    Attrib::DataCubes*		getOutput(OutputSlotID);

    int				getOutputIdx(OutputSlotID) const;
    void			enableOutput(OutputSlotID);

    virtual bool		canInputAndOutputBeSame() const { return false;}
    virtual bool		needsFullVolume() const { return true; }
    const Attrib::DataCubes*	getOutput() const	{ return output_; }
    Attrib::DataCubes*		getOutput()		{ return output_; }

    virtual const VelocityDesc*	getVelDesc() const	{ return 0; } // old

    virtual bool		areSamplesIndependent() const { return true; }
				/*!<returns whether samples in the output
				 are independent from each other.*/

    virtual Task*		createTask();

    virtual void		fillPar(IOPar&) const;
    virtual bool		usePar(const IOPar&);

    virtual void		releaseData();

    virtual const char*		errMsg() const { return 0; }

protected:
				Step();

    friend		class BinIDWiseTask;
    virtual bool	prefersBinIDWise() const		{ return false;}
    virtual bool	computeBinID(const BinID&,int threadid)	{ return false;}
    virtual bool	prepareComp(int nrthreads)		{ return true;}

    Chain*				chain_;

    ObjectSet<const Attrib::DataCubes>	inputs_;
    TypeSet<InputSlotID>		inputslotids_;

    BufferString			username_;
    ID					id_;

    HorSampling				hrg_;
    StepInterval<int>			zrg_;
    TypeSet<OutputSlotID>		outputslotids_; // enabled slotids

private:
    Attrib::DataCubes*		output_;
};



/*!
\brief A chain of Steps that can be applied to a volume of scalars.
*/

mExpClass(VolumeProcessing) Chain
{ mRefCountImpl(Chain);
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

    bool			areSamplesIndependent() const;
    bool			needsFullVolume() const;
    const char*			errMsg() const;

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

    BufferString		errmsg_;
    Threads::Atomic<int>	freeid_;
};



/*!
\brief Chain Executor
*/

mExpClass(VolumeProcessing) ChainExecutor : public Executor
{
public:
				ChainExecutor(Chain&);
				~ChainExecutor();

    const char*			errMsg() const;
    const char*			nrDoneText() const;

    bool			setCalculationScope(const HorSampling&,
						    const StepInterval<int>&);

    const Attrib::DataCubes*	getOutput() const;
    int				nextStep();

private:
    class Epoch
    {
    public:
				Epoch(const ChainExecutor& c)
				    : taskgroup_( *new TaskGroup )
				    , chainexec_( c )
				{ taskgroup_.setParallel(true); }

				~Epoch()		{ delete &taskgroup_; }

	void			addStep(Step* s)	{ steps_ += s; }

	bool			doPrepare();
	Task&			getTask()		{ return taskgroup_; }

	bool			needsStepOutput(Step::ID) const;
	const Attrib::DataCubes* getOutput() const;

    private:

	BufferString		errmsg_;
	const ChainExecutor&	chainexec_;
	TaskGroup&		taskgroup_;
	ObjectSet<Step>		steps_;
    };

    bool			scheduleWork();
    int				computeLatestEpoch(Step::ID) const;
    void			computeComputationScope(Step::ID stepid,
				    HorSampling& stepoutputhrg,
				    StepInterval<int>& stepoutputzrg ) const;

    void			controlWork(Task::Control);
    od_int64			nrDone() const;
    od_int64			totalNr() const;
    const char*			message() const;

    void			releaseMemory();

    Epoch*			curepoch_;

    bool			isok_;
    bool			firstisprep_;
    Chain&			chain_;

    HorSampling			outputhrg_;
    StepInterval<int>		outputzrg_;

    mutable BufferString	errmsg_;
    ObjectSet<Step>		scheduledsteps_;
    ObjectSet<Epoch>		epochs_;
    Chain::Web			web_;
    int				totalnrepochs_;

    const Attrib::DataCubes*	outputvolume_;
};

} // namespace VolProc

#endif

