#ifndef attribprocessor_h
#define attribprocessor_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Kristofer Tingdahl
 Date:          07-10-1999
 RCS:           $Id: attribprocessor.h,v 1.2 2005-02-04 09:28:35 kristofer Exp $
________________________________________________________________________

-*/

#include "position.h"
#include "executor.h"

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
    			Processor( Desc& );
    			~Processor();

    virtual bool	isOK() const;
    void		addOutput( Output* );

    int			nextStep();

protected:

    Desc&			desc;
    Provider*			provider;
    int				nriter;

    ObjectSet<Output>		outputs;
};


}; //Namespace


#endif
