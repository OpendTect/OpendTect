#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "prestackgather.h"

#include "factory.h"
#include "keystrs.h"
#include "paralleltask.h"


namespace PreStack
{

/*!
\brief Processes PreStack data at one cdp location. The algorithm is
implemented in subclasses, and can be created by the PreStack::PF() factory.
*/

mExpClass(PreStackProcessing) Processor : public ParallelTask
{
public:
				mDefineFactoryInClass( Processor, factory );
				~Processor();

    virtual bool		reset(bool force=true);

    virtual const BinID&	getInputStepout() const;
    virtual bool		wantsInput(const BinID& relbid) const;
    void			setInput(const BinID& relbid,DataPackID);
    void			setInput(const BinID&,Gather*);
    void			setInput(const BinID&,const Gather*);

    const BinID&		getOutputStepout() const;
    virtual bool		setOutputInterest(const BinID& relbid,bool);
    bool			getOutputInterest(const BinID& relbid) const;
    DataPackID			getOutputID(const BinID& relbid) const;
    ConstRefMan<Gather>		getOutput(const BinID& relbid) const;

    virtual bool		prepareWork();
    virtual uiString		errMsg() const
				{ return uiString::emptyString(); }

    virtual void		fillPar(IOPar&) const			= 0;
    virtual bool		usePar(const IOPar&)			= 0;
    bool			doWork(od_int64 start,
				       od_int64 stop,int) override = 0;
				/*!<If nrIterations is not overridden, start and
				    stop will refer to offsets that should
				    be processed. */

    int				nrOffsets() const;
    od_int64			nrIterations() const override
				{ return nrOffsets(); }
				/*!<If algorithms cannot be done in parallel
				    with regards to offsets, override function
				    and return 1. doWork() will then be called
				    with start=stop=0 and you can do whatever
				    you want in doWork. You can also return
				    number of samples in gather and run it
				    parallel vertically.*/

    virtual bool		usesPreStackInput() const	{ return true; }
    virtual void		adjustPossibleCompArea(TrcKeySampling&){return;}
    virtual void		retainCurBID( const BinID& ) {};
    virtual bool		mustHaveUserInput() { return true; }

protected:
				Processor(const char* nm);
    virtual Gather*		createOutputArray(const Gather& input) const;
    static int			getRelBidOffset(const BinID& relbid,
						const BinID& stepout);
    static void			freeArray(RefObjectSet<Gather>&);
    mDeprecated("Use method taking RefObjectSet")
    static void			freeArray(ObjectSet<Gather>&);

    BinID			outputstepout_;
    RefObjectSet<Gather>	outputs_;
    BoolTypeSet			outputinterest_;

    RefObjectSet<Gather>	inputs_;
};


/*!
\brief Orgainizes a number of PreStack::Processors into a chain which
   can be processed.

  Support for processing prestack gathers is done by a
  PreStack::ProcessManager. The PreStack::ProcessManager has a chain of
  PreStack::Processor which are run in sequence.

  Example:
  \code
  PreStack::ProcessManager processmanager;
  PreStack::AGC* agc = new PreStack::AGC;
  agc->setWindow( Interval<float>( -120, 120 ) );
  processmanager.addProcessor( agc );

  processmanager.reset();
  //Not really necessary since the manager has not been used before

  const BinID stepout = processmanager.getInputStepout();
  BinID relbid;
  for ( relbid.inl()=-stepout.inl(); relbid.inl()<=stepout.inl();
		relbid.inl()++ )
  {
      for ( relbid.crl()=-stepout.crl(); relbid.crl()<=stepout.crl();
		relbid.crl()++ )
      {
	  if ( !processor.wantsInput(relbid) )
	      continue;

	  const BinID inputbid( relbid*BinID(SI().inlStep(),SI().crlStep()) );

	  const DataPackID dpid = getDataPackFromSomewhere( inputbid );
	  if ( dpid==DataPack::cNoID() )
	      return error;

	  processmanager.setInput( relbid, dpid );
      }
  }

  if ( !processmanager.process() )
      return error;

  DataPackID result = processmanager.getOutput();
  \endcode
*/

mExpClass(PreStackProcessing) ProcessManager : public CallBacker
{ mODTextTranslationClass(ProcessManager)
public:
				ProcessManager();
				~ProcessManager();

				//Setup
    int				nrProcessors() const;
    Processor*			getProcessor(int);
    const Processor*		getProcessor(int) const;
    bool			needsPreStackInput() const;


    void			addProcessor(Processor*);
    void			removeProcessor(int);
    void			swapProcessors(int,int);
    int				indexOf(const Processor*) const;

    void			removeAllProcessors();

    void			notifyChange()	{ setupChange.trigger(); }
    Notifier<ProcessManager>	setupChange;

				//Runtime
    bool			reset(bool force=true);
				//!<Call when you are about to process new data
    bool			prepareWork();
    BinID			getInputStepout() const;
				//!<Only after prepareWork
    virtual bool		wantsInput(const BinID& relbid) const;
				//!<Only after prepareWork
    void			setInput(const BinID& relbid,DataPackID);
    void			setInput(const BinID&,Gather*);
    void			setInput(const BinID&,const Gather*);

    bool			process();

    DataPackID			getOutputID() const;
    ConstRefMan<Gather>		getOutput() const;

    void			fillPar(IOPar&) const;
    bool			usePar(const IOPar&);

    uiString			errMsg() const	{ return errmsg_; }

    //Keys for od_process_prestack
    static const char*		sKeyLineKey()	{ return sKey::LineKey(); }
    static const char*		sKeySetup(){ return "Processing Setup"; }
    static const char*		sKeyCDPRange(){ return "CDP Range"; }
    static const char*		sKeyInputData() { return "Input"; }
    static const char*		sKeyOutputData(){ return "Output"; }


protected:

    static const char*	sKeyNrProcessors()	{ return "Nr processors"; }

    ObjectSet<Processor>	processors_;
    uiString			errmsg_;
};


#define mPSProcAddStepoutStep( array, arrtype, oldstepout, newstepout ) \
{ \
    arrtype arrcopy( array ); \
    array.erase(); \
\
    for ( int idx=-newstepout.inl(); idx<=newstepout.inl(); idx++ ) \
    { \
	for ( int idy=-newstepout.crl(); idy<=newstepout.crl(); idy++ ) \
	{ \
	    const BinID curpos( idx, idy ); \
\
	    if ( idy<-oldstepout.crl() || idy>oldstepout.crl() || \
		idx<-oldstepout.inl() || idx>oldstepout.inl() ) \
	    { \
		array += 0; \
	    } \
	    else \
	    { \
		const int oldoffset=getRelBidOffset(curpos,oldstepout);\
		array += arrcopy[oldoffset]; \
	    } \
	} \
    } \
}

} // namespace PreStack
