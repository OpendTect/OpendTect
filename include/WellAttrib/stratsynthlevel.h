#ifndef stratsynthlevel_h
#define stratsynthlevel_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Satyaki/Bruno
 Date:		July 2013
 RCS:		$Id$
________________________________________________________________________

-*/

#include "wellattribmod.h"
#include "namedobj.h"
#include "color.h"
#include "typeset.h"
#include "valseriesevent.h"


mClass(WellAttrib) StratSynthLevel : public NamedObject
{
public:

			StratSynthLevel( const char* nm, Color c,
					 const TypeSet<float>* dpts=0 )
			    : NamedObject(nm), col_(c), snapev_(VSEvent::None)
			{ if ( dpts ) zvals_ = *dpts; }

    TypeSet<float> 	zvals_;
    Color          	col_;
    VSEvent::Type  	snapev_;

};


#endif
