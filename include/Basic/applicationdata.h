#ifndef applicationdata_h
#define applicationdata_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	K. Tingdahl
 Date:		Aug 2014
 RCS:		$Id: genc.h 33946 2014-04-01 12:34:27Z nanne.hemstra@dgbes.com $
________________________________________________________________________

Wrapper class around QCoreApplication

-*/

#include "basicmod.h"

#include "ptrman.h"

mFDQtclass(QCoreApplication);

/*!Wrapper class around the QCoreApplicaiton, which provides an event-loop
   for console applications.
*/

mExpClass(Basic) ApplicationData
{
public:
    			ApplicationData();

    bool		exec();

    static void		setOrganizationName(const char*);
    static void		setOrganizationDomain(const char*);
    static void		setApplicationName(const char*);

protected:
    PtrMan<mQtclass(QCoreApplication)>	application_;
};


#endif

