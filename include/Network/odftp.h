#ifndef odftp_h
#define odftp_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Nanne Hemstra
 Date:          August 2006
 RCS:           $Id: odftp.h,v 1.1 2006-08-17 19:49:36 cvsnanne Exp $
________________________________________________________________________

-*/


#include "callbacker.h"

class QFtp;


class ODFtp : public CallBacker
{
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
    int64		nrdone_;
    int64		totalnr_;
    int			commandid_;
    int			connectionstate_;
};

#endif
