#ifndef zdomain_h
#define zdomain_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nanne Hemstra & K. Tingdahl
 Date:		April 2009 / Aug 2010
 RCS:		$Id: zdomain.h,v 1.12 2012-07-02 15:01:54 cvskris Exp $
________________________________________________________________________

-*/

#include "bufstring.h"
class IOPar;


namespace ZDomain
{

class Def;

mGlobal const Def&	SI();
mGlobal const Def&	Depth();
mGlobal const Def&	Time();

mGlobal bool		isSI(const IOPar&);
mGlobal bool		isDepth(const IOPar&);
mGlobal bool		isTime(const IOPar&);
mGlobal void		setSI(IOPar&);
mGlobal void		setDepth(IOPar&);
mGlobal void		setTime(IOPar&);


mClass Def
{
public:

    static const Def&	get(const char*);
    static const Def&	get(const IOPar&);
    void		set(IOPar&) const;	//!< Only key

    const char*		key() const		{ return key_; }
    const char*		userName() const	{ return usrnm_; }
    int			userFactor() const	{ return usrfac_; }

    const char*		unitStr(bool withparens=false) const;
    			//In case of depth, ft or m will come from SurvInfo

    bool		isSI() const;
    bool		isTime() const;
    bool		isDepth() const;
    
    const char*		entityStr() const { return isTime() ? "t" : "d"; }

    bool		operator ==( const Def& def ) const
			{ return key_ == def.key_; }

    // For plugins:
    			Def( const char* ky, const char* usrnm,
				const char* defun, int usrfac=1 )
			    : key_(ky), usrnm_(usrnm)
			    , defunit_(defun), usrfac_(usrfac)	{}
    static bool		add(Def*);

protected:

    BufferString	key_;
    BufferString	usrnm_;
    BufferString	defunit_;
    int			usrfac_; // usually 1 or 1000, not FeetFac
};


mClass Info
{
public:
    			Info(const Def&);
    			Info(const Info&);
    			Info(const IOPar&);
    			~Info();

    const Def&		def_;
    IOPar&		pars_;

    bool		hasID() const;
    const char*		getID() const;
    void		setID(const char*);

    bool		isCompatibleWith(const IOPar&) const;

    // Convenience
    const char*		key() const		{ return def_.key(); }
    const char*		userName() const	{ return def_.userName(); }
    const char*		unitStr(bool wp=false) const
    						{ return def_.unitStr(wp); }
    int			userFactor() const	{ return def_.userFactor(); }

};

mGlobal const char*	sKey();
mGlobal const char*	sKeyTime();
mGlobal const char*	sKeyDepth();

} // namespace ZDomain

#endif
