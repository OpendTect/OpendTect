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

#include "datachar.h"
#include "executor.h"
#include "fixedstring.h"
#include "paralleltask.h"
#include "sets.h"
#include "trckeyzsampling.h"
#include "uistring.h"
#include "valseriesinterpol.h"

class BinDataDesc;
class BinIDValueSet;
class IOObj;
class RegularSeisDataPack;
class Scaler;
class SeisTrc;
class SeisTrcBuf;
class SeisTrcReader;
class TraceData;

template <class T> class Array2D;
template <class T> class Array3D;
template <class T> class DataInterpreter;
namespace PosInfo { class CubeData; class CubeDataIterator; }


namespace Seis
{

class ObjectSummary;
class SelData;

/*!Reads a 3D Seismic volume in parallel into an Array3D<float> or
   into a BinIDValueSet
   Consider using the SequentialReader class for better performance
   and additional functionality
*/

mExpClass(Seis) ParallelReader : public ParallelTask
{ mODTextTranslationClass(ParallelReader)
public:
			ParallelReader(const IOObj&,const TrcKeyZSampling&);
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

    void		setComponents( const TypeSet<int>& compnrs )
			{ components_ = compnrs; }
    mDeprecated bool	setOutputComponents(const TypeSet<int>&);
			/*!< Will be removed after 6.2 */

    void		 setDataPack(RegularSeisDataPack*);
    RegularSeisDataPack* getDataPack();

    uiString		uiNrDoneText() const;
    uiString		uiMessage() const;

protected:
    od_int64		nrIterations() const { return totalnr_; }

    bool		setOutputComponents();

    bool		doPrepare(int nrthreads);
    bool		doWork(od_int64,od_int64,int);
    bool		doFinish(bool);


    TypeSet<int>		components_;

    BinIDValueSet*		bidvals_;

    RegularSeisDataPack*	dp_;
    TrcKeyZSampling		tkzs_;

    IOObj*			ioobj_;
    od_int64			totalnr_;

    uiString			errmsg_;
    TypeSet<int>		seisrdroutcompmgr_;

private:

    void		submitUdfWriterTasks();
};


/*!Reads a 2D Seismic volume in parallel into an Array2D<float> */

mExpClass(Seis) ParallelReader2D : public ParallelTask
{ mODTextTranslationClass(ParallelReader2D)
public:
			ParallelReader2D(const IOObj&,Pos::GeomID,
					 const TrcKeyZSampling* =0,
					 const TypeSet<int>* comps=0);
			/*!<Calculates nr of comps and allocates arrays to
			    fit the cs. */

			~ParallelReader2D();

    void		setDataChar(DataCharacteristics::UserType);
    void		setScaler(Scaler*);
    bool		init();

    RegularSeisDataPack* getDataPack(); // The caller now owns the datapack

    uiString		uiNrDoneText() const;
    uiString		uiMessage() const;

protected:
    od_int64		nrIterations() const;
    bool		doPrepare(int nrthreads);
    bool		doWork(od_int64,od_int64,int);
    bool		doFinish(bool);

    RegularSeisDataPack*	dp_;
    TypeSet<int>		components_;
    TrcKeyZSampling		tkzs_;
    Pos::GeomID			geomid_;
    IOObj*			ioobj_;
    DataCharacteristics		dc_;
    Scaler*			scaler_;
    od_int64			totalnr_;
    uiString			msg_;

    bool			dpclaimed_;
};


/*!Reads a 3D Seismic volume in parallel into a RegularSeisDataPack

    Usage example:
    SequentialReader rdr( myiioobj ); // I want to read all
    rdr.setDataChar( DataCharacteristics:: ); // read in another format
    rdr.setScaler( myscaler ); // scale data to fit in required format
    if ( rdr.init() ) // something is not right
    rdr.execute();
*/

mExpClass(Seis) SequentialReader : public Executor
{ mODTextTranslationClass(SequentialReader);
public:
			SequentialReader(const IOObj&,
					 const TrcKeyZSampling* =0,
					 const TypeSet<int>* components=0);
			/*!< For 2D data, pass line GeomID as lineNr in
			     TrcKeySampling
			*/
			~SequentialReader();

    void		setComponents( const TypeSet<int>& compnrs )
			{ components_ = compnrs; }
    mDeprecated bool	setOutputComponents(const TypeSet<int>&);
			/*!< Will be removed after 6.2 */

    void		setComponentScaler(const Scaler&,int compidx);
			/*!< Will force the datapack to float */

    void		setDataChar(DataCharacteristics::UserType);
    void		setScaler(Scaler*);

    bool		setDataPack(RegularSeisDataPack&,od_ostream* strm=0);
			/*!< No need for init if setDataPack is called
			     Will allocate memory if not done already
			     Scaler and sampling get forwarded to the reader
			     DataChar is not forwarded
			*/

    RegularSeisDataPack* getDataPack();

    uiString		uiMessage() const	{ return msg_; }
    uiString		uiNrDoneText() const;
    od_int64		nrDone() const		{ return nrdone_; }
    od_int64		totalNr() const		{ return totalnr_; }

    bool		init();

protected:

    bool		setOutputComponents();
    virtual int		nextStep();
    virtual bool	goImpl(od_ostream*,bool,bool,int);

private:

    void		adjustDPDescToScalers(const BinDataDesc& trcdesc);
    bool		getTrcsPosForRead(int& desirednrpos,
					  TypeSet<TrcKey>&) const;
    void		submitUdfWriterTasks();

    IOObj*			ioobj_;
    bool			is2d_;
    SeisTrcReader&		rdr_;
    SelData*			sd_;
    RegularSeisDataPack*	dp_;
    TrcKeyZSampling		tkzs_;
    TypeSet<int>		components_;
    Interval<int>		samprg_;
    PosInfo::CubeData*		trcssampling_;
    PosInfo::CubeDataIterator*	trcsiterator3d_;
    bool			samedatachar_;
    StepInterval<float>		dpzsamp_;
    bool			needresampling_;
    DataCharacteristics		dc_;
    Scaler*			scaler_;
    ObjectSummary*		seissummary_;

    int				queueid_;

    od_int64			totalnr_;
    od_int64			nrdone_;
    uiString			msg_;
    bool			initialized_;
    TypeSet<int>		outcomponents_;
    ObjectSet<Scaler>		compscalers_;
};


/*!\brief Buffer to a set of entire traces ( header + component data )
	  Can contain traces for several positions. */


mClass(Seis) RawTrcsSequence
{ mODTextTranslationClass(Seis::RawTrcsSequence);
public:
			RawTrcsSequence(const ObjectSummary&,int nrpos);
			RawTrcsSequence(const RawTrcsSequence&);
			~RawTrcsSequence();

    RawTrcsSequence&	operator =(const RawTrcsSequence&);

    bool		isOK() const;
    bool		isPS() const;
    const DataCharacteristics	getDataChar() const;

    const StepInterval<float>&	getZRange() const;
    int			nrPositions() const;
    float		get(int idx,int pos,int comp) const;
    float		getValue(float,int pos,int comp) const;

    void		set(int idx,float val,int pos,int comp);
    void		setPositions(const TypeSet<TrcKey>&); //Becomes mine
    void		copyFrom(const SeisTrc&,int* ipos=0);
    void		copyFrom(const SeisTrcBuf&)		{}

    //No checks
    const unsigned char* getData(int ipos,int icomp,int is=0) const;
    unsigned char*	getData(int ipos,int icomp,int is=0);
    const TrcKey&	getPosition(int ipos) const;

private:

    const ValueSeriesInterpolator<float>&	interpolator() const;

    ObjectSet<TraceData>	data_;
    const ObjectSummary&	info_;
    const TypeSet<TrcKey>*	tks_;
    const int			nrpos_;

    mutable PtrMan<ValueSeriesInterpolator<float> >	intpol_;

public:

		// Special users only

    TraceData&		getTraceData( int pos ) { return *(data_[pos]); }

};


/*!> Seismic traces conforming the ValueSeries<float> interface.

  One of the components of a RawTrcsSequence can be selected to form
  a valueSeries

*/


mClass(Seis) RawTrcsSequenceValueSeries : public ValueSeries<float>
{
public:
			RawTrcsSequenceValueSeries(const RawTrcsSequence&,
						   int pos, int comp);
			~RawTrcsSequenceValueSeries();

    ValueSeries<float>* clone() const;

    inline void		setPosition( int pos )		{ ipos_ = pos; }
    inline void		setComponent( int idx )		{ icomp_ = idx; }
    void		setValue(od_int64,float);
    float*		arr();

    float		value(od_int64) const;
    bool		writable() const		{ return true; }
    const float*	arr() const;

private:

    RawTrcsSequence&	seq_;
    int			ipos_;
    int			icomp_;
};

} // namespace Seis

#endif
