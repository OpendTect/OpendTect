#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "madagascarmod.h"
#include "bufstring.h"
#include "strmdata.h"
#include "uistring.h"


namespace ODMad
{

mExtern(Madagascar) const char* sKeyMadagascar();
mExtern(Madagascar) int sKeyMadSelKey();

/*!\brief Specifies file name and optional mask filename */

mExpClass(Madagascar) FileSpec
{ mODTextTranslationClass(FileSpec);
public:

    			FileSpec(bool forread);
    bool		set(const char* fnm,const char* maskfnm=0);

    const char*		fileName() const	{ return fnm_; }
    const char*		maskFileName()		{ return maskfnm_; }

    StreamData		open() const;		//!< if !usable() -> errMsg()
    StreamData		openMask() const;	//!< if !usable() -> errMsg()
    uiString		errMsg() const		{ return errmsg_; }

    static const char*	defPath();	//!< returns Madagascar dir in survey
    static const char*	madDataPath();

    static const char*	sKeyMaskFile();

    void		fillPar(IOPar&) const;
    bool		usePar(const IOPar&);	//!< sets errMsg() if failed

protected:

    bool		forread_;
    BufferString	fnm_;
    BufferString	maskfnm_;
    mutable uiString	errmsg_;

    StreamData		doOpen(const char*) const;
    bool		fileNameOK(const char*) const;
};

} // namespace
