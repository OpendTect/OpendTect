#ifndef odftp_h
#define odftp_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:          August 2006
 RCS:           $Id$
________________________________________________________________________

-*/


#include "networkmod.h"
#include "callback.h"
#include "bufstringset.h"

class QFile;
class QFtp;
class QFtpConnector;

mClass(Network) ODFtp : public CallBacker
{
friend class QFtpConnector;

public:
    			ODFtp();
			~ODFtp();

    int			connectToHost(const char* host,int port=21);
    int			login(const char* usrnm=0,const char* passwd=0);
    int			close();
    void		abort();
    bool		hasPendingCommands() const;

    int			get(const char* file,const char* dest);
    int			put(const char* file);

    int			cd(const char* dir);
    int			list();
    int			rename(const char* oldname,const char* newname);
    int			remove(const char* file);
    int			mkdir(const char* dir);
    int			rmdir(const char* dir);

    od_int64		bytesAvailable() const;
    BufferString	readBuffer() const;

    od_int64		nrDone() const		{ return nrdone_; }
    od_int64		totalNr() const		{ return totalnr_; }
    const BufferStringSet& files() const	{ return files_; }

    bool		isOK() const		{ return !error_; }
    void		setMessage(const char*);
    const char*		message() const		{ return message_.buf(); }
    int			commandID() const	{ return commandid_; }

    Notifier<ODFtp>	messageReady;
    Notifier<ODFtp>	connected;
    Notifier<ODFtp>	disconnected;
    Notifier<ODFtp>	loginDone;
    Notifier<ODFtp>	dataTransferProgress;
    Notifier<ODFtp>	transferDone;
    Notifier<ODFtp>	readyRead;
    Notifier<ODFtp>	listReady;
    Notifier<ODFtp>	done;

    Notifier<ODFtp>	commandStarted;
    Notifier<ODFtp>	commandFinished;

protected:

    QFtp*		qftp_;
    QFtpConnector*	qftpconn_;

    TypeSet<int>	getids_;
    ObjectSet<QFile>	qfiles_;

    bool		error_;
    BufferString	message_;
    od_int64		nrdone_;
    od_int64		totalnr_;
    int			commandid_;
    int			connectionstate_;
    BufferStringSet	files_;

    void		transferDoneCB(CallBacker*);
};

#endif

