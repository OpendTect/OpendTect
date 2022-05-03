#pragma once

/*+
 _________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Wayne Mogg
 Date:		November 2019
 ________________________________________________________________________

 -*/

#include "networkmod.h"

#include "batchjobdispatch.h"
#include "enums.h"
#include "namedobj.h"
#include "networkcommon.h"

class od_ostream;
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
    Encapsulates information of an external application/service


*/
namespace Network
{

typedef int		ProcID;

mExpClass(Network) Service : public NamedObject
{  mODTextTranslationClass(Service);
public:

    typedef ProcID	ID;
    typedef Batch::ID	SubID;

    enum ServType	{ ODGui, ODBatchGui, ODBatch, ODTest, Python, Other };
			mDeclareEnumUtils(ServType);

			Service(PortNr_Type,const char* hostnm=nullptr);
			Service(const Authority&);
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
    bool		isBatch() const;

    ID			getID() const;
    SubID		getSubID() const	{ return subid_; }
    ProcID		PID() const		{ return pid_; }
    BufferString	url() const;
    Authority		getAuthority() const	{ return auth_; }
    BufferString	address() const;
    PortNr_Type		port() const;
    ServType		type() const		{ return type_; }
    BufferString	logFnm() const;
    uiRetVal		message() const		{ return msg_; }
    void		fillJSON(OD::JSON::Object&) const;

    void		setAuthority(const Authority&);
    void		setPort(PortNr_Type);
    void		setHostName(const char*);
    void		setLogFile(const char*);
    void		setLogFile(const FilePath&);
    void		setPID(const OS::CommandLauncher&);
    void		setPID(ProcID);
    void		setType( ServType typ ) { type_ = typ; }
    uiRetVal		useJSON(const OD::JSON::Object&);
    void		stop(bool removelog=true);
    void		setEmpty();
    void		setViewOnly( bool yn=true )	{ viewonly_ = yn; }

    static BufferString getServiceName(const OD::JSON::Object&);
    static ID		getID(const OD::JSON::Object&);

    static const char*	sKeyServiceName()	{ return "servicename"; }
    static const char*	sKeyServiceType()	{ return "servicetype"; }
    static const char*	sKeyPID()		{ return "pid"; }
    static const char*	sKeySubID();
    static const char*	sKeyLogFile()		{ return "logfile"; }

    void		printInfo(const char* ky=nullptr,
				  od_ostream* =nullptr) const;

private:

    void		init();

    Authority		auth_;
    ProcID		pid_	= 0;
    SubID		subid_	= Batch::JobDispatcher::getInvalid();
    FilePath*		logfp_	= nullptr;
    bool		viewonly_ = false;
    ServType		type_;

    mutable uiRetVal	msg_;

};

};
