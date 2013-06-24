#ifndef odusgserver_h
#define odusgserver_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Mar 2009
 RCS:           $Id$
________________________________________________________________________

-*/

#include "usagemod.h"
#include "callback.h"
#include "threadlock.h"

#include <iosfwd>

class IOPar;
namespace Threads { class Thread; }


namespace Usage
{
class Info;

mExpClass(Usage) Server : public CallBacker
{
public:

    			Server(const IOPar*,std::ostream&);
    virtual		~Server();

    const IOPar&	pars()		{ return pars_; }
    int			port() const	{ return port_; }

    bool		go(bool innewthread=false);

    static const char*	sKeyPort();
    static const char*	sKeyFileBase();
    static int		cDefaulPort();

    static IOPar*	getPars();

    void		addInfo(Info&);
    static const char*	setupFileName(const char* adm_name=0);

protected:

    const IOPar&	pars_;
    int			port_;
    std::ostream&	logstrm_;
    Threads::Thread*	thread_;
    Threads::Lock	threadlock_;
    ObjectSet<Info>	infos_;

    void		usePar();
    bool		doWork(CallBacker*);
    void		getRemoteInfos();
    void		sendReply(const Usage::Info&);

};

#define mUsgServDefaulPort 23948


} // namespace


#endif

