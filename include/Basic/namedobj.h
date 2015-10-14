#ifndef namedobj_h
#define namedobj_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		Sep 1994, Aug 2006, Mar 2015
 RCS:		$Id$
________________________________________________________________________

-*/

#include "basicmod.h"
#include "callback.h"
#include "bufstring.h"


/*!
\brief Object with a name.

  The NamedObject has a name and it can notify another NamedObject when it is
  about to be deleted. The name may either be a string of the object itself,
  or the name of another object, linked directly. This not only saves memory,
  but allows for names that are fundamentally linked.

  The name() function delivers a string that can be displayed to users in lists,
  trees, etc, and it is usable as a key. For displaying as annotation, use
  annotName(). This string is clean from keying details, just as users would
  expect it on maps etc.

*/

mExpClass(Basic) NamedObject : public CallBacker
{
public:

			NamedObject(const char* nm=0);
			NamedObject(const NamedObject* linkedto);
			NamedObject(const NamedObject&);
    virtual		~NamedObject();
    void		setLinkedTo(NamedObject*);
    bool		operator ==( const NamedObject& oth ) const
			{ return name() == oth.name(); }

    virtual const OD::String& name() const
			{ return name_ ? *name_ : linkedto_->name(); }
    virtual const OD::String& annotName() const
			{ return linkedto_ ? linkedto_->annotName() : name();}

    virtual void	setName(const char*);
    void		setCleanName(const char*); //!< cleans string first

    void		deleteNotify(const CallBack&);

protected:

    BufferString*	name_;
    NamedObject*	linkedto_;
    CallBackSet*	delnotify_;

private:

    void		cbRem(NamedObject*);

};


#endif

