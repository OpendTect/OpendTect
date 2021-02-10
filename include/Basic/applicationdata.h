#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	K. Tingdahl
 Date:		Aug 2014
________________________________________________________________________

Wrapper class around QCoreApplication

-*/

#include "basicmod.h"
#include "callback.h"
#include "thread.h"
class uiString;

mFDQtclass(QCoreApplication);
class QEventLoopReceiver;


/*!Wrapper class around the QCoreApplicaiton, which provides an event-loop
   for console applications. It can also be used in GUI program as an
   access-point (adding events and telling it to quit) to QApplication (which
   has to be created before creating the ApplicationData.
*/

mExpClass(Basic) ApplicationData : public CallBacker
{
public:
			ApplicationData();
			/*!<Will create a QCoreApplication if no
			    QCoreApplication (or inheriting classes
			    are instantiated.) */
			~ApplicationData();

    static bool		hasInstance();

    static int		exec();
			//!<Starts the event loop

    static void		exit(int returncode);
			//!<Tells the eventloop to quit.

    static bool		isCommandAndCTRLSwapped();
			//!<On Mac, is Ctrl and Command buttons swapped?
    static void		swapCommandAndCTRL(bool);

    static void		setOrganizationName(const uiString&);
    static void		setOrganizationDomain(const char*);
    static void		setApplicationName(const uiString&);

protected:

    mQtclass(QCoreApplication)* application_ = nullptr;

};
