#ifndef gmtprocflow_h
#define gmtprocflow_h
/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Raman Singh
 * DATE     : Sept 2008
 * ID       : $Id: gmtprocflow.h,v 1.4 2012-08-03 13:01:31 cvskris Exp $
-*/

#include "gmtmod.h"
#include "iopar.h"
#include "namedobj.h"
#include "bufstringset.h"


namespace ODGMT
{

mClass(GMT) ProcFlow : public ::NamedObject
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

