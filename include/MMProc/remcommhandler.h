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

namespace Network { class Server; }

/*!
\brief Handles commands to be executed remotely on a different machine.
*/

mExpClass(MMProc) RemCommHandler : public CallBacker
{ mODTextTranslationClass(RemCommHandler);
public:
			RemCommHandler(PortNr_Type);
			~RemCommHandler();

    void		listen() const; //!< Has to be called
    void		writeLog(const char* msg);

protected:

    void		dataReceivedCB(CallBacker*);
    void		doStatus(int,const IOPar&);
    od_ostream&		createLogFile();
    od_ostream&		logstrm_;

    Network::Server&	server_;
    const PortNr_Type	port_;

};

