#ifndef odusgadmin_h
#define odusgadmin_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Mar 2009
 RCS:           $Id$
________________________________________________________________________

-*/

#include "usagemod.h"
#include "namedobj.h"
#include "objectset.h"
#include "odusginfo.h"
#include "od_iosfwd.h"


namespace Usage
{



mExpClass(Usage) Administrator : public ::NamedObject
{
public:

    			Administrator(const char* nm);
    virtual		~Administrator();

    virtual bool	handle(Info&)		= 0;
    virtual void	handleQuit(Info::ID&)	{}

    const ObjectSet<IOPar>& pars() const	{ return pars_; }
    void		reInit();

    static int		add(Administrator*);
    static bool		dispatch(Info&);

    static od_ostream*	logStream(); //!< can be null
    static void		setLogStream( od_ostream* s ) { logstrm_ = s; }
    			    //!< ostream does *not* become mine, can be null

protected:

    ObjectSet<IOPar>	pars_;

    void		readPars();
    void		addPars(const char*,const char*);

    bool		haveLogging() const	{ return logstrm_; }
    void		toLogFile(const char*) const;
    virtual void	reset()			{}

    static od_ostream*	logstrm_;

};


} // namespace


#endif

