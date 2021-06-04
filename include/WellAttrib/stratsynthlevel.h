#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Satyaki/Bruno
 Date:		July 2013
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

			StratSynthLevel( const char* nm, OD::Color c,
					 const TypeSet<float>* dpts=nullptr )
			    : NamedObject(nm), col_(c), snapev_(VSEvent::None)
			{ if ( dpts ) zvals_ = *dpts; }

    TypeSet<float> 	zvals_;
    OD::Color		col_;
    VSEvent::Type  	snapev_;

};


