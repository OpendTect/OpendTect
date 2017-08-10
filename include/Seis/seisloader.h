#pragma once
/*
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	K. Tingdahl
 Date:		July 2010
________________________________________________________________________

*/

#include "seiscommon.h"
#include "datachar.h"
#include "executor.h"
#include "paralleltask.h"
#include "survgeom.h"
#include "trckeyzsampling.h"
#include "uistring.h"

class BinDataDesc;
class BinIDValueSet;
class IOObj;
class RegularSeisDataPack;
class Scaler;

class GatherSetDataPack;
namespace PosInfo { class CubeData; class CubeDataIterator;
		    class Line2DData; class Line2DDataIterator; }

namespace Seis
{

class ObjectSummary;
class Provider;
class SelData;

mExpClass(Seis) Loader
{ mODTextTranslationClass(Loader)
public:

    virtual		~Loader();

    void		setDataChar(DataCharacteristics::UserType);

    void		setComponents(const TypeSet<int>&);
    bool		setOutputComponents(const TypeSet<int>&);
			/*!< If and only if different from components_
			     For instance to map the input component 3
			     with the output component 2
			     Returns false if size different from components_
			  */

    void		setComponentScaler(const Scaler&,int compidx);
			/*!< Will force the datapack to float */

    void		setScaler(const Scaler*);

    virtual RegularSeisDataPack*	getDataPack()	{ return dp_; }

    virtual uiString	nrDoneText() const;
    virtual uiString	message() const			{ return msg_; }

    virtual od_int64	totalNr() const { return tkzs_.hsamp_.totalNr(); }

protected:
			Loader(const IOObj&,const TrcKeyZSampling*,
			       const TypeSet<int>* components);

    void		adjustDPDescToScalers(const BinDataDesc& trcdesc);
    void		submitUdfWriterTasks(int queueid);

    RefMan<RegularSeisDataPack> dp_;
    IOObj*		ioobj_;
    TrcKeyZSampling	tkzs_;

    DataCharacteristics dc_;
    TypeSet<int>	components_;
    ObjectSet<Scaler>	compscalers_; //Same size as components_
    TypeSet<int>*	outcomponents_;
    Scaler*		scaler_;
    ObjectSummary*	seissummary_;
    PosInfo::CubeData*	trcssampling_;

    uiString		msg_;
};


/*!Loads a 3D Seismic volume in parallel into a RegularSeisDataPack or
   into a BinIDValueSet from a Parallel File System
*/

mExpClass(Seis) ParallelFSLoader3D : public Loader
				   , public ParallelTask
{ mODTextTranslationClass(ParallelFSLoader3D)
public:
			ParallelFSLoader3D(const IOObj&,const TrcKeyZSampling&);
			/*!<Calculates nr of comps and allocates cubes to
			    fit the cs. */

			ParallelFSLoader3D(const IOObj&,BinIDValueSet&,
					   const TypeSet<int>& components);
			/*!<Will read the z from the first value. Will add
			    values to accomodate nr of components. If data
			    cannot be read, that binid/z will be set to
			    mUdf */

    void		setDataPack(RegularSeisDataPack*);

    virtual uiString	nrDoneText() const;
    virtual uiString	message() const;

protected:
    od_int64		nrIterations() const		{ return totalnr_; }

private:
    bool		doPrepare(int);
    bool		doWork(od_int64,od_int64,int);

    BinIDValueSet*	bidvals_;
    const od_int64	totalnr_;
};


/*!Loads a 2D Seismic volume in parallel into a RegularSeisDataPack
   from a Parallel File System
*/

mExpClass(Seis) ParallelFSLoader2D : public Loader
				   , public ParallelTask
{ mODTextTranslationClass(ParallelFSLoader2D)
public:
			ParallelFSLoader2D(const IOObj&,
					   const TrcKeyZSampling&,
					   const TypeSet<int>* comps=0);
			/*!<Calculates nr of comps and allocates arrays to
			    fit the cs. */

    RegularSeisDataPack* getDataPack(); // The caller now owns the datapack

    virtual uiString	nrDoneText() const;
    virtual uiString	message() const;

protected:
    od_int64		nrIterations() const		{ return totalnr_; }

private :
    bool		doPrepare(int);
    bool		doWork(od_int64,od_int64,int);

    bool		dpclaimed_;
    const od_int64	totalnr_;
};


/*!Loads a Seismic Dataset in parallel into a RegularSeisDataPack

    Usage example:
    SequentialFSLoader rdr( myiioobj ); // I want to read all
    rdr.setDataChar( DataCharacteristics() ); // read in another format
    rdr.setScaler( myscaler ); // sets the datapack scaler
    if ( rdr.init() ) // something is not right
    rdr.execute();
*/

mExpClass(Seis) SequentialFSLoader : public Loader
				   , public Executor
{ mODTextTranslationClass(SequentialFSLoader)
public:
			SequentialFSLoader(const IOObj&,
					   const TrcKeyZSampling* =0,
					   const TypeSet<int>* components=0);
			/*!< For 2D data, pass line GeomID as lineNr in
			     TrcKeySampling
			*/
			~SequentialFSLoader();

    bool		setDataPack(RegularSeisDataPack&,od_ostream* strm=0);
			/*!< No need for init if setDataPack is called
			     Will allocate memory if not done already
			     Scaler and sampling get forwarded to the reader
			     DataChar is not forwarded
			*/

    od_int64		nrDone() const			{ return nrdone_; }
    virtual od_int64	totalNr() const			{ return totalnr_; }

    virtual uiString	nrDoneText() const;
    virtual uiString	message() const;

protected:

    bool		init();
    int			nextStep();
    bool		goImpl(od_ostream*,bool,bool,int);

private:

    bool		getTrcsPosForRead(TypeSet<TrcKey>&) const;

    Provider*		prov_;
    SelData*		sd_;
    Interval<int>	samprg_;
    PosInfo::Line2DData* line2ddata_;
    PosInfo::Line2DDataIterator* trcsiterator2d_;
    PosInfo::CubeDataIterator*	trcsiterator3d_;
    od_int64		totalnr_;
    bool		samedatachar_;
    StepInterval<float> dpzsamp_;
    bool		needresampling_;

    int			queueid_;

    od_int64		nrdone_;
    bool		initialized_;

};


/*
\brief Loads a Prestack Seismic Dataset into a GatherSetDataPack
*/

mExpClass(Seis) SequentialPSLoader : public Executor
{ mODTextTranslationClass(SequentialPSLoader)
public:
			SequentialPSLoader(const IOObj&,
					   const Interval<int>* linerg=0,
					   Pos::GeomID geomid=mUdfGeomID);
			~SequentialPSLoader();

    GatherSetDataPack*	getPSDataPack()			{ return gatherdp_;}

    od_int64		totalNr() const;
    od_int64		nrDone() const			{ return nrdone_; }
    virtual uiString	nrDoneText() const;
    virtual uiString	message() const			{ return msg_; }

protected:
    bool		init();
    int			nextStep();

    RefMan<GatherSetDataPack>	gatherdp_;

    Provider*		prov_;
    SelData*		sd_;
    Pos::GeomID		geomid_;
    IOObj*		ioobj_;

    od_int64		nrdone_;
    uiString		msg_;
};

} // namespace Seis
