#ifndef uivolprocpartserv_h
#define uivolprocpartserv_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nanne Hemstra
 Date:		December 2013
 RCS:		$Id$
________________________________________________________________________

-*/

#include "uivolumeprocessingmod.h"
#include "uiapplserv.h"
#include "uistring.h"

class IOObj;
class MultiID;
namespace VolProc { class Chain; class uiChain; }

/*!
\brief Service provider for application level - VolumeProcessing
*/

mExpClass(uiVolumeProcessing) uiVolProcPartServer : public uiApplPartServer
{ mODTextTranslationClass(uiVolProcPartServer);
public:
			uiVolProcPartServer(uiApplService&);
			~uiVolProcPartServer();

    const char*		name() const			{ return "VolProc"; }

    void		doVolProc(const MultiID*,const char* steptype=0);
    void		createVolProcOutput(const IOObj*);

    void		fillPar(IOPar&) const;
    bool		usePar(const IOPar&);

protected:

    void		volprocchainDlgClosed(CallBacker*);
    VolProc::Chain*	volprocchain_;
    VolProc::uiChain*	volprocchaindlg_;
};

#endif

