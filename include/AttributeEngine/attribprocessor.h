#ifndef attribprocessor_h
#define attribprocessor_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Kristofer Tingdahl
 Date:          07-10-1999
 RCS:           $Id: attribprocessor.h,v 1.13 2005-10-20 13:58:50 cvshelene Exp $
________________________________________________________________________

-*/

#include "executor.h"
#include "position.h"

class CubeSampling;
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
    			Processor(Desc&,const char* lk="");
    			~Processor();

    virtual bool	isOK() const;
    void		addOutput(Output*);

    int			nextStep();
    void		init();
    int			totalNr() const;
    int 		nrDone() const 		{ return nrdone; }
    const char*         message() const
			{ return *(const char*)errmsg ? (const char*)errmsg
						      : "Processing"; }

    void		addOutputInterest(int sel)     { outpinterest_ += sel; }
    bool		setZIntervals(TypeSet< Interval<int> >&, BinID);
    
    Notifier<Attrib::Processor>      moveonly;
                     /*!< triggered after a position is reached that requires
                          no processing, e.g. during initial buffer fills. */
    
    const char*		getAttribName(); 	
    Provider*		getProvider() 		{ return provider; }
    ObjectSet<Output>   outputs;

protected:

    Desc&		desc_;
    Provider*		provider;
    int			nriter;
    int			nrdone;
    bool 		is2d_;
    TypeSet<int>	outpinterest_;
    BufferString	errmsg;
    bool		isinited;

    BinID		prevbid_;
};


} // namespace Attrib


#endif
