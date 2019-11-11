#ifndef netservice_h
#define netservice_h

/*+
 _________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Wayne Mogg
 Date:		November 2019
 RCS:		$Id$
 ________________________________________________________________________

 -*/

#include "networkmod.h"
#include "namedobj.h"
#include "uistrings.h"

class FilePath;

namespace OD {
    namespace JSON {
	class Object;
    };
};

namespace OS {
    class CommandLauncher;
};


/*\brief
    Encapsulates information and actions for an external application or service


*/
typedef int		ProcID;
typedef unsigned short	port_nr_type;

namespace Network
{

mExpClass(Network) Service : public NamedObject
{  mODTextTranslationClass(Service);
public:
    Service();
    Service(port_nr_type portid, const char* hostnm=nullptr);
    Service(const Service&);
    ~Service();

    Service&		operator =(const Service&) = delete;
    bool		operator ==(const Service&) const;
    bool		operator !=(const Service&) const;

    bool		isEmpty() const		{ return portid_==0; }

    BufferString	hostname() const	{ return hostnm_; }
    port_nr_type	port() const		{ return portid_; }
    ProcID		PID() const		{ return pid_; }
    BufferString	url() const;
    BufferString	logFnm() const;
    uiRetVal		message() const		{ return msg_; }
    bool		fillJSON( OD::JSON::Object& ) const;
    uiRetVal		useJSON( const OD::JSON::Object& );

    void		setPort(port_nr_type portid)	{ portid_ = portid; }
    void		setHostName(const char* hostnm) { hostnm_ = hostnm; }
    void		setLogFile(const char*);
    void		setPID(const OS::CommandLauncher&);
    void		setPID(ProcID pid)		{ pid_ = pid; }

    void		setEmpty();

    static const char*	sKeyServiceName()	{ return "servicename"; }
    static const char*	sKeyPID()		{ return "pid"; }
    static const char*	sKeyLogFile()		{ return "logfile"; }



private:

    BufferString	hostnm_;
    port_nr_type	portid_ = 0;
    ProcID		pid_	= 0;
    FilePath*		logfp_	= nullptr;

    mutable uiRetVal	msg_;

};

};
#endif
