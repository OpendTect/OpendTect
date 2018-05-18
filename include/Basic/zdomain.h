#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nanne Hemstra & K. Tingdahl
 Date:		April 2009 / Aug 2010
________________________________________________________________________

-*/

#include "basicmod.h"
#include "bufstring.h"
#include "uistring.h"


namespace ZDomain
{

class Def;

mGlobal(Basic) const Def&	SI();
mGlobal(Basic) const Def&	Depth();
mGlobal(Basic) const Def&	Time();

mGlobal(Basic) bool		isSI(const IOPar&);
mGlobal(Basic) bool		isDepth(const IOPar&);
mGlobal(Basic) bool		isTime(const IOPar&);
mGlobal(Basic) void		setSI(IOPar&);
mGlobal(Basic) void		setDepth(IOPar&);
mGlobal(Basic) void		setTime(IOPar&);


/*!\brief Definition of z-domain.

  Because there is a factory on string-key, we can generate a secondary
  key from the string which is likely to be unique (if it aint, the inventor
  of the domain should adapt the string key).

*/

mExpClass(Basic) Def
{ mODTextTranslationClass(Def)
public:

    typedef od_uint16	GenID;

    static const Def&	get(const char*);
    static const Def&	get(const IOPar&);
    static const Def&	get(GenID);
    void		set(IOPar&) const;	//!< Only key

    const char*		key() const		{ return key_; }
    GenID		genID() const;
    uiString		userName() const	{ return usrnm_; }
    int			userFactor() const	{ return usrfac_; }

    uiString		getLabel() const;
			//!<Username and unit
    uiString		getRange() const;
			//!< <username> Range

    const char*		fileUnitStr(bool withparens=false) const;
			//In case of depth, ft or m will come from SurvInfo
    uiString		unitStr() const;

    bool		isSI() const;
    bool		isTime() const;
    bool		isDepth() const;

    const char*		entityStr() const { return isTime() ? "t" : "d"; }

    bool		operator==( const Def& def ) const
			{ return key_ == def.key_; }
    bool		operator!=( const Def& def ) const
			{ return !(*this==def); }

    // For plugins:
			Def( const char* ky, const uiString& usrnm,
				const char* defun, int usrfac=1 )
			    : key_(ky), usrnm_(usrnm)
			    , defunit_(defun), usrfac_(usrfac)	{}
    static bool		add(Def*);

protected:

    BufferString	key_;
    BufferString	defunit_;
    uiString		usrnm_;
    int			usrfac_; // usually 1 or 1000, not FeetFac

public:

    mDeprecated uiString	uiUnitStr( bool wp=false ) const
				{
				    uiString res = unitStr();
				    if ( wp )
					res.parenthesize();
				    return res;
				}
};


/*!\brief Information of z-domain. */

mExpClass(Basic) Info
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

    bool		isCompatibleWith(const ZDomain::Info&) const;
    bool		isCompatibleWith(const IOPar&) const;

    // Convenience
    const char*		key() const		{ return def_.key(); }
    uiString		userName() const	{ return def_.userName(); }
    const char*		fileUnitStr(bool wp=false) const
						{ return def_.fileUnitStr(wp); }
    uiString		unitStr() const		{ return def_.unitStr(); }
    uiString		getLabel() const	{ return def_.getLabel(); }
    int			userFactor() const	{ return def_.userFactor(); }


    mDeprecated uiString	uiUnitStr(bool wp=false) const
				{
				    uiString res = def_.unitStr();
				    if ( wp )
					res.parenthesize();
				    return res;
				}

};

mGlobal(Basic) const char*	sKey();
mGlobal(Basic) const char*	sKeyTime();
mGlobal(Basic) const char*	sKeyDepth();

} // namespace ZDomain
