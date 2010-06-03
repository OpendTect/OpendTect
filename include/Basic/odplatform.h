#ifndef odplatform_h
#define odplatform_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          May 2010
 RCS:           $Id: odplatform.h,v 1.4 2010-06-03 08:20:26 cvsbert Exp $
________________________________________________________________________

-*/

#include "enums.h"

namespace OD
{

mClass Platform
{
public:

    virtual		~Platform()		{};

    static const Platform&	local();	//!< This platform

    enum Type		{ Lin32, Lin64, Win32, Win64, Mac };
			DeclareEnumUtils(Type)

    			Platform();		//!< This platform
    			Platform( Type t )	//!< That platform
			    : type_(t)		{}
			Platform( const char* s, bool isshortnm )
						{ set(s,isshortnm); }
			Platform( bool iswin, bool is32, bool ismac=false )
			    			{ set(iswin,is32,ismac); }
    			Platform( const Platform& p )
			    : type_(p.type_)	{}
    virtual bool        operator ==( const Platform& p ) const
			{ return type_ == p.type_; }
    virtual bool        operator ==( const Platform::Type& t ) const
			{ return type_ == t; }

    virtual const char*	longName() const
    			{ return eString(Type,type_); }
    virtual const char*	shortName() const;	//!< mac, lux32, win64, etc.

    static bool		isValidName(const char*,bool isshortnm);
    virtual void	set(const char*,bool isshortnm);
    inline virtual void	set( bool iswin, bool is32, bool ismac=false )
    			{ type_ = ismac ? Mac : (iswin ? (is32 ? Win32 : Win64)
				       : (is32 ? Lin32 : Lin64) ); }

    inline virtual bool	isWindows() const
			{ return type_ == Win32 || type_ == Win64; }
    inline virtual bool	isLinux() const
			{ return type_ == Lin32 || type_ == Lin64; }
    inline virtual bool	isMac() const
			{ return type_ == Mac; }

    inline virtual bool	is32Bits() const
			{ return type_ != Win64 && type_ != Lin64; }

    virtual Type	type() const		{ return type_; }
    virtual void	setType( Type t )	{ type_ = t; }

protected:

    Type		type_;

};

} // namespace


#define mPlf(ptyp) OD::Platform(OD::Platform::ptyp)
#define mPlfShortName(ptyp) mPlf(ptyp).shortName()


#endif
