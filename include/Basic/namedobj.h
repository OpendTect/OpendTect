#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "basicmod.h"
#include "notify.h"
#include "bufstring.h"


/*!\brief object with a name. */

mExpClass(Basic) ObjectWithName
{
public:

    typedef OD::String		name_type;

    virtual			~ObjectWithName()			{}

    virtual const name_type&	name() const		= 0;

    mDeprecatedObs inline bool	hasName( const char* nm ) const
				{ return name() == nm; }
    mDeprecatedObs inline bool	hasName( const name_type& nm ) const
				{ return name() == nm; }

    void			putNameInPar(IOPar&) const;
};

/*!\brief object with a name. */

mExpClass(Basic) NamedObject : public ObjectWithName
{
public:

			NamedObject( const char* nm=nullptr )
			    : name_(nm)			{}
			NamedObject( const NamedObject& oth )
			    : name_(oth.getName())	{}
    virtual		~NamedObject()			{}
    NamedObject&	operator =(const NamedObject&);
    mDeprecatedObs bool	operator ==( const NamedObject& oth ) const
			{ return name_ == oth.getName(); }

    const name_type&	name() const override		{ return name_; }
    virtual BufferString getName() const		{ return name_; }
    virtual void	setName( const char* nm )	{ name_ = nm; }

    bool		getNameFromPar(const IOPar&);

protected:

    BufferString	name_;

};


/*!\brief CallBacker object with a name. Use if you want your object to be
  able to send and receive CallBack's, but Monitorable is not an option.*/

mExpClass(Basic) NamedCallBacker : public CallBacker
				 , public NamedObject
{
public:
			NamedCallBacker(const char* nm=0);
			mOD_DisableCopy(NamedCallBacker)

    inline bool		operator ==( const NamedCallBacker& oth ) const
			{ return name_ == oth.getName(); }
    inline bool		operator ==( const NamedObject& oth ) const
			{ return name_ == oth.getName(); }

    virtual Notifier<NamedCallBacker>&	objectToBeDeleted() const
			{ return mSelf().delnotif_; }

protected:

    Notifier<NamedCallBacker>	delnotif_;
    mutable Threads::Atomic<bool> delalreadytriggered_;
    void			sendDelNotif() const;

};


mGlobal(Basic) inline bool operator >( const ObjectWithName& obj1,
				       const ObjectWithName& obj2 )
{
    return obj1.name() > obj2.name();
}
