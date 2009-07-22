#ifndef odftp_h
#define odftp_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:          August 2006
 RCS:           $Id: odftp.h,v 1.4 2009-07-22 16:01:17 cvsbert Exp $
________________________________________________________________________

-*/


#include "callback.h"

class QFtp;


class ODFtp : public CallBacker
{
friend class QFtpConnector;

public:
    			ODFtp();	

    Notifier<ODFtp>	commandFinished;
    Notifier<ODFtp>	commandStarted; 
    Notifier<ODFtp>	dataTransferProgress;
    Notifier<ODFtp>	done;
    Notifier<ODFtp>	listInfo;
    Notifier<ODFtp>	readyRead;
    Notifier<ODFtp>	stateChanged;

protected:

    QFtp*		qftp_;

    bool		error_;
    od_int64		nrdone_;
    od_int64		totalnr_;
    int			commandid_;
    int			connectionstate_;
};

#endif
