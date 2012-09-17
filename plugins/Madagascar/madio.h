#ifndef madio_h
#define madio_h
/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert
 * DATE     : Sep 2007
 * ID       : $Id: madio.h,v 1.7 2010/07/13 21:10:30 cvskris Exp $
-*/

#include "bufstring.h"
#include "strmdata.h"
class IOPar;


namespace ODMad
{

mExtern const char* sKeyMadagascar();
mExtern const char* sKeyMadSelKey();

/*!\brief Specifies file name and optional mask filename */

mClass FileSpec
{
public:

    			FileSpec(bool forread);
    bool		set(const char* fnm,const char* maskfnm=0);

    const char*		fileName() const	{ return fnm_; }
    const char*		maskFileName()		{ return maskfnm_; }

    StreamData		open() const;		//!< if !usable() -> errMsg()
    StreamData		openMask() const;	//!< if !usable() -> errMsg()
    const char*		errMsg() const		{ return errmsg_.str(); }

    static const char*	defPath();	//!< returns Madagascar dir in survey
    static const char*	madDataPath();

    static const char*	sKeyMaskFile();

    void		fillPar(IOPar&) const;
    bool		usePar(const IOPar&);	//!< sets errMsg() if failed

protected:

    bool		forread_;
    BufferString	fnm_;
    BufferString	maskfnm_;
    mutable BufferString errmsg_;

    StreamData		doOpen(const char*) const;
    bool		fileNameOK(const char*) const;
};

} // namespace

#endif
