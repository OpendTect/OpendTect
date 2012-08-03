#ifndef visanchor_h
#define visanchor_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Kris Tingdahl
 Date:		Feb 2007
 RCS:		$Id: visanchor.h,v 1.4 2012-08-03 13:01:23 cvskris Exp $
________________________________________________________________________

-*/

#include "visbasemod.h"
#include "visdatagroup.h"

class SbString;
class SoWWWAnchor;

namespace visBase
{


mClass(visBase) Anchor : public DataObjectGroup
{
public:
    static Anchor*	create()
			mCreateDataObj(Anchor);
    
    void		enable( bool yn );

    Notifier<Anchor>	click;

protected:
    SoGroup*		createGroup();
    SoWWWAnchor*	getAnchor();

    static void		clickCB(const SbString&,void*,SoWWWAnchor*);
    static void		highlightCB(const SbString&,void*,SoWWWAnchor*);
    bool		ishighlighted_;
};
}; //namespace

#endif

