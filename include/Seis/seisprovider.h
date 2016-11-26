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


namespace Seis
{

class SelData;


/*!\brief is the access point for seismic traces. Instantiate a subclass and ask
  for what you need.

 After instantiation, provide the DBKey with setInput. Then you can ask
 questions about the geometry and components of the seismic object.

 Use setSubsel() or a subclass-specific subsel setter before the first get()
 or getNext(). usePar() should get you in the same state directly.

  */


mExpClass(Seis) Provider
{ mODTextTranslationClass(Seis::Provider);
public:

    virtual		~Provider()				{}

    virtual bool	isRandomAccess() const			{ return true; }
    virtual GeomType	geomType() const			= 0;

    static Provider*	create(Seis::GeomType);

    virtual uiRetVal	setInput(const DBKey&)			= 0;

    virtual BufferStringSet getComponentInfo() const		= 0;
    virtual ZSampling	getZSampling() const			= 0;

    void		setSubsel(const SelData&);
    void		forceTraceDataCharacteristics( OD::FPDataRepType r )
								{ datarep_ = r;}
    uiRetVal		usePar(const IOPar&);

    // check get and getNext on isFinished( uirv )
    uiRetVal		getNext(SeisTrc&) const;
    uiRetVal		get(const TrcKey&,SeisTrc&) const;

    static const char*	sKeyForcedDataChar()
			{ return "Forced Data Characteristics"; }

protected:

			Provider();

    SelData*		subsel_;
    float		zstep_;
    OD::FPDataRepType	datarep_;

    void		ensureRightDataRep(SeisTrc&) const;
    void		ensureRightZSampling(SeisTrc&) const;

    virtual void	doUsePar(const IOPar&,uiRetVal&)		    = 0;
    virtual void	doGetNext(SeisTrc&,uiRetVal&) const		    = 0;
    virtual void	doGet(const TrcKey&,SeisTrc&,uiRetVal&) const	    = 0;

};


} // namespace Seis
