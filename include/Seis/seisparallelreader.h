#ifndef parallelseisread_h
#define parallelseisread_h
/*
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	K. Tingdahl
 Date:		July 2010
 RCS:		$Id: seisparallelreader.h,v 1.1 2011-08-10 06:15:41 cvskris Exp $
________________________________________________________________________

*/

#include "sets.h"
#include "task.h"
#include "cubesampling.h"

class IOObj;
class SeisTrcReader;
class CubeSampling;
class BinIDValueSet;

template <class T> class Array3D;


namespace Seis
{

mClass ParallelReader : public ParallelTask
{
public:
    			ParallelReader( const IOObj&,
				const TypeSet<int>& components,
				ObjectSet<Array3D<float> >&,
				const CubeSampling& );

    			ParallelReader( const IOObj&, BinIDValueSet&,
				const TypeSet<int>& components );

    			~ParallelReader();
protected:
    od_int64			nrIterations() const { return totalnr_; }
    bool			doPrepare(int nrthreads);
    bool			doWork(od_int64,od_int64,int);
    bool			doFinish(bool);

    TypeSet<int>		components_;

    BinIDValueSet*		bidvals_;

    ObjectSet<Array3D<float> >*	arrays_;
    CubeSampling		cs_;

    IOObj*			ioobj_;
    od_int64			totalnr_;
};

}; //namespace
#endif

