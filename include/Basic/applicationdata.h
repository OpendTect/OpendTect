#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "basicmod.h"
#include "callback.h"
#include "thread.h"

mFDQtclass(QCoreApplication);
class QEventLoopReceiver;


/*!
\brief Wrapper class around the QCoreApplicaiton.

   Provides an event-loop for console applications. It can also be used in GUI
   program as an access-point (adding events and telling it to quit) to
   QApplication (which has to be created before creating the ApplicationData.
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

    static void		setOrganizationName(const char*);
    static void		setOrganizationDomain(const char*);
    static void		setApplicationName(const char*);
    static BufferString	applicationName();
protected:

    mQtclass(QCoreApplication)* application_ = nullptr;

};
