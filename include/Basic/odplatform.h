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

    enum Type	{ Lin64, Win32, Win64, Mac };
		mDeclareEnumUtils(Type)

    		Platform();		//!< This platform
		Platform(Type);		//!< That platform
		Platform( const char* s, bool isshortnm )
					{ set(s,isshortnm); }
		Platform( bool iswin, bool is32, bool ismac=false )
		    			{ set(iswin,is32,ismac); }
    bool	operator ==( const Platform& p ) const
					{ return type_ == p.type_; }
    bool	operator ==( const Platform::Type& t ) const
					{ return type_ == t; }

    const char*	longName() const { return toString(type_); }
    const char*	shortName() const;	//!< mac, lux32, win64, etc.
    const char*	osName() const; // Linux, Windows, MacOS

    static bool	isValidName(const char*,bool isshortnm);
    void	set(const char*,bool isshortnm);
    inline void	set( bool iswin, bool is32, bool ismac=false )
    		{ type_ = ismac ? Mac : (iswin	? (is32 ? Win32 : Win64)
						: Lin64 ); }

    inline bool	isWindows() const
			{ return type_ == Win32 || type_ == Win64; }
    inline bool	isLinux() const
			{ return  type_ == Lin64; }
    inline bool	isMac() const
			{ return type_ == Mac; }

    inline bool	is32Bits() const
			{ return type_ != Win64 && type_ != Lin64; }

    inline Type	type() const		{ return type_; }
    inline Type& type()			{ return type_; }
    inline void	setType( Type t )	{ type_ = t; }

    static const char* sPlatform()	{ return "Platform"; }

protected:

    Type	type_;

};

} // namespace OD


#define mPlf(ptyp) OD::Platform(OD::Platform::ptyp)
#define mPlfShortName(ptyp) mPlf(ptyp).shortName()
