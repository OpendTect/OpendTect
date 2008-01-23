#ifndef prestackprocessor_h
#define prestackprocessor_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	K. Tingdahl
 Date:		April 2005
 RCS:		$Id: prestackprocessor.h,v 1.11 2008-01-23 20:56:59 cvskris Exp $
________________________________________________________________________


-*/

#include "bufstringset.h"
#include "datapack.h"
#include "factory.h"
#include "position.h"
#include "sets.h"
#include "task.h"

class IOPar;

namespace PreStack
{

class Gather;

class Processor : public ParallelTask
{
public:
    virtual			~Processor();

    virtual bool		reset();

    virtual const BinID&	getInputStepout() const;
    virtual bool		wantsInput(const BinID& relbid) const;
    void			setInput(const BinID& relbid,DataPack::ID);

    const BinID&		getOutputStepout() const;
    virtual bool		setOutputInterest(const BinID& relbid,bool);
    bool			getOutputInterest(const BinID& relbid) const;
    DataPack::ID		getOutput(const BinID& relbid) const;
    
    virtual bool		prepareWork();
    virtual const char*		errMsg() const { return 0; }

    virtual void		fillPar(IOPar&) const			= 0;
    virtual bool		usePar(const IOPar&)			= 0;
    virtual bool		doWork(int start, int stop, int)	= 0;
    				/*!<If totalNr is not overridden, start and
				    stop will refer to offsets that should
				    be processed. */

    int				nrOffsets() const;
    virtual int			totalNr() const { return nrOffsets(); }
    				/*!<If algorithms cannot be done in parallel
				    with regards to offsets, override function
				    and return 1. doWork() will then be called
				    with start=stop=0 and you can do whatever
				    you want in doWork. You can also return
				    number of samples in gather and run it
				    parallel vertically.*/

protected:
    				Processor( const char* nm );
    virtual Gather*		createOutputArray(const Gather& input) const;
    static void			setStepout(ObjectSet<Gather>&,
	    				   const BinID& oldstepout,
					   const BinID& newstepout);
    static int			getRelBidOffset(const BinID& stepout,
	    					const BinID& relbid);
    static void			freeArray(ObjectSet<Gather>&);

    BinID			outputstepout_;
    ObjectSet<Gather>		outputs_;
    BoolTypeSet			outputinterest_;

    ObjectSet<Gather>		inputs_;
};


mDefineFactory( Processor, PF );

class ProcessManager : public CallBacker
{
public:
    				ProcessManager();
    				~ProcessManager();

    BinID			getInputStepout() const;
    virtual bool		wantsInput(const BinID& relbid) const;
    void			setInput(const BinID& relbid,DataPack::ID);

    bool			reset();
    bool			process(bool forceall);
    DataPack::ID		getOutput() const;

    void			addProcessor(Processor*);
    int				nrProcessors() const;
    void			removeProcessor(int);
    void			swapProcessors(int,int);

    Processor*			getProcessor(int);
    void			notifyChange()	{ setupChange.trigger(); }

    const Processor*		getProcessor(int) const;

    Notifier<ProcessManager>	setupChange;

    void			fillPar(IOPar&) const;
    bool			usePar(const IOPar&);

protected:

    static const char*	sKeyNrProcessors()	{ return "Nr processors"; }

    ObjectSet<Processor>	processors_;
};

}; //namespace

#endif
