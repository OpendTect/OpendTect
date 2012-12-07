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

mClass(uiIo) uiIOObj
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


/*!
\defgroup uiIo uiIo
  User Interface related to I/O

  This module contains some basic classes that handle the selection and
  management of an IOObj. An IOObj contains all info necessary to be able
  to load or store an object from disk. In OpendTect, users select IOObj's,
  not files (at least most often not direct but via na IOObj entry). Users
  recognise the IOObj by its name. Every IOObj has a unique identifier, the
  IOObj's key() which is a MultiID.

  In order to make the right selection, the IOObj selectors must know the
  context of the selection: what type of object, is it for read or write,
  should the user be able to create a new entry, and so forth. That's why
  you have to pass a CtxtIOObj in many cases.

  Other objects have been stuffed into this module as there was space left.
  More seriously, one can say that those objects are too OpendTect specific for
  the uiTools directory, but too general for any specific UI directory.

*/


#endif

