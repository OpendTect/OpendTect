#pragma once

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
#include "networkcommon.h"

namespace File{ class Path; };

namespace OD {
    namespace JSON {
	class Object;
    };
};

namespace OS {
    class CommandLauncher;
};


/*\brief
    Encapsulates information of an external application/service


*/

namespace Network
{

mExpClass(Network) Service : public NamedObject
{  mODTextTranslationClass(Service);
public:
    typedef PID_Type	ID;

			Service(PortNr_Type,const char* hostnm=nullptr);
			Service(const OD::JSON::Object&);
			Service(const Service&)		= delete;
			~Service();

    Service&		operator =(const Service&)	= delete;
    bool		operator ==(const Service&) const;
    bool		operator !=(const Service&) const;

    bool		isOK() const;
    bool		isEmpty() const;
    bool		isPortValid() const;
    bool		isAlive() const;

    ID			getID() const;
    BufferString	url() const;
    Authority		getAuthority() const	{ return auth_; }
    BufferString	address() const;
    PortNr_Type		port() const;
    PID_Type		PID() const		{ return pid_; }
    BufferString	logFnm() const;
    uiRetVal		message() const		{ return msg_; }
    bool		fillJSON(OD::JSON::Object&) const;

    void		setPort(PortNr_Type);
    void		setHostName(const char*);
    void		setLogFile(const char*);
    void		setPID(const OS::CommandLauncher&);
    void		setPID(PID_Type);
    uiRetVal		useJSON(const OD::JSON::Object&);
    void		stop(bool removelog=true);
    void		setEmpty();

    static bool		fillJSON(const Authority&,OD::JSON::Object&);
    static BufferString getServiceName(const OD::JSON::Object&);
    static ID		getID(const OD::JSON::Object&);

    static const char*	sKeyServiceName()	{ return "servicename"; }
    static const char*	sKeyPID()		{ return "pid"; }
    static const char*	sKeyLogFile()		{ return "logfile"; }

private:

    Authority		auth_;
    PID_Type		pid_	= 0;
    File::Path*		logfp_	= nullptr;

    mutable uiRetVal	msg_;

};

};
