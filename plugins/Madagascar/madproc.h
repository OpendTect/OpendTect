#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "madagascarmod.h"
#include "bufstringset.h"
#include "uistring.h"


namespace ODMad
{

mExpClass(Madagascar) Proc
{ mODTextTranslationClass(Proc);
public:

    enum IOType		{ Vol, VolPS, Line, LinePS, Madagascar, SegY, SU,
			  VPlot, None };

    			Proc(const char* cmd,const char* auxcmd=0);
    			~Proc();

    bool		isValid() const		{ return isvalid_; }
    IOType		inpType() const		{ return inptype_; }
    IOType		outpType() const	{ return outptype_; }
    int			nrPars() const		{ return parstrs_.size(); }
    const char*		progName() const	{ return progname_.buf(); }
    const char*		auxCommand() const	{ return auxcmd_.buf(); }

    const char*		parStr(int) const;
    const char*		getCommand() const;
    const char*		getSummary() const;
    uiString		errMsg() const		{ return errmsg_; }

    void		fillPar(IOPar&) const;
    bool		usePar(const IOPar&);

    static bool		progExists(const char*);
    static const char*	sKeyCommand()		{ return "Command"; }
    static const char*  sKeyAuxCommand()	{ return "Auxillary Command"; }
protected:

    bool		isvalid_;
    BufferString	progname_;
    BufferStringSet	parstrs_;
    BufferString	auxcmd_;
    uiString		errmsg_;
    IOType		inptype_;
    IOType		outptype_;

    void		makeProc(const char* cmd,const char* auxcmd=0);

};

} // namespace ODMad
