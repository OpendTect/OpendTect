#ifndef odusgadmin_h
#define odusgadmin_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Bert
 Date:          Mar 2009
 RCS:           $Id: odusgadmin.h,v 1.2 2009-04-17 12:34:40 cvsranojay Exp $
________________________________________________________________________

-*/

#include "namedobj.h"
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
    const IOPar&	pars() const		{ return pars_; }
    void		reInit();

    static int		add(Administrator*);
    static bool		dispatch(Info&);

    static void		setLogStream( std::ostream* s ) { logstrm_ = s; }
    			    //!< ostream does *not* become mine, can be null

protected:

    IOPar&		pars_;
    void		readPars();

    bool		haveLogging() const	{ return logstrm_; }
    void		toLogFile(const char*) const;
    virtual void	reset()			{}

};


} // namespace


#endif
