#ifndef attribprocessor_h
#define attribprocessor_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Kristofer Tingdahl
 Date:          07-10-1999
 RCS:           $Id: attribprocessor.h,v 1.1 2005-02-03 15:35:02 kristofer Exp $
________________________________________________________________________

-*/

#include "position.h"
#include "executor.h"

class BinIDValueSet;
class CubeSampling;
template <class T> class Interval;

namespace Attrib
{
class DataHolder;
class Desc;
class Provider;

class AttribProcessor : public Executor
{
public:
    			AttribProcessor( Desc& );
    			~AttribProcessor();

    virtual bool	isOK() const;
    void		enableOutput( int );

    bool		getPossibleOutput(CubeSampling&) const;

    int			nextStep();

protected:
    virtual CubeSampling	getDesiredVolume() const		= 0;

    virtual bool		collectData()				= 0;
    BinID			currentPosition() const;
    const DataHolder*		getData(const Interval<int>& zrg);

    Desc&			desc;
    Provider*			provider;
    BinIDValueSet*		geometry;
    
    int				nriter;
};

}; //Namespace


#endif
