#ifndef remcommhandler_h
#define remcommhandler_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Ranojay Sen
 Date:          May 2010
 RCS:           $Id: remcommhandler.h,v 1.2 2010-09-13 09:21:25 cvsnanne Exp $
________________________________________________________________________

-*/

#include "callback.h"

class BufferString;
class IOPar;
class TcpServer;

mClass RemCommHandler : public CallBacker
{
public:
			RemCommHandler(int port);
		   	~RemCommHandler();

    void	   	listen() const; //!< Has to be called

protected:

    void	  	dataReceivedCB(CallBacker*);
    bool		mkCommand(const IOPar&,BufferString&);
    void		uiErrorMsg(const char*);

    const int	   	port_;  
    TcpServer&   	server_;
};

#endif
