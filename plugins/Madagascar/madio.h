#ifndef madio_h
#define madio_h
/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : Bert
 * DATE     : Sep 2007
 * ID       : $Id: madio.h,v 1.5 2009-04-06 07:24:44 cvsranojay Exp $
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
    const char*		errMsg() const		{ return errmsg_; }

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
