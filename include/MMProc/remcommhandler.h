#ifndef RemCommHandler_h
#define RemCommHandler_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Ranojay Sen
 Date:          May 2010
 RCS:           $Id: remcommhandler.h,v 1.1 2010-09-10 11:52:47 cvsranojay Exp $
________________________________________________________________________

-*/
#include "callback.h"

class ServerSocket;
class TcpServer;
class FilePath;
class IOPar;

mClass RemCommHandler : public CallBacker
{
public:
			RemCommHandler(const int);
		   	~RemCommHandler();
    void	   	listen() const;

protected:

    void	  	dataReceivedCB(CallBacker*);
    bool		initEnv(const IOPar&);
    void		uiErrorMsg(const char*);

    const char*	   	hostaddress_;
    const int	   	port_;  
    TcpServer&   	server_;
    FilePath&		odbinpath_;
    BufferString	procname_;
    BufferString	cmdline_;
    BufferString	batfile_;
};

#endif
