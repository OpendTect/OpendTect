#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uivolumeprocessingmod.h"
#include "uiapplserv.h"
#include "uistring.h"

class IOObj;
class MultiID;
namespace VolProc { class Chain; class uiBatchSetup; class uiChain; }

/*!
\brief Service provider for application level - VolumeProcessing
*/

mExpClass(uiVolumeProcessing) uiVolProcPartServer : public uiApplPartServer
{ mODTextTranslationClass(uiVolProcPartServer);
public:
			uiVolProcPartServer(uiApplService&);
			~uiVolProcPartServer();

    const char*		name() const override		{ return "VolProc"; }

    void		doVolProc(const MultiID*,const char* steptype=0,
				  bool is2d=false);
    void		createVolProcOutput(const IOObj*,bool is2d=false);

    void		fillPar(IOPar&) const;
    bool		usePar(const IOPar&);

protected:

    void		volprocchainDlgClosed(CallBacker*);
    VolProc::Chain*	volprocchain_;
    VolProc::Chain*	volprocchain2d_;
    VolProc::uiChain*	volprocchaindlg_;
    VolProc::uiChain*	volprocchaindlg2d_;
    VolProc::uiBatchSetup*	volprocdlg_;
    VolProc::uiBatchSetup*	volprocdlg2d_;
};
