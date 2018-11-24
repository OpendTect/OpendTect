#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Satyaki Maitra
 Date:		October 2016
________________________________________________________________________

-*/

#include "generalmod.h"
#include "saveablemanager.h"
#include "probe.h"


/*!\brief Manages all stored Probe information objects.

 If a Probe is not yet loaded, then it will be loaded by fetch().

*/

mExpClass(General) ProbeManager : public SaveableManager
{ mODTextTranslationClass(ProbeManager)
public:

    ConstRefMan<Probe>	fetch(const ObjID&,uiRetVal&) const;
    RefMan<Probe>	fetchForEdit(const ObjID&,uiRetVal&);
    ConstRefMan<Probe>	fetch(const ObjID&) const;
    RefMan<Probe>	fetchForEdit(const ObjID&);

    ObjID		getID(const Probe&) const;

    uiRetVal		store(const Probe&,const TaskRunnerProvider&,
			      const IOPar* ioobjpars=0) const;
			//!< uses name to decide whether to create or replace
    uiRetVal		store(const Probe&,const ObjID&,
			      const TaskRunnerProvider&,
			      const IOPar* ioobjpars=0) const;
    uiRetVal		save(const ObjID&,const TaskRunnerProvider&) const;
    uiRetVal		save(const Probe&,const TaskRunnerProvider&) const;
    bool		needsSave(const ObjID&) const;
    bool		needsSave(const Probe&) const;

			// Use MonitorLock when iterating
    ConstRefMan<Probe>	get(idx_type) const;
    RefMan<Probe>	getForEdit(idx_type);

protected:

			ProbeManager();
			~ProbeManager();

    virtual Saveable*	getSaver(const SharedObject&) const;

    template<class T> T doFetch(const ObjID&,uiRetVal&) const;
    Probe*		gtProbe(const ObjID&) const;

public:

    mDeclareSaveableManagerInstance(ProbeManager);

};


/*!\brief access to the singleton Probe Manager */
inline ProbeManager& ProbeMGR()
{
    return ProbeManager::getInstance();
}
