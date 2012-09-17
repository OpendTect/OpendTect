#ifndef gmtprocflow_h
#define gmtprocflow_h
/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Raman Singh
 * DATE     : Sept 2008
 * ID       : $Id: gmtprocflow.h,v 1.3 2009/07/22 16:01:27 cvsbert Exp $
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
