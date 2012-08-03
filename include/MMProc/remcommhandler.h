#ifndef remcommhandler_h
#define remcommhandler_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Ranojay Sen
 Date:          May 2010
 RCS:           $Id: remcommhandler.h,v 1.4 2012-08-03 13:00:29 cvskris Exp $
________________________________________________________________________

-*/

#include "mmprocmod.h"
#include "callback.h"

class BufferString;
class IOPar;
class TcpServer;

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

