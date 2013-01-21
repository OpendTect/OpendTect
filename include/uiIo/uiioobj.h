#ifndef uiioobj_h
#define uiioobj_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Bril
 Date:          May 2006
 RCS:           $Id$
________________________________________________________________________

-*/

#include "uiiomod.h"
#include "gendefs.h"
class IOObj;
class CtxtIOObj;


/*!
\ingroup uiIo
\brief Dialog letting the user select an object.
It returns an IOObj* after successful go().
*/

mExpClass(uiIo) uiIOObj
{
public:
				uiIOObj( IOObj& i, bool silent=false )
				    : ioobj_(i), silent_(silent)	{}

    bool			removeImpl(bool remove_entry,bool mustrmimpl);
    //!< Removes the underlying file(s) that an IOObj describes, with warnings
    //!< if !remove_entry, the entry is kept in the omf

    static bool			fillCtio(CtxtIOObj&,bool warnifexist=true);
    //!< Creates a new default IOObj, optionally with an askGoOn if there is
    //!< already an object with that name.

protected:

    IOObj&			ioobj_;
    bool			silent_;

};

#endif

