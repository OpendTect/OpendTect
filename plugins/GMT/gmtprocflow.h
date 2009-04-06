#ifndef gmtprocflow_h
#define gmtprocflow_h
/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : Raman Singh
 * DATE     : Sept 2008
 * ID       : $Id: gmtprocflow.h,v 1.2 2009-04-06 07:19:31 cvsranojay Exp $
-*/

#include "iopar.h"
#include "namedobj.h"
#include "bufstringset.h"


namespace ODGMT
{

mClass ProcFlow : public ::NamedObject
{
public:

    			ProcFlow(const char* nm=0);
    			~ProcFlow();

    const IOPar&	pars() const			{ return iop_; }
    IOPar&		pars()				{ return iop_; }

protected:

    IOPar		iop_;
};

} // namespace ODMad

#endif
