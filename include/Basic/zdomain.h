#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "basicmod.h"

#include "bufstring.h"
#include "odcommonenums.h"
#include "uistring.h"


namespace ZDomain
{

class Def;
class Info;

mGlobal(Basic) const Def&	SI();
mGlobal(Basic) const Def&	Depth();
mGlobal(Basic) const Def&	Time();

mGlobal(Basic) bool		isSI(const IOPar&);
mGlobal(Basic) bool		isDepth(const IOPar&);
mGlobal(Basic) bool		isTime(const IOPar&);
mGlobal(Basic) void		setSI(IOPar&);
mGlobal(Basic) void		setDepth(IOPar&);
mGlobal(Basic) void		setTime(IOPar&);

mGlobal(Basic) const Info&	TWT();
mGlobal(Basic) const Info&	DepthMeter();
mGlobal(Basic) const Info&	DepthFeet();
mGlobal(Basic) const Info*	get(const IOPar&);
				// never manage the returned pointer


/*!
\brief Definition of z-domain.
*/

mExpClass(Basic) Def
{
public:
			~Def();

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

    int			nrZDecimals(float zstep) const;

    bool		isSI() const;
    bool		isTime() const;
    bool		isDepth() const;

    const char*		entityStr() const { return isTime() ? "t" : "d"; }

    bool		operator ==( const Def& def ) const
			{ return key_ == def.key_; }
    bool		operator !=( const Def& def ) const
			{ return key_ != def.key_; }

    // For plugins:
			Def(const char* ky,const uiString& usrnm,
			    const char* defun,int usrfac=1);
			Def(const Info&)	= delete;
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
			Info(const Def&,const char* unitstr=nullptr);
			Info(const Info&);
			Info(const IOPar&);
			~Info();

    bool		operator ==(const Info&) const;
    bool		operator !=(const Info&) const;

    bool		fillPar(IOPar&) const;
   // No usePar(const IOPar&), use ZDomain::get(const IOPar&) or the constructor
   // Returns true if changed

    const Def&		def_;
    IOPar&		pars_;

    bool		hasID() const;
    const MultiID	getID() const;
    void		setID(const MultiID&);
    void		setDepthUnit(DepthType);

    bool		isCompatibleWith(const Info&) const;
    bool		isCompatibleWith(const IOPar&) const;
    Interval<float>	getReasonableZRange(bool foruser=false) const;


    // Convenience
    const char*		key() const		{ return def_.key(); }
    uiString		userName() const	{ return def_.userName(); }
    const char*		unitStr(bool wp=false) const;
    uiString		uiUnitStr(bool wp=false) const;
    uiString		getLabel() const	{ return def_.getLabel(); }
    int			userFactor() const	{ return def_.userFactor(); }

    bool		isTime() const		{ return def_.isTime(); }
    bool		isDepth() const		{ return def_.isDepth(); }
    TimeType		timeType() const;  // Only valid for Time
    DepthType		depthType() const; // Only valid for Depth
    bool		isDepthMeter() const;
    bool		isDepthFeet() const;

    mDeprecated("Use MultiID Overloaded function")
    void		setID(const char*);
};

mGlobal(Basic) const char*	sKey();
mGlobal(Basic) const char*	sKeyTime();
mGlobal(Basic) const char*	sKeyDepth();
mGlobal(Basic) const char*	sKeyUnit();

} // namespace ZDomain
