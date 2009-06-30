#ifndef odusgserver_h
#define odusgserver_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Bert
 Date:          Mar 2009
 RCS:           $Id: odusgserver.h,v 1.2 2009-06-30 15:23:47 cvsbert Exp $
________________________________________________________________________

-*/

#include "thread.h"
#include <iosfwd>
class IOPar;


namespace Usage
{
class Info;

mClass Server : public CallBacker
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

protected:

    const IOPar&	pars_;
    int			port_;
    std::ostream&	logstrm_;
    mThreadDeclareMutexedVar(Threads::Thread*,thread_);
    ObjectSet<Info>	infos_;

    void		usePar();
    bool		doWork(CallBacker*);
    void		getRemoteInfos();
    void		sendReply(const Usage::Info&);

};

#define mUsgServDefaulPort 23948


} // namespace


#endif
