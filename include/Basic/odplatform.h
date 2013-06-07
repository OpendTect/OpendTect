#ifndef odplatform_h
#define odplatform_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          May 2010
 RCS:           $Id$
________________________________________________________________________

-*/

#include "enums.h"

namespace OD
{

mClass Platform
{
public:

    static const Platform& local();	//!< This platform

    enum Type	{ Lin32, Lin64, Win32, Win64, Mac };
		DeclareEnumUtils(Type)

    		Platform();		//!< This platform
    		Platform( Type t )	//!< That platform
		    : type_(t)		{}
		Platform( const char* s, bool isshortnm )
					{ set(s,isshortnm); }
		Platform( bool iswin, bool is32, bool ismac=false )
		    			{ set(iswin,is32,ismac); }
    bool        operator ==( const Platform& p ) const
					{ return type_ == p.type_; }
    bool        operator ==( const Platform::Type& t ) const
					{ return type_ == t; }

    const char*	longName() const { return getTypeString(type_); }
    const char*	shortName() const;	//!< mac, lux32, win64, etc.

    static bool	isValidName(const char*,bool isshortnm);
    void	set(const char*,bool isshortnm);
    inline void	set( bool iswin, bool is32, bool ismac=false )
    		{ type_ = ismac ? Mac : (iswin	? (is32 ? Win32 : Win64)
						: (is32 ? Lin32 : Lin64) ); }

    inline bool	isWindows() const
			{ return type_ == Win32 || type_ == Win64; }
    inline bool	isLinux() const
			{ return type_ == Lin32 || type_ == Lin64; }
    inline bool	isMac() const
			{ return type_ == Mac; }

    inline bool	is32Bits() const
			{ return type_ != Win64 && type_ != Lin64; }

    inline Type	type() const		{ return type_; }
    inline Type& type()			{ return type_; }
    inline void	setType( Type t )	{ type_ = t; }

protected:

    Type	type_;

};

} // namespace


#define mPlf(ptyp) OD::Platform(OD::Platform::ptyp)
#define mPlfShortName(ptyp) mPlf(ptyp).shortName()


#endif
