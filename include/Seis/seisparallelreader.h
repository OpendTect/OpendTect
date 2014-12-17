#ifndef seisparallelreader_h
#define seisparallelreader_h
/*
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	K. Tingdahl
 Date:		July 2010
 RCS:		$Id$
________________________________________________________________________

*/

#include "seismod.h"
#include "uistring.h"
#include "executor.h"
#include "fixedstring.h"
#include "paralleltask.h"
#include "sets.h"
#include "trckeyzsampling.h"

class BinIDValueSet;
class CBVSSeisTrcTranslator;
class IOObj;
class SampledAttribDataPack;

template <class T> class Array2D;
template <class T> class Array3D;


namespace Seis
{

class SelData;

/*!Reads a 3D Seismic volume in parallel into an Array3D<float> or
   into a BinIDValueSet */

mExpClass(Seis) ParallelReader : public ParallelTask
{ mODTextTranslationClass(ParallelReader)
public:
			ParallelReader(const IOObj&,
			    const TypeSet<int>& components,
			    const ObjectSet<Array3D<float> >&,
			    const TrcKeyZSampling&);
			/*!<Allocates & resizes the cubes to fit the cs and the
			    nr of comps. If data is missing in the storage, the
			    cube will not be overwritten in those locations. */

			ParallelReader(const IOObj&,
				const TrcKeyZSampling&);
			/*!<Calculates nr of comps and allocates cubes to
			    fit the cs. */

			ParallelReader(const IOObj&,
			    BinIDValueSet&,
			    const TypeSet<int>& components);
			/*!<Will read the z from the first value. Will add
			    values to accomodate nr of components. If data
			    cannot be read, that binid/z will be set to
			    mUdf */

			~ParallelReader();

    const ObjectSet<Array3D<float> >* getArrays() const	{ return arrays_; }

    uiString		uiNrDoneText() const;
    uiString		uiMessage() const;

protected:
    od_int64		nrIterations() const { return totalnr_; }
    bool		doPrepare(int nrthreads);
    bool		doWork(od_int64,od_int64,int);
    bool		doFinish(bool);


    TypeSet<int>		components_;

    BinIDValueSet*		bidvals_;

    ObjectSet<Array3D<float> >*	arrays_;
    TrcKeyZSampling		tkzs_;

    IOObj*			ioobj_;
    od_int64			totalnr_;

    uiString			errmsg_;
};


/*!Reads a 2D Seismic volume in parallel into an Array2D<float> */

mExpClass(Seis) ParallelReader2D : public ParallelTask
{ mODTextTranslationClass(ParallelReader2D)
public:
			ParallelReader2D(const IOObj&,Pos::GeomID,
					 const TrcKeyZSampling&);
			/*!<Calculates nr of comps and allocates arrays to
			    fit the cs. */

			~ParallelReader2D();

    const ObjectSet<Array2D<float> >* getArrays() const { return arrays_; }

    uiString		uiNrDoneText() const;
    uiString		uiMessage() const;

protected:
    od_int64		nrIterations() const;
    bool		doPrepare(int nrthreads);
    bool		doWork(od_int64,od_int64,int);
    bool		doFinish(bool);

    TypeSet<int>		components_;
    ObjectSet<Array2D<float> >* arrays_;
    TrcKeyZSampling		tkzs_;
    Pos::GeomID			geomid_;
    IOObj*			ioobj_;
    od_int64			totalnr_;
    uiString			errmsg_;
};


mExpClass(Seis) SequentialReader : public Executor
{
public:
			SequentialReader(const IOObj&,
					 const TypeSet<int>& components,
					 const TrcKeyZSampling&);
			~SequentialReader();

    uiString		uiMessage() const	{ return msg_; }
    uiString		uiNrDoneText() const	{ return "Traces read"; }
    od_int64		nrDone() const		{ return nrdone_; }
    od_int64		totalNr() const		{ return totalnr_; }
    int			nextStep();

protected:

    CBVSSeisTrcTranslator*	trl_;
    Seis::SelData*		sd_;
    TrcKeyZSampling		tkzs_;
    SampledAttribDataPack*	dp_;
    TypeSet<int>		components_;
    Interval<int>		samprg_;
    unsigned char**		blockbufs_;

    int				queueid_;

    od_int64			totalnr_;
    od_int64			nrdone_;
    uiString			msg_;
};

} // namespace Seis

#endif
