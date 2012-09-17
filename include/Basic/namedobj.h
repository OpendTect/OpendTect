#ifndef namedobj_h
#define namedobj_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	A.H.Bril
 Date:		Sep 1994, Aug 2006
 RCS:		$Id: namedobj.h,v 1.5 2011/04/22 13:28:55 cvsbert Exp $
________________________________________________________________________

-*/

#include "callback.h"
#include "bufstring.h"


/*!\brief object with a name

The NamedObject has a name and it can notify another NamedObject when it is
about to be deleted. The name may either be a string of the object itself,
or the name of another object, linked directly. This not only saves memory,
but allows for names that are fundamentally linked.

*/


mClass NamedObject : public CallBacker
{
public:
			NamedObject(const char* nm=0)
			: linkedto_(0), delnotify_(0)
			{ name_ = new BufferString(nm); }
			NamedObject( const NamedObject* l )
			: name_(0), linkedto_((NamedObject*)l), delnotify_(0)
			{}
			NamedObject( const NamedObject& o )
			: linkedto_(o.linkedto_), delnotify_(0)
			{ if ( o.name_ ) name_ = new BufferString(*o.name_); }
    virtual		~NamedObject()	;
    void		setLinked( NamedObject* l )
			{
			    if ( l )	{ delete name_; name_ = 0; }
			    else if ( !name_ ) name_ = new BufferString;
			    linkedto_ = l;
			}

    virtual const BufferString&	name() const
			{ return name_ ? *name_ : linkedto_->name(); }
    virtual void	setName(const char*);
    void		setCleanName(const char*);
    bool		operator ==( const NamedObject& no ) const
			{ return name() == no.name(); }

    void		deleteNotify(const CallBack&);

protected:

    BufferString*	name_;
    NamedObject*	linkedto_;
    CallBackSet*	delnotify_;

private:

    void		cbRem(NamedObject*);

};


#endif
