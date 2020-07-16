#pragma once
/*
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	K. Tingdahl
 Date:		July 2010
________________________________________________________________________

*/

#include "seiscommon.h"

#include "atomic.h"
#include "datachar.h"
#include "executor.h"
#include "paralleltask.h"
#include "survgeom.h"
#include "linesubsel.h"
#include "cubesubsel.h"
#include "uistring.h"

class BinDataDesc;
class BinnedValueSet;
class RegularSeisDataPack;
class Scaler;

class GatherSetDataPack;
namespace PosInfo { class LineCollData; class LineCollDataIterator; }

namespace Seis
{

class ObjectSummary;
class Provider;
class SelData;

mExpClass(Seis) Loader : public CallBacker
{ mODTextTranslationClass(Loader)
public:

    mUseType( PosInfo,	LineCollData );
    mUseType( PosInfo,	LineCollDataIterator );
    mUseType( Survey,	GeomSubSel );

    virtual		~Loader();

    void		setDataChar(DataCharacteristics::UserType);

    void		setComponents(const TypeSet<int>&);
    void		setComponentScaler(const Scaler&,int compidx);
			/*!< Will force the datapack to float */

    void		setScaler(const Scaler*);

    ConstRefMan<RegularSeisDataPack>	getDataPack();

    virtual uiString	nrDoneText() const;
    virtual uiString	message() const			{ return msg_; }

    virtual od_int64	totalNr() const { return reqss_->totalSize(); }

protected:

			Loader(const IOObj&,const GeomSubSel*,
			       const TypeSet<int>* components);

    bool		setOutputComponents();
    void		setTracePositionsFromProv(const Provider&);
    void		adjustDPDescToScalers(const BinDataDesc&);
    void		submitUdfWriterTasks();
    void		releaseDP();

    bool		is2d_;
    RefMan<RegularSeisDataPack> dp_;
    bool		dpismine_;
    Threads::Atomic<bool> arrayfillererror_ = false;
    bool		udftraceswritefinished_;
    IOObj*		ioobj_;
    GeomSubSel*		reqss_ = nullptr;

    DataCharacteristics dc_;
    TypeSet<int>	components_;
    ObjectSet<Scaler>	compscalers_; //Same size as components_
    TypeSet<int>	outcomponents_;
    Scaler*		scaler_			= nullptr;
    ObjectSummary*	seissummary_		= nullptr;
    LineCollData*	trcposns_		= nullptr;
    od_int64		totalnr_		= -1;

    int			queueid_;

    uiString		msg_;

private:

    void		udfTracesWrittenCB(CallBacker*);

public:

    void		arrayFillerCB(CallBacker*);

};


/*!Loads a 3D Seismic volume in parallel into a RegularSeisDataPack or
   into a BinnedValueSet from a Parallel File System
*/

mExpClass(Seis) ParallelFSLoader3D : public Loader
				   , public ParallelTask
{ mODTextTranslationClass(ParallelFSLoader3D)
public:
			ParallelFSLoader3D(const IOObj&,const CubeSubSel&);
			    /*!<Calculates nr of comps and allocates cubes to
				fit the cs. */
			~ParallelFSLoader3D();

    void		setDataPack(RegularSeisDataPack*);

    virtual uiString	nrDoneText() const;
    virtual uiString	message() const;

protected:

    od_int64		nrIterations() const		{ return totalnr_; }

private:

    bool		doPrepare(int);
    bool		doWork(od_int64,od_int64,int);
    bool		executeParallel(bool);

    ObjectSet<CubeSubSel>	csss_;

};


/*!Loads a 2D Seismic volume in parallel into a RegularSeisDataPack
   from a Parallel File System
*/

mExpClass(Seis) ParallelFSLoader2D : public Loader
				   , public ParallelTask
{ mODTextTranslationClass(ParallelFSLoader2D)
public:
			ParallelFSLoader2D(const IOObj&,
					   const LineSubSel&,
					   const TypeSet<int>* comps=0);
			/*!<Calculates nr of comps and allocates arrays to
			    fit the cs. */
			~ParallelFSLoader2D();

    virtual uiString	nrDoneText() const;
    virtual uiString	message() const;

protected:

    od_int64		nrIterations() const		{ return totalnr_; }

private:

    bool		doPrepare(int);
    bool		doWork(od_int64,od_int64,int);
    bool		executeParallel(bool);

    TypeSet<int>	trcnrs_;
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
					   const GeomSubSel* gss=0,
					   const TypeSet<int>* components=0);
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
    LineCollDataIterator* trcsiterator_;
    bool		samedatachar_;
    StepInterval<float> dpzsamp_;
    bool		needresampling_;

    od_int64		nrdone_;
    bool		initialized_;

};


/*
\brief Loads a Prestack Seismic Dataset into a GatherSetDataPack
*/

mExpClass(Seis) SequentialPSLoader : public Executor
{ mODTextTranslationClass(SequentialPSLoader)
public:

    mUseType( Survey,	GeomSubSel );

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

    bool		is2d_;
    Provider*		prov_;
    SelData*		sd_;
    Pos::GeomID		geomid_;
    IOObj*		ioobj_;

    od_int64		nrdone_;
    uiString		msg_;
};


} // namespace Seis
