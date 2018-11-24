#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		May 2001 / Mar 2016
________________________________________________________________________

-*/

#include "seismod.h"
#include "saveablemanager.h"
#include "wavelet.h"


/*!\brief Manages all stored Wavlets.  */

mExpClass(Seis) WaveletManager : public SaveableManager
{ mODTextTranslationClass(Pick::WaveletManager)
public:

    ConstRefMan<Wavelet> fetch(const ObjID&,uiRetVal&) const;
    RefMan<Wavelet>	fetchForEdit(const ObjID&,uiRetVal&);
    ConstRefMan<Wavelet> fetch(const ObjID&) const;
    RefMan<Wavelet>	fetchForEdit(const ObjID&);

    ObjID		getID(const Wavelet&) const;

    uiRetVal		store(const Wavelet&,const TaskRunnerProvider&,
			      const IOPar* ioobjpars=0) const;
			//!< uses name to decide whether to create or replace
    uiRetVal		store(const Wavelet&,const ObjID&,
			      const TaskRunnerProvider&,
			      const IOPar* ioobjpars=0) const;
    uiRetVal		save(const ObjID&,const TaskRunnerProvider&) const;
    uiRetVal		save(const Wavelet&,const TaskRunnerProvider&) const;
    bool		needsSave(const ObjID&) const;
    bool		needsSave(const Wavelet&) const;

			// Use MonitorLock when iterating
    ConstRefMan<Wavelet> get(idx_type) const;
    RefMan<Wavelet>	getForEdit(idx_type);

    bool		isScaled(const ObjID&) const;
    bool		getScalingInfo(const ObjID&,ObjID& orgid,
				DBKey& horid,DBKey& seisid,
				BufferString& lvlnm) const;
			//!< returns false if unscaled
			//!< orgid == udf when external scaling
    void		setScalingInfo(const ObjID&,
				const ObjID* orgid=0,const DBKey* horid=0,
				const DBKey* seisid=0,const char* lvlnm=0);
			//!< orgid == null => unscaled
			//!< *orgid == udf => external scaling

protected:

			WaveletManager();
			~WaveletManager();

    virtual Saveable*	getSaver(const SharedObject&) const;

    template<class T> T	doFetch(const ObjID&,uiRetVal&) const;
    Wavelet*		gtWavelet(const ObjID&) const;

public:

    mDeclareSaveableManagerInstance(WaveletManager);

};


/*!\brief access to the singleton Wavelet Manager */
inline WaveletManager& WaveletMGR()
{
    return WaveletManager::getInstance();
}
