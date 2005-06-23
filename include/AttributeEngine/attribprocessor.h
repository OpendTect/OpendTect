#ifndef attribprocessor_h
#define attribprocessor_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Kristofer Tingdahl
 Date:          07-10-1999
 RCS:           $Id: attribprocessor.h,v 1.4 2005-06-23 09:13:36 cvshelene Exp $
________________________________________________________________________

-*/

#include "position.h"
#include "executor.h"
#include "linekey.h"

class CubeSampling;
template <class T> class Interval;

namespace Attrib
{
class DataHolder;
class Desc;
class Provider;
class Output;

class Processor : public Executor
{
public:
    			Processor( Desc&, const char* lk = "" );
    			~Processor();

    virtual bool	isOK() const;
    void		addOutput( Output* );

    int			nextStep();

    int 		nrDone() { return nriter; }

    void		addOutputInterest(int seloutp) {outpinterest+= seloutp;}
    
    Notifier<Attrib::Processor>      moveonly;
                     /*!< triggered after a position is reached that requires
                          no processing, e.g. during initial buffer fills. */
    
    int			totalNr();
    ObjectSet<Output>           outputs;

protected:

    Desc&			desc_;
    BufferString		lk_;
    Provider*			provider;
    int				nriter;
    bool 			is2d_;
    TypeSet<int>		outpinterest;

};


}; //Namespace


#endif
