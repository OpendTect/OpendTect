#ifndef odusgserver_h
#define odusgserver_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Bert
 Date:          Mar 2009
 RCS:           $Id: odusgserver.h,v 1.1 2009-03-12 15:51:31 cvsbert Exp $
________________________________________________________________________

-*/

#include "gendefs.h"
#include <iosfwd>
class IOPar;


namespace Usage
{

mClass Server
{
public:

    			Server(const IOPar&,std::ostream&);
    virtual		~Server();

    int			port() const	{ return port_; }

    bool		go();

    static const char*	sKeyPort();
    static int		DefaulPort();

protected:

    const IOPar&	pars_;
    int			port_;
    std::ostream&	logstrm_;

};

#define mUsageServerDefaulPort 23948


} // namespace


#endif
