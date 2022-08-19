#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
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
