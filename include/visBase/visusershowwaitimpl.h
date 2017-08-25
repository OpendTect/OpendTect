#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		Aug 2017
________________________________________________________________________


-*/

#include "visbasecommon.h"
class MouseCursorChanger;


namespace visBase
{

class Scene;


mExpClass(visBase) UserShowWaitPoster
{
public:

    virtual			~UserShowWaitPoster()			{}
    virtual void		post(const uiString&)			= 0;

};

mExpClass(visBase) UserShowWaitPosterFactory
{
public:

    virtual			~UserShowWaitPosterFactory()		{}
    virtual UserShowWaitPoster*  getPoster(const Scene*,int) const	= 0;

};


mExpClass(visBase) UserShowWaitImpl
{
public:

			UserShowWaitImpl(const visBase::DataObject*,
					    const uiString&,int);
			~UserShowWaitImpl()		{ readyNow(); }

    void		setMessage(const uiString&);
    void		readyNow();

    static void		setPosterFactory( UserShowWaitPosterFactory* f )
			{ factory_ = f; }

protected:

    const Scene*			scene_;
    const int				sbfld_;
    UserShowWaitPoster*			poster_;
    static UserShowWaitPosterFactory*	factory_;

    // fallback
    MouseCursorChanger*			mcc_;

};

} //namespace visBase
