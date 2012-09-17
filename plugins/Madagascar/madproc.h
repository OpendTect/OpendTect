#ifndef madproc_h
#define madproc_h
/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Raman Singh
 * DATE     : Sept 2008
 * ID       : $Id: madproc.h,v 1.4 2009/07/22 16:01:27 cvsbert Exp $
-*/


#include "bufstringset.h"

class IOPar;

namespace ODMad
{

mClass Proc
{
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
    BufferString	errMsg() const		{ return errmsg_; }

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
    BufferString	errmsg_;
    IOType		inptype_;
    IOType		outptype_;

    void		makeProc(const char* cmd,const char* auxcmd=0);

};

} // namespace ODMad

#endif
