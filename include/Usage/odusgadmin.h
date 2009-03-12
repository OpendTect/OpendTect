#ifndef odusgadmin_h
#define odusgadmin_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Bert
 Date:          Mar 2009
 RCS:           $Id: odusgadmin.h,v 1.1 2009-03-12 15:51:31 cvsbert Exp $
________________________________________________________________________

-*/

#include "namedobj.h"
#include <iosfwd>
class IOPar;


namespace Usage
{
class Info;

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

private:

    static std::ostream* logstrm_;

};


} // namespace


#endif
