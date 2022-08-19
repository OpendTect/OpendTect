#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "bufstring.h"
#include "uistring.h"


namespace MMProc
{

mClass(MMProc) ExecProxy
{ mODTextTranslationClass(ExecProxy);
    			ExecProxy(const char* prognm,const char* hostnm);

    bool		launch(const IOPar&);
    enum Status		{ NotStarted, Running, Failed, Ended };
    Status		update();

    uiString		uiMessage() const	{ return msg_; }
    int			nrDone() const		{ return nrdone_; }

    const char*		progName() const	{ return prognm_; }
    const char*		hostName() const	{ return hostnm_; }

protected:

    const BufferString	prognm_;
    const BufferString	hostnm_;
    uiString		msg_;
    int			nrdone_;

};

} // namespace MMProc
