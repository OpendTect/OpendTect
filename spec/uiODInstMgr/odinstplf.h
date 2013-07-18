#ifndef odinstplf_h
#define odinstplf_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Apr 2010
 RCS:           $Id: odinstplf.h 8009 2013-06-20 06:11:26Z kristofer.tingdahl@dgbes.com $
________________________________________________________________________

-*/

#include "uiodinstmgrmod.h"
#include "odplatform.h"
#include "bufstring.h"


namespace ODInst
{

mDefClass(uiODInstMgr) Platform
{
public:

    static Platform	thisPlatform();

    		Platform()
		    : indep_(true)				{}
    		Platform( OD::Platform::Type t )
		    : indep_(false), plf_(t)			{}
    		Platform( const OD::Platform& p )
		    : indep_(false), plf_(p)			{}
    		Platform( bool iswin, bool is32, bool ismac=false )
		    : indep_(false), plf_(iswin,is32,ismac)	{}
    Platform&	operator =( const ODInst::Platform& p )
		    { indep_ = p.indep_; plf_ = p.plf_; return *this; }
    Platform&	operator =( const OD::Platform& p )
		    { indep_ = false; plf_ = p; return *this; }
    Platform&	operator =( OD::Platform::Type t )
		    { indep_ = false; plf_.setType(t); return *this; }
    bool	operator ==( const ODInst::Platform& p ) const
		    { return indep_ == p.indep_ && plf_ == p.plf_; }
    bool	operator ==( const OD::Platform& p ) const
		    { return !indep_ && plf_ == p; }
    bool	operator ==( OD::Platform::Type& t ) const
		    { return !indep_ && plf_ == t; }


    bool	isIndep() const			{ return indep_; }
    inline OD::Platform::Type type() const	{ return plf_.type(); }
    void	setIsIndep( bool yn )		{ indep_ = yn; }
    inline void	setType( OD::Platform::Type t )
					{ indep_ = false; plf_.setType(t); }

    const char*	longName() const;
    const char*	shortName() const;
    void set(const char*,bool isshortnm);
    void set( bool iswin, bool is32, bool ismac=false )
		    { indep_ = false; plf_.set(iswin,is32,ismac); }

    inline bool isWindows() const
		    { return !indep_ && plf_.isWindows(); }
    inline bool isLinux() const
		    { return !indep_ && plf_.isLinux(); }
    inline bool isMac() const
		    { return !indep_ && plf_.isMac(); }
    inline bool is32Bits() const
		    { return !indep_ && plf_.is32Bits(); }
    inline bool is64Bits() const
		    { return !indep_ && !plf_.is32Bits(); }
    BufferString    getPlfSubDir() const;

protected:

    OD::Platform	plf_;
    bool		indep_;

};

#define mInstPlf(ptyp) ODInst::Platform(OD::Platform::ptyp)


} // namespace

#endif

