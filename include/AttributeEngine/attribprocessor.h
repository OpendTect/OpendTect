#ifndef attribprocessor_h
#define attribprocessor_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Kristofer Tingdahl
 Date:          07-10-1999
 RCS:           $Id: attribprocessor.h,v 1.19 2007-11-23 11:59:06 cvsbert Exp $
________________________________________________________________________

-*/

#include "executor.h"
#include "position.h"

class CubeSampling;
class SeisTrcInfo;
namespace Seis { class SelData; }
template <class T> class Interval;

namespace Attrib
{
class DataHolder;
class Desc;
class Output;
class Provider;

class Processor : public Executor
{
public:
    			Processor(Desc&,const char*,BufferString&);
    			~Processor();

    virtual bool	isOK() const;
    void		addOutput(Output*);

    int			nextStep();
    void		init();
    int			totalNr() const;
    int 		nrDone() const 		{ return nrdone_; }
    const char*         message() const
			{ return *(const char*)errmsg_ ? (const char*)errmsg_
						       : "Processing"; }

    void		addOutputInterest(int sel)     { outpinterest_ += sel; }
    bool		setZIntervals(TypeSet< Interval<int> >&, BinID);
    void		computeAndSetRefZStep();
    
    Notifier<Attrib::Processor>      moveonly;
                     /*!< triggered after a position is reached that requires
                          no processing, e.g. during initial buffer fills. */
    
    const char*		getAttribName(); 	
    Provider*		getProvider() 		{ return provider_; }
    ObjectSet<Output>   outputs_;

protected:

    Desc&		desc_;
    Provider*		provider_;
    int			nriter_;
    int			nrdone_;
    bool 		is2d_;
    TypeSet<int>	outpinterest_;
    BufferString	errmsg_;
    bool		isinited_;
    bool		useshortcuts_;

    BinID		prevbid_;
    Seis::SelData*	sd_;

    void		useFullProcess(int&);
    void		useSCProcess(int&);
    void		fullProcess(const SeisTrcInfo*);

    void		defineGlobalOutputSpecs(TypeSet<int>&,CubeSampling&);
    void		prepareForTableOutput();
    void		computeAndSetPosAndDesVol(CubeSampling&);
};


} // namespace Attrib


#endif
