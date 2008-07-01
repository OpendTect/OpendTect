#ifndef mmexecproxy_h
#define mmexecproxy_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Bert
 Date:		Jul 2008
 RCS:		$Id: mmexecproxy.h,v 1.1 2008-07-01 14:18:23 cvsbert Exp $
________________________________________________________________________

-*/

#include "bufstring.h"
class IOPar;

namespace MMProc
{


class ExecProxy
{
    			ExecProxy(const char* prognm,const char* hostnm);

    bool		launch(const IOPar&);
    enum Status		{ NotStarted, Running, Failed, Ended };
    Status		update();

    const char*		message() const		{ return msg_; }
    int			nrDone() const		{ return nrdone_; }

    const char*		progName() const	{ return prognm_; }
    const char*		hostName() const	{ return hostnm_; }

protected:

    const BufferString	prognm_;
    const BufferString	hostnm_;
    BufferString	msg_;
    int			nrdone_;

};


}; // namespace MMProc


#endif
