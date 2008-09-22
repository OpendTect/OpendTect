#ifndef attribprocessor_h
#define attribprocessor_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Kristofer Tingdahl
 Date:          07-10-1999
 RCS:           $Id: attribprocessor.h,v 1.23 2008-09-22 13:06:42 cvskris Exp $
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

    virtual bool		isOK() const;
    void			addOutput(Output*);

    int				nextStep();
    void			init();
    od_int64			totalNr() const;
    od_int64			nrDone() const;
    const char*         	message() const;

    void			addOutputInterest(int sel);
    bool			setZIntervals(TypeSet< Interval<int> >&,
					      const BinID&,const Coord&);
    void			computeAndSetRefZStep();
    
    Notifier<Attrib::Processor>	moveonly;
				/*!< triggered after a position is reached that
				     requires no processing, e.g. during initial
				     buffer fills. */
    
    const char*			getAttribName() const; 	
    const char*			getAttribUserRef() const; 	
    Provider*			getProvider() 		{ return provider_; }
    ObjectSet<Output>   	outputs_;

protected:
    void		useFullProcess(int&);
    void		useSCProcess(int&);
    void		fullProcess(const SeisTrcInfo*);

    void		defineGlobalOutputSpecs(TypeSet<int>&,CubeSampling&);
    void		prepareForTableOutput();
    void		computeAndSetPosAndDesVol(CubeSampling&);

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
};


} // namespace Attrib


#endif
