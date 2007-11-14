#ifndef prestackprocessor_h
#define prestackprocessor_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	K. Tingdahl
 Date:		April 2005
 RCS:		$Id: prestackprocessor.h,v 1.9 2007-11-14 17:54:32 cvskris Exp $
________________________________________________________________________


-*/

#include "task.h"
#include "bufstringset.h"
#include "datapack.h"
#include "sets.h"
#include "factory.h"

class IOPar;

namespace PreStack
{

void initBuiltinClasses();

class Gather;

class Processor : public ParallelTask
{
public:
    virtual			~Processor();

    void			setInput(DataPack::ID);
    DataPack::ID		getOutput() const;
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

    Gather*			output_;
    const Gather*		input_;
};


mDefineFactory( Processor, PF );

class ProcessManager : public CallBacker
{
public:
    				ProcessManager();
    				~ProcessManager();
    void			setInput(DataPack::ID);
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
    const Gather*		input_;
    const Gather*		output_;
};

}; //namespace

#endif
