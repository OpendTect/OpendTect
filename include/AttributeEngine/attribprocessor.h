#ifndef attribprocessor_h
#define attribprocessor_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Kristofer Tingdahl
 Date:          07-10-1999
 RCS:           $Id: attribprocessor.h,v 1.16 2007-01-04 15:29:26 cvshelene Exp $
________________________________________________________________________

-*/

#include "executor.h"
#include "position.h"

class CubeSampling;
class SeisSelData;
class SeisTrcInfo;
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
    SeisSelData*	sd_;

    void		useFullProcess(int&);
    void		useSCProcess(int&);
    void		fullProcess(const SeisTrcInfo*);
};


} // namespace Attrib


#endif
