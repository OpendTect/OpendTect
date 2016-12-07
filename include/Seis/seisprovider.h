#pragma once

/*
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		Nov 2016
________________________________________________________________________

*/

#include "seistrc.h"
#include "trckeyzsampling.h"
#include "dbkey.h"
#include "atomic.h"
#include "threadlock.h"
class SeisTrcBuf;
namespace PosInfo { class CubeData; }


namespace Seis
{

class SelData;
class Fetcher;
class Fetcher2D;
class Fetcher3D;


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
    virtual		~Provider()			{}

    uiRetVal		setInput(const DBKey&);

    virtual GeomType	geomType() const		= 0;
    virtual BufferStringSet getComponentInfo() const	= 0;
    virtual ZSampling	getZSampling() const		= 0;

    void		setSubsel(const SelData&);
    void		setSampleInterval(float);
    void		selectComponent(int);
    void		forceFPData(bool yn=true);
    void		setReadMode(ReadMode);
    uiRetVal		usePar(const IOPar&);

    uiRetVal		getNext(SeisTrc&) const;
    uiRetVal		getNextGather(SeisTrcBuf&) const;
    uiRetVal		get(const TrcKey&,SeisTrc&) const;
    uiRetVal		getGather(const TrcKey&,SeisTrcBuf&) const;

    od_int64		nrDone() const			{ return nrdone_; }
    od_int64		totalNr() const;

    static const char*	sKeyForceFPData()		{ return "Force FPs"; }

    static void		putTraceInGather(const SeisTrc&,SeisTrcBuf&);
			//!< components become offsets 0, 100, 200, ...
    static void		putGatherInTrace(const SeisTrcBuf&,SeisTrc&);
			//!< offsets become components

protected:

			Provider();

    mutable Threads::Lock lock_;
    DBKey		dbky_;
    SelData*		seldata_;
    float		zstep_;
    int			selcomp_;
    ReadMode		readmode_;
    bool		forcefpdata_;
    mutable od_int64	totalnr_;
    mutable bool	setupchgd_;

    mutable Threads::Atomic<od_int64> nrdone_;

    uiRetVal		reset() const;
    void		ensureRightDataRep(SeisTrc&) const;
    void		ensureRightZSampling(SeisTrc&) const;
    bool		handleSetupChanges(uiRetVal&) const;
    void		handleTrace(SeisTrc&) const;
    void		handleTraces(SeisTrcBuf&) const;

    virtual od_int64	getTotalNrInInput() const			= 0;
    virtual void	doReset(uiRetVal&) const			= 0;
    virtual void	doUsePar(const IOPar&,uiRetVal&)		= 0;

			// define at least either SeisTrc or SeisTrcBuf fns
    virtual void	doGetNext(SeisTrc&,uiRetVal&) const;
    virtual void	doGet(const TrcKey&,SeisTrc&,uiRetVal&) const;
    virtual void	doGetNextGather(SeisTrcBuf&,uiRetVal&) const;
    virtual void	doGetGather(const TrcKey&,SeisTrcBuf&,uiRetVal&) const;

    friend class	Fetcher;
    friend class	Fetcher2D;
    friend class	Fetcher3D;

};


/*!\brief base class for Providers for 3D data. Extends Provider with some
  3D specific services. */


mExpClass(Seis) Provider3D : public Provider
{ mODTextTranslationClass(Seis::Provider3D);
public:

    virtual TrcKeySampling getHSampling() const				= 0;
    virtual void	getGeometryInfo(PosInfo::CubeData&) const	= 0;

protected:

			Provider3D()					{}

    virtual od_int64	getTotalNrInInput() const;

};


} // namespace Seis
