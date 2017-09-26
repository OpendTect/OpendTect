#pragma once

/*
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		Nov 2016
________________________________________________________________________

*/

#include "databuf.h"
#include "datachar.h"
#include "dbkey.h"
#include "seistype.h"
#include "survgeom.h"
#include "threadlock.h"
#include "trckeyzsampling.h"
#include "valseriesinterpol.h"

class Scaler;
class SeisTrc;
class SeisTrcBuf;
class SeisTrcInfo;
class SeisTrcTranslator;
class TraceData;
namespace PosInfo { class CubeData; class Line2DData; }


namespace Seis
{

class Fetcher;
class Fetcher2D;
class Fetcher3D;
class ObjectSummary;
class RawTrcsSequence;
class SelData;


/*!\brief is the access point for seismic traces. Instantiate a subclass and ask
  for what you need. The class can fetch multi-threaded, it is (should be)
  MT-safe.

 After instantiation, provide the DBKey with setInput. Then you can ask
 questions about the geometry and components of the seismic object.

 By default, you will get all stored components. If you want just one,
 use selectComponent(). You can have the data resampled; just use
 setSampleInterval().

 You can get all components in a gather, or the entire gather in one trace as
 components. Just call the corresponding get or getNext function.

 Note: the getNext() function may return !isOK(). At end of input, this will
 return the special 'error' 'Finished' which can be checked by
 isFinished( uirv ).

  */


mExpClass(Seis) Provider
{ mODTextTranslationClass(Seis::Provider);
public:

    static Provider*	create(Seis::GeomType);
    static Provider*	create(const DBKey&,uiRetVal* uirv=0);
    static Provider*	create(const IOPar&,uiRetVal* uirv=0);
    static DBKey	dbKey(const IOPar&);
    virtual		~Provider();

    uiRetVal		setInput(const DBKey&);

    virtual GeomType	geomType() const		= 0;
    bool		is2D() const	{ return Seis::is2D(geomType()); }
    bool		isPS() const	{ return Seis::isPS(geomType()); }
    BufferString	name() const;
    Pos::GeomID		firstGeomID() const	{ return curGeomID(); }
    DBKey		dbKey() const		{ return dbky_; }
    ZSampling		getZRange() const	{ return doGetZRange(); }
    uiRetVal		getComponentInfo(BufferStringSet&,DataType* dt=0) const;
    int			nrOffsets() const; //!< at a representative location
					   //!< always 1 for post-stack data

    void		setSelData(SelData*); //!< becomes mine
    void		setSampleInterval(float);
    void		selectComponent(int);
    void		selectComponents(const TypeSet<int>&);
    void		forceFPData(bool yn=true);
    void		setReadMode(ReadMode);
    uiRetVal		goTo(const TrcKey&);
    uiRetVal		fillPar(IOPar&) const;
    uiRetVal		usePar(const IOPar&);

    bool		isPresent( const TrcKey& tk ) const
			{ return doGetIsPresent( tk ); }
    uiRetVal		getNext(SeisTrc&) const;
    uiRetVal		getNextGather(SeisTrcBuf&) const;
    uiRetVal		get(const TrcKey&,SeisTrc&) const;
    uiRetVal		getData(const TrcKey&,TraceData&,
				SeisTrcInfo* info=0) const;
    uiRetVal		getGather(const TrcKey&,SeisTrcBuf&) const;
    uiRetVal		getSequence(RawTrcsSequence&) const;

    const TypeSet<int>& getSelectedComponents() const	{ return selcomps_;}
    bool		haveSelComps() const;

    TrcKey		curPosition() const
			{ return doGetCurPosition(); }
    Pos::GeomID		curGeomID() const	{ return doGetCurGeomID(); }
    od_int64		nrDone() const			{ return nrdone_; }
    od_int64		totalNr() const;

    static const char*	sKeyForceFPData()
			{ return "Force FPs"; }
    static const char*	sKeySelectedComponents()
			{ return "Selected Components"; }

    static void		putTraceInGather(const SeisTrc&,SeisTrcBuf&);
			//!< components become offsets 0, 100, 200, ...
    static void		putGatherInTrace(const SeisTrcBuf&,SeisTrc&);
			//!< offsets become components

    uiRetVal		reset() const; //!< done automatically when needed
    const SelData*	selData() const		{ return seldata_; }

protected:

			Provider();

    mutable Threads::Lock lock_;
    DBKey		dbky_;
    SelData*		seldata_;
    float		zstep_;
    TypeSet<int>	selcomps_;
    ReadMode		readmode_;
    bool		forcefpdata_;
    mutable od_int64	totalnr_;
    mutable int		nrcomps_;
    mutable bool	setupchgd_;

    mutable Threads::Atomic<od_int64> nrdone_;

    void		ensureRightDataRep(TraceData&) const;
    void		ensureRightZSampling(SeisTrc&) const;
    void		ensureRightComponents(TraceData&) const;
    bool		handleSetupChanges(uiRetVal&) const;
    void		handleTrace(SeisTrc&) const;
    void		handleTraces(SeisTrcBuf&) const;

    virtual od_int64	getTotalNrInInput() const			= 0;
    virtual void	doReset(uiRetVal&) const			= 0;
    virtual TrcKey	doGetCurPosition() const			= 0;
    virtual bool	doGoTo(const TrcKey&)				= 0;
    virtual void	doFillPar(IOPar&,uiRetVal&) const;
    virtual void	doUsePar(const IOPar&,uiRetVal&)		= 0;

    virtual int		gtNrOffsets() const			{ return 1; }
    virtual uiRetVal	doGetComponentInfo(BufferStringSet&,DataType&) const;
				//!< def impl: { sKey::Data(), UnknownData }
    virtual Pos::GeomID doGetCurGeomID() const				= 0;
    virtual ZSampling	doGetZRange() const				= 0;
    virtual bool	doGetIsPresent(const TrcKey&) const;

			    // define at least either SeisTrc or SeisTrcBuf fns
    virtual void	doGetNext(SeisTrc&,uiRetVal&) const;
    virtual void	doGet(const TrcKey&,SeisTrc&,uiRetVal&) const;
    virtual void	doGetData(const TrcKey&,TraceData&,SeisTrcInfo*,
				  uiRetVal&) const;
    virtual void	doGetNextGather(SeisTrcBuf&,uiRetVal&) const;
    virtual void	doGetGather(const TrcKey&,SeisTrcBuf&,uiRetVal&) const;
    virtual void	doGetSequence(RawTrcsSequence&,uiRetVal&) const;

private:

    virtual SeisTrcTranslator*	getCurrentTranslator() const		= 0;

    friend class	Fetcher;
    friend class	Fetcher2D;
    friend class	Fetcher3D;
    friend class	ObjectSummary;

};


/*!\brief base class for Providers for 3D data. Extends Provider with some
  3D specific services. */


mExpClass(Seis) Provider3D : public Provider
{ mODTextTranslationClass(Seis::Provider3D);
public:

    virtual bool	getRanges(TrcKeyZSampling&) const		= 0;
    virtual void	getGeometryInfo(PosInfo::CubeData&) const	= 0;

protected:

			Provider3D()					{}

    virtual od_int64	getTotalNrInInput() const;
    virtual void	doFillPar( IOPar& iop, uiRetVal& uirv ) const
			{ Provider::doFillPar( iop, uirv ); }
    virtual void	doUsePar( const IOPar& iop, uiRetVal& uirv )
			{ Provider::doUsePar( iop, uirv ); }
    virtual Pos::GeomID doGetCurGeomID() const
			{ return Survey::GM().default3DSurvID(); }
    virtual ZSampling	doGetZRange() const;

    virtual SeisTrcTranslator*	getCurrentTranslator() const	{ return 0; }

};


/*!\brief base class for Providers for 2D data. Extends Provider with some
  2D specific services. */


mExpClass(Seis) Provider2D : public Provider
{ mODTextTranslationClass(Seis::Provider2D);
public:


    virtual int		nrLines() const					= 0;
    virtual Pos::GeomID	geomID(int) const				= 0;
    virtual BufferString lineName(int) const				= 0;
    virtual int		lineNr(Pos::GeomID) const			= 0;
    virtual int		curLineIdx() const				= 0;
    virtual bool	getRanges(int,StepInterval<int>&,ZSampling&) const = 0;
    virtual void	getGeometryInfo(int,PosInfo::Line2DData&) const	= 0;

protected:

			Provider2D()					{}

    virtual od_int64	getTotalNrInInput() const;
    virtual void	doFillPar( IOPar& iop, uiRetVal& uirv ) const
			{ Provider::doFillPar( iop, uirv ); }
    virtual void	doUsePar( const IOPar& iop, uiRetVal& uirv )
			{ Provider::doUsePar( iop, uirv ); }
    virtual Pos::GeomID doGetCurGeomID() const
			{ return geomID( curLineIdx() ); }
    virtual ZSampling	doGetZRange() const;

    virtual SeisTrcTranslator*	getCurrentTranslator() const	{ return 0; }

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

    const ZSampling&	getZRange() const;
    int			nrPositions() const;
    float		get(int idx,int pos,int comp) const;
    float		getValue(float,int pos,int comp) const;

    void		set(int idx,float val,int pos,int comp);
    void		setPositions(const TypeSet<TrcKey>&);	 //Becomes mine
    void		setTrcScaler(int pos,const Scaler*);
    void		copyFrom(const SeisTrc&,int* ipos=0);
    void		copyFrom(const SeisTrcBuf&)		{}

private:

    //No checks
    const DataBuffer::buf_type* getData(int ipos,int icomp,int is=0) const;
    DataBuffer::buf_type*	getData(int ipos,int icomp,int is=0);

    const ValueSeriesInterpolator<float>&	interpolator() const;

    ObjectSet<TraceData>	data_;
    ObjectSet<Scaler>		trcscalers_;
    const ObjectSummary&	info_;
    const TypeSet<TrcKey>*	tks_;
    const int			nrpos_;

    mutable PtrMan<ValueSeriesInterpolator<float> >	intpol_;
    friend class ArrayFiller;
    friend class Provider;
    friend class RawTrcsSequenceValueSeries;
};


/*!> Seismic traces conforming the ValueSeries<float> interface.

  One of the components of a RawTrcsSequence can be selected to form
  a ValueSeries.

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
