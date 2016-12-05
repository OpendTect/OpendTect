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

class BinDataDesc;
class BinIDValueSet;
class IOObj;
class RegularSeisDataPack;
class Scaler;
class SeisTrcReader;

template <class T> class Array2D;
template <class T> class Array3D;


namespace Seis
{

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
    bool		setOutputComponents(const TypeSet<int>&);
			/*!< If and only if different from components_
			     For instance to map the input component 3
			     with the output component 2
			     Returns false if size different from components_
			  */

    void		 setDataPack(RegularSeisDataPack*);
    RegularSeisDataPack* getDataPack();

    uiString		uiNrDoneText() const;
    uiString		uiMessage() const;

protected:
    od_int64		nrIterations() const { return totalnr_; }
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
			~SequentialReader();

    void		setComponents( const TypeSet<int>& compnrs )
			{ components_ = compnrs; }
    bool		setOutputComponents(const TypeSet<int>&);
			/*!< If and only if different from components_
			     For instance to map the input component 3
			     with the output component 2
			     Returns false if size different from components_
			 */

    void		setComponentScaler(const Scaler&,int compidx);
			/*!< Will force the datapack to float */

    void		setDataChar(DataCharacteristics::UserType);
    void		setScaler(Scaler*); //!< Scaler becomes mine

    bool		init();
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
    int			nextStep();

protected:

    void		adjustDPDescToScalers(const BinDataDesc& trcdesc);

    IOObj*			ioobj_;
    SeisTrcReader&		rdr_;
    Seis::SelData*		sd_;
    RegularSeisDataPack*	dp_;
    TrcKeyZSampling		tkzs_;
    TypeSet<int>		components_;
    Interval<int>		samprg_;
    DataCharacteristics		dc_;
    Scaler*			scaler_;

    int				queueid_;

    od_int64			totalnr_;
    od_int64			nrdone_;
    uiString			msg_;
    bool			initialized_;
};

} // namespace Seis

#endif
