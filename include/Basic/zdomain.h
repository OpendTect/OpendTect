#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
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


/*!
\brief Definition of z-domain.
*/

mExpClass(Basic) Def
{
public:

    static const Def&	get(const char*);
    static const Def&	get(const IOPar&);
    void		set(IOPar&) const;	//!< Only key

    const char*		key() const		{ return key_; }
    uiString		userName() const	{ return usrnm_; }
    int			userFactor() const	{ return usrfac_; }

    uiString		getLabel() const;
			//!<returns userName plus Unit
    uiString		getRange() const;
			//!<returns userName plus Range

    const char*		unitStr(bool withparens=false) const;
			//!<In case of depth, ft or m will come from SurvInfo
    uiString		uiUnitStr(bool withparens=false) const;

    bool		isSI() const;
    bool		isTime() const;
    bool		isDepth() const;

    const char*		entityStr() const { return isTime() ? "t" : "d"; }

    bool		operator ==( const Def& def ) const
			{ return key_ == def.key_; }
    bool		operator !=( const Def& def ) const
			{ return key_ != def.key_; }

    // For plugins:
			Def( const char* ky, const uiString& usrnm,
				const char* defun, int usrfac=1 )
			    : key_(ky), usrnm_(usrnm)
			    , defunit_(defun), usrfac_(usrfac)	{}
    static bool		add(Def*);

protected:

    BufferString	key_;
    uiString		usrnm_;
    BufferString	defunit_;
    int			usrfac_; //!< usually 1 or 1000, not FeetFac
};


/*!
\brief Information of z-domain.
*/

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
    void		setID(const MultiID&);

    bool		isCompatibleWith(const IOPar&) const;

    // Convenience
    const char*		key() const		{ return def_.key(); }
    uiString		userName() const	{ return def_.userName(); }
    const char*		unitStr(bool wp=false) const
						{ return def_.unitStr(wp); }
    uiString		uiUnitStr(bool wp=false) const
						{ return def_.uiUnitStr(wp); }
    uiString		getLabel() const	{ return def_.getLabel(); }
    int			userFactor() const	{ return def_.userFactor(); }

};

mGlobal(Basic) const char*	sKey();
mGlobal(Basic) const char*	sKeyTime();
mGlobal(Basic) const char*	sKeyDepth();

} // namespace ZDomain
