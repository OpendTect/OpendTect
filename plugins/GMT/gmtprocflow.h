#ifndef gmtprocflow_h
#define gmtprocflow_h
/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : Raman Singh
 * DATE     : Sept 2008
 * ID       : $Id: gmtprocflow.h,v 1.1 2008-09-12 11:32:25 cvsraman Exp $
-*/

#include "iopar.h"
#include "namedobj.h"
#include "bufstringset.h"


namespace ODGMT
{

class ProcFlow : public ::NamedObject
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
