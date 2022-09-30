#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiiomod.h"
#include "gendefs.h"
#include "uistring.h"
class IOObj;
class CtxtIOObj;


/*!
\brief Dialog letting the user select an object.
It returns an IOObj* after successful go().
*/

mExpClass(uiIo) uiIOObj
{ mODTextTranslationClass(uiIOObj);
public:
				uiIOObj(IOObj&,bool silent=false);
				~uiIOObj();

    bool			removeImpl(bool remove_entry,bool mustrmimpl,
					   bool doconfirm=true);
    //!< Removes the underlying file(s) that an IOObj describes, with warnings
    //!< if !remove_entry, the entry is kept in the omf

    static bool			fillCtio(CtxtIOObj&,bool warnifexist=true);
    //!< Creates a new default IOObj, optionally with an askGoOn if there is
    //!< already an object with that name.

protected:

    IOObj&			ioobj_;
    bool			silent_;

};
