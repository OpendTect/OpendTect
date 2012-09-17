#ifndef volprocchain_h
#define volprocchain_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	K. Tingdahl
 Date:		October 2006
 RCS:		$Id: volprocchain.h,v 1.16 2011/08/26 08:24:52 cvskris Exp $
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

    void			setZStep(float z,bool zit) {zstep_=z,zit_=zit; }
    float			getZStep() const	   { return zstep_; }
    bool			zIsT() const		   { return zit_; }

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

    bool			areSamplesIndependent() const;

    const char*			errMsg() const;

protected:

    static const char*		sKeyNrSteps()	{ return "Nr Steps"; }
    static const char*		sKeyStepType()	{ return "Type"; }
    MultiID			storageid_;
    ObjectSet<Step>		steps_;

    float			zstep_;
    bool			zit_;

    BufferString		errmsg_;
};


/*!An algorithm/calculation/transoformation that takes one scalar volume as
   input, processes it, and puts the output in another volume. */
mClass Step
{
public:
				mDefineFactoryInClass( Step, factory );
    				Step();
    virtual			~Step();

    Chain&			getChain() { return *chain_; }
    void			setChain(Chain& c) { chain_ = &c; }

    virtual const char*		userName() const;
    virtual void		setUserName(const char* nm);

    void			enable(bool yn);
    bool			enabled() const;

    virtual bool		needsInput() const		= 0;
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
    virtual void		setOutput(Attrib::DataCubes*,
	    			    const StepInterval<int>& inlrg,
				    const StepInterval<int>& crlrg,
				    const StepInterval<int>& zrg);

    virtual bool		canInputAndOutputBeSame() const { return false;}
    virtual bool		needsFullVolume() const { return true;}
    const Attrib::DataCubes*	getOutput() const	{ return output_; }
    Attrib::DataCubes*		getOutput()		{ return output_; }

    virtual const VelocityDesc*	getVelDesc() const	{ return 0; }

    virtual bool		areSamplesIndependent() const { return true; }
    				//!<\returns wether samples in the output
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
    bool			enabled_;

    Chain*			chain_;

    Attrib::DataCubes*		output_;
    const Attrib::DataCubes*	input_;
    BufferString		username_;


    HorSampling			hrg_;
    StepInterval<int>		zrg_;
};



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
