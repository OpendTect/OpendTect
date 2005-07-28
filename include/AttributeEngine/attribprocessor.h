#ifndef attribprocessor_h
#define attribprocessor_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Kristofer Tingdahl
 Date:          07-10-1999
 RCS:           $Id: attribprocessor.h,v 1.5 2005-07-28 10:53:49 cvshelene Exp $
________________________________________________________________________

-*/

#include "position.h"
#include "executor.h"
#include "linekey.h"
#include "attribdesc.h"

class CubeSampling;
template <class T> class Interval;

namespace Attrib
{
class DataHolder;
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
    const char*		getAttribName() { return desc_.attribName(); }
    Provider*		getProvider() { return provider; }
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
