#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nanne Hemstra
 Date:		December 2013
________________________________________________________________________

-*/

#include "uivolumeprocessingmod.h"
#include "uiapplserv.h"
#include "uistring.h"

class IOObj;
namespace VolProc { class Chain; class uiBatchSetup; class uiChain; }

/*!
\brief Service provider for application level - VolumeProcessing
*/

mExpClass(uiVolumeProcessing) uiVolProcPartServer : public uiApplPartServer
{ mODTextTranslationClass(uiVolProcPartServer);
public:
			uiVolProcPartServer(uiApplService&);
			~uiVolProcPartServer();

    const char*		name() const			{ return "VolProc"; }

    void		doVolProc(bool is2d,const DBKey*,
				  const char* steptype=0);
    void		createVolProcOutput(bool is2d,const IOObj*);

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
