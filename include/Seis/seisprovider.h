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
namespace PosInfo { class CubeData; }


namespace Seis
{

class SelData;


/*!\brief is the access point for seismic traces. Instantiate a subclass and ask
  for what you need. The class can fetch multi-threaded.

 After instantiation, provide the DBKey with setInput. Then you can ask
 questions about the geometry and components of the seismic object.

 By default, you will get all stored components. If you want just one,
 use selectComponent(). You can have the data resampled; just use
 setSampleInterval().

 The class is (should be) MT-safe.

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
			//!< check return on isFinished()
    uiRetVal		get(const TrcKey&,SeisTrc&) const;

    od_int64		nrDone() const			{ return nrdone_; }
    od_int64		totalNr() const;

    static const char*	sKeyForceFPData()		{ return "Force FPs"; }

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
    void		handleTrace(SeisTrc&) const;
    bool		handleSetupChanges(uiRetVal&) const;

    virtual od_int64	getTotalNrInInput() const			= 0;
    virtual void	doReset(uiRetVal&) const			= 0;
    virtual void	doUsePar(const IOPar&,uiRetVal&)		= 0;
    virtual void	doGetNext(SeisTrc&,uiRetVal&) const		= 0;
    virtual void	doGet(const TrcKey&,SeisTrc&,uiRetVal&) const	= 0;

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
