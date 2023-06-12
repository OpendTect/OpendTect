#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
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

    void		writeLog(const char* msg);

    static uiRetVal	parseArgs(const CommandLineParser&);

protected:
			mOD_DisableCopy(RemCommHandler);

    MMPServer&		server_;
    const PortNr_Type	port_;
    od_ostream&		logstrm_;
    BufferString	logfilename_;

    void		getLogFileCB(CallBacker*);
    void		dataRootChgCB(CallBacker*);
    void		startJobCB(CallBacker*);
    void		writeLogCB(CallBacker*);
    mDeprecated("Provide removeold argument")
    void		createLogFile();
    void		createLogFile(bool removeold);
};
