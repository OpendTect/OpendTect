#ifndef volprocchain_h
#define volprocchain_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	K. Tingdahl
 Date:		October 2006
 RCS:		$Id: volprocchain.h,v 1.11 2010-05-10 16:10:45 cvskris Exp $
________________________________________________________________________


-*/

#include "attribdatacubes.h"
#include "multiid.h"
#include "executor.h"
#include "factory.h"
#include "refcount.h"
#include "samplingdata.h"
#include "thread.h"

namespace Attrib { class DataCubes; }

class VelocityDesc;
class Executor;
class HorSampling;
template <class T> class StepInterval;
template <class T> class ValueSeries;

namespace VolProc
{

class Step;
class StepExecutor;
class StepTask;


/*!A chain of Steps that can be applied to a volume of scalars. */

mClass Chain
{ mRefCountImpl(Chain);
public:
    				Chain();

    void			setZSampling(const SamplingData<float>&,
	    				     bool zit);
    const SamplingData<float>&	getZSampling() const;
    bool			zIsT() const { return zit_; }

    int				nrSteps() const; 
    Step*			getStep(int);
    int				indexOf(const Step*) const;
    void			addStep(Step*);
    void			insertStep(int,Step*);
    void			swapSteps(int,int);
    void			removeStep(int);

    const VelocityDesc*		getVelDesc() const;

    void			fillPar(IOPar&) const;
    bool			usePar(const IOPar&);

    void			setStorageID(const MultiID& mid);
    const MultiID&		storageID() const { return storageid_; }

    const char*			errMsg() const;

protected:

    static const char*		sKeyNrSteps()	{ return "Nr Steps"; }
    static const char*		sKeyStepType()	{ return "Type"; }
    MultiID			storageid_;
    ObjectSet<Step>		steps_;
    SamplingData<float>		zsampling_;
    bool			zit_;
    BufferString		errmsg_;
};


/*!An algorithm/calculation/transoformation that takes one scalar volume as
   input, processes it, and puts the output in another volume. */
mClass Step
{
public:
    				Step(Chain&);
    virtual			~Step();
    const Chain&		getChain() const	{ return chain_; }

    virtual const char*		type() const 				= 0;
    virtual const char*		userName() const;
    virtual void		setUserName(const char* nm);

    void			enable(bool yn);
    bool			enabled() const;

    virtual bool		needsInput(const HorSampling&) const	= 0;
    				/*!<When computing HorSampling, do I need
				    the input? */
    virtual HorSampling		getInputHRg(const HorSampling&) const;
    				/*!<When computing HorSampling, how
				    big input is needed? */
    virtual StepInterval<int>	getInputZRg(const StepInterval<int>&) const;
    				/*!<When computing HorSampling, how
				    big input is needed?*/

    virtual bool		setInput(const Attrib::DataCubes*);
    				//!<\returns true if it wants to keep the data.
    virtual void		setOutput(Attrib::DataCubes*);
    virtual bool		canInputAndOutputBeSame() const { return false;}
    virtual bool		needsFullVolume() const { return true;}
    const Attrib::DataCubes*	getOutput() const	{ return output_; }
    Attrib::DataCubes*		getOutput()		{ return output_; }

    virtual const VelocityDesc*	getVelDesc() const	{ return 0; }

    virtual bool		areSamplesIndependent() const { return true; }
    				//!<\returns wether samples int the output
				//!<	     are independent from each other

    virtual Task*		createTask();

    virtual void		fillPar(IOPar&) const;
    virtual bool		usePar(const IOPar&);

    virtual void		releaseData();

    virtual const char*		errMsg() const { return 0; }

protected:
    friend		class BinIDWiseTask;
    virtual bool	prefersBinIDWise() const		{ return false;}
    virtual bool	computeBinID(const BinID&,int threadid)	{ return false;}
    virtual bool	prepareComp(int nrthreads)		{ return true;}

    static const char*		sKeyEnabled() { return "Enabled"; }
    Chain&			chain_;
    bool			enabled_;

    Attrib::DataCubes*		output_;
    const Attrib::DataCubes*	input_;
    BufferString		username_;
};


mDefineFactory1Param( Step, Chain&, PS );

mClass ChainExecutor : public Executor
{
public:
				ChainExecutor(Chain&);
				~ChainExecutor();

    const char*			errMsg() const;

    bool			setCalculationScope(const CubeSampling&);
    const Attrib::DataCubes*	getOutput() const;
    int				nextStep();

protected:
    bool			prepareNewStep();
    void			controlWork(Task::Control);
    od_int64			nrDone() const;
    od_int64			totalNr() const;
    const char*			message() const;

    mutable Threads::Mutex	curtasklock_;
    Task*			curtask_;
    ObjectSet<Step>		steps_;
    int				currentstep_;

    bool			isok_;
    bool			firstisprep_;
    Chain&			chain_;

    HorSampling			hrg_;
    StepInterval<int>		zrg_;

    BufferString		errmsg_;

    RefMan<Attrib::DataCubes>   curinput_;
    RefMan<Attrib::DataCubes>   curoutput_;
};


}; //namespace

#endif
