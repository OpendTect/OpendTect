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


namespace Seis
{

class SelData;


/*!\brief is the access point for seismic traces. Instantiate a subclass and ask
  for what you need. The class can fetch multi-threaded.

 After instantiation, provide the DBKey with setInput. Then you can ask
 questions about the geometry and components of the seismic object.

 Use setSubsel() or a subclass-specific subsel setter before the first get()
 or getNext(). usePar() should get you in the same state directly.

 By default, you will get all stored components. If you want just one,
 use selectComponent(). You can have the data resampled; just use
 setSampleInterval().

  */


mExpClass(Seis) Provider
{ mODTextTranslationClass(Seis::Provider);
public:

    static Provider*	create(Seis::GeomType);
    static Provider*	create(const DBKey&,uiRetVal* uirv=0);
    virtual		~Provider()			{}

    virtual uiRetVal	setInput(const DBKey&)		= 0;

    virtual GeomType	geomType() const		= 0;
    virtual BufferStringSet getComponentInfo() const	= 0;
    virtual ZSampling	getZSampling() const		= 0;

    void		setSubsel(const SelData&);
    void		setSampleInterval( float zs )	{ zstep_ = zs; }
    virtual void	selectComponent( int icomp )	{ selcomp_ = icomp; }
    void		forceFPData( bool yn=true )	{ forcefpdata_ = yn; }
    void		setReadMode( ReadMode rm )	{ readmode_ = rm; }
    uiRetVal		usePar(const IOPar&);

    uiRetVal		getNext(SeisTrc&) const;
			//!< check return on isFinished()
    uiRetVal		get(const TrcKey&,SeisTrc&) const;

    od_int64		nrDone() const			{ return nrtrcs_; }

    static const char*	sKeyForceFPData()		{ return "Force FPs"; }

protected:

			Provider();

    DBKey		dbky_;
    SelData*		seldata_;
    float		zstep_;
    int			selcomp_;
    ReadMode		readmode_;
    bool		forcefpdata_;

    mutable Threads::Atomic<od_int64>	nrtrcs_;
    mutable Threads::Lock		getlock_;

    void		ensureRightDataRep(SeisTrc&) const;
    void		ensureRightZSampling(SeisTrc&) const;
    void		handleTrace(SeisTrc&) const;

    virtual void	doUsePar(const IOPar&,uiRetVal&)		    = 0;
    virtual void	doGetNext(SeisTrc&,uiRetVal&) const		    = 0;
    virtual void	doGet(const TrcKey&,SeisTrc&,uiRetVal&) const	    = 0;

};


} // namespace Seis
