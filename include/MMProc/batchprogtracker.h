#pragma once

/*+
 _________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Prajjaval Singh
 Date:		June 2022
 ________________________________________________________________________

 -*/


#include "mmprocmod.h"
#include "netservice.h"
#include "callback.h"

mExpClass(MMProc) BatchProgramTracker : public CallBacker
{
public:
				    BatchProgramTracker();
				    ~BatchProgramTracker();

    const TypeSet<Network::Service::ID>& getServiceIDs() const;
    void			    cleanAll();
    void			    cleanProcess(const Network::Service::ID&);

protected:
    void			    unregisterProcess(CallBacker*);
    void			    registerProcess(CallBacker*);
    void			    cleanUp(CallBacker*);

private:
    TypeSet<Network::Service::ID>   serviceids_;

};


mGlobal(MMProc) BatchProgramTracker&	    eBPT();
mGlobal(MMProc) const BatchProgramTracker&  BPT();
