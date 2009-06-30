#ifndef odusgadmin_h
#define odusgadmin_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Bert
 Date:          Mar 2009
 RCS:           $Id: odusgadmin.h,v 1.3 2009-06-30 15:23:47 cvsbert Exp $
________________________________________________________________________

-*/

#include "namedobj.h"
#include "objectset.h"
#include <iosfwd>
class IOPar;


namespace Usage
{
class Info;
static std::ostream* logstrm_ = 0;



mClass Administrator : public ::NamedObject
{
public:

    			Administrator(const char* nm);
    virtual		~Administrator();

    virtual bool	handle(Info&)		= 0;
    const ObjectSet<IOPar>& pars() const		{ return pars_; }
    void		reInit();

    static int		add(Administrator*);
    static bool		dispatch(Info&);

    static void		setLogStream( std::ostream* s ) { logstrm_ = s; }
    			    //!< ostream does *not* become mine, can be null

protected:

    ObjectSet<IOPar>	pars_;

    void		readPars();
    void		addPars(const char*,const char*);

    bool		haveLogging() const	{ return logstrm_; }
    void		toLogFile(const char*) const;
    virtual void	reset()			{}

};


} // namespace


#endif
