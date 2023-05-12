#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "basicmod.h"
#include "enums.h"

namespace OD
{

/*!
\brief Platform or Operating System
*/

mExpClass(Basic) Platform
{
public:

    static const Platform& local();	//!< This platform

    enum Type		{ Linux, Windows, MacARM, MacIntel };
			mDeclareEnumUtils(Type)

			Platform();		//!< This platform
			Platform(Type);		//!< That platform
			Platform( const char* s, bool isshortnm )
			{ set(s,isshortnm); }

    bool		operator ==( const Platform& p ) const
			{ return type_ == p.type_; }
    bool		operator ==( const Platform::Type& t ) const
			{ return type_ == t; }

    const char*		longName() const { return toString(type_); }
    const char*		shortName() const;	//!< lux64, win64, etc.
    const char*		osName() const; // Linux, Windows, macOS

    static bool		isValidName(const char*,bool isshortnm);
    void		set(const char*,bool isshortnm);

    inline bool		isWindows() const
			{ return type_ == Windows; }
    inline bool		isLinux() const
			{ return type_ == Linux; }
    inline bool		isMac() const
			{ return type_ == MacARM || type_ == MacIntel; }

    inline Type		type() const		{ return type_; }
    inline void		setType( Type t )	{ type_ = t; }

    static const char*	sPlatform()		{ return "Platform"; }

protected:

    Type		type_;

};

} // namespace OD


#define mPlf(ptyp) OD::Platform(OD::Platform::ptyp)
#define mPlfShortName(ptyp) mPlf(ptyp).shortName()
