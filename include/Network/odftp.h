#ifndef odftp_h
#define odftp_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:          August 2006
 RCS:           $Id: odftp.h,v 1.5 2009-10-15 03:10:47 cvsnanne Exp $
________________________________________________________________________

-*/


#include "callback.h"

class QFtp;


class ODFtp : public CallBacker
{
friend class QFtpConnector;

public:
    			ODFtp();

    int			connectToHost(const char* host,int port=21);
    int			login(const char* usrnm,const char* passwd);
    int			close();
    void		abort();

    int			get(const char* file);
    int			put(const char* file);

    int			cd(const char* dir);
    int			rename(const char* oldname,const char* newname);
    int			remove(const char* file);
    int			mkdir(const char* dir);
    int			rmdir(const char* dir);

    bool		hasPendingCommands() const;
    od_int64		nrDone() const			{ return nrdone_; }
    od_int64		totalNr() const			{ return totalnr_; }

    const char*		error() const;


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
