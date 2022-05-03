#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Ranojay Sen
 Date:          May 2010
________________________________________________________________________

-*/

#include "mmprocmod.h"

#include "networkcommon.h"
#include "od_iosfwd.h"

class MMPServer;

/*!
\brief Handles commands to be executed remotely on a different machine.
*/

mExpClass(MMProc) RemCommHandler : public CallBacker
{ mODTextTranslationClass(RemCommHandler);
public:
			RemCommHandler(PortNr_Type);
			~RemCommHandler();
			RemCommHandler(const RemCommHandler&) = delete;
    RemCommHandler	operator=(const RemCommHandler&) = delete;

    void		writeLog(const char* msg);

protected:
    MMPServer&		server_;
    const PortNr_Type	port_;
    od_ostream&		logstrm_;
    BufferString	logfilename_;

    void		getLogFileCB(CallBacker*);
    void		dataRootChgCB(CallBacker*);
    void		startJobCB(CallBacker*);
    void		writeLogCB(CallBacker*);
    void		createLogFile();
};

