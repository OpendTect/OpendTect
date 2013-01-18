#ifndef remcommhandler_h
#define remcommhandler_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Ranojay Sen
 Date:          May 2010
 RCS:           $Id$
________________________________________________________________________

-*/

#include "mmprocmod.h"
#include "callback.h"

class BufferString;
class IOPar;
class TcpServer;

/*!
\brief Handles commands to be executed remotely on a different machine.
*/

mClass(MMProc) RemCommHandler : public CallBacker
{
public:
			RemCommHandler(int port);
		   	~RemCommHandler();

    void	   	listen() const; //!< Has to be called

protected:

    void	  	dataReceivedCB(CallBacker*);
    bool		mkCommand(const IOPar&,BufferString&);
    void		uiErrorMsg(const char*);
    std::ostream& 	createLogFile();
    void		writeLog(const char* msg);
    std::ostream&       logstrm_;

    const int	   	port_;  
    TcpServer&   	server_;
};

#endif

