#ifndef mmexecproxy_h
#define mmexecproxy_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		Jul 2008
 RCS:		$Id$
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
