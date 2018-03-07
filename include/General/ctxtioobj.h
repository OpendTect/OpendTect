#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		7-1-1996
________________________________________________________________________

-*/

#include "ioobjctxt.h"
#include "ioobj.h"


/*!\brief Holds an IOObjCtxt plus a pointer to an IOObj and/or an IOPar.

  Neither the IOObj nor the IOPar are managed by this object. But, when you
  use setObj or setPar, the old object pointed to will be deleted. If you don't
  want that, you'll have to just assign.
*/

mExpClass(General) CtxtIOObj : public NamedObject
{
public:
			CtxtIOObj( const IOObjContext& ct, IOObj* o=0 )
			    : NamedObject(ct), ctxt_(ct), ioobj_(o)
			      , iopar_(0)
			{ if ( o ) setName(o->name()); }
			CtxtIOObj( const CtxtIOObj& ct )
			    : NamedObject(ct), ctxt_(ct.ctxt_)
			    , ioobj_(ct.ioobj_?ct.ioobj_->clone():0)
			    , iopar_(ct.iopar_?new IOPar(*ct.iopar_):0)
			{}
    void		destroyAll();

    virtual const OD::String& name() const	{ return ctxt_.name(); }
    virtual void	setName(const char* nm)	{ ctxt_.setName(nm); }
    virtual BufferString getName() const	{ return ctxt_.getName(); }

    void		setObj(IOObj*); //!< destroys previous
    void		setObj(const DBKey&); //!< destroys previous
    void		setPar(IOPar*); //!< destroys previous
    int			fillObj(bool mktmpifnew=false,int translidxfornew=-1);
			//!< If ioobj not valid, fills using ctxt.name()
			//!< return 0=fail, 1=existing found, 2=new made
    void		fillIfOnlyOne();
				//!< replaces ioobj if there's only one
				//!< That one must match the preconditions
    void		fillDefault(bool alsoifonlyone=true);
				//!< gets Default.xx or does fillIfOnlyOne()
    void		fillDefaultWithKey(const char*,bool alsoifonlyone=true);
				//!< With alternate key

    IOObjContext		ctxt_;
    IOObj*			ioobj_;
    IOPar*			iopar_;

};
