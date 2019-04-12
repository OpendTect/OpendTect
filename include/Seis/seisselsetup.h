#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		Nov 2007 / Feb 2019
________________________________________________________________________

-*/

#include "seiscommon.h"
#include "bufstring.h"


namespace Seis
{

/*!\brief setup for subselection of seismic data */

mClass(Seis) SelSetup
{
public:

		SelSetup( Seis::GeomType gt )
		    : is2d_(Seis::is2D(gt))
		    , isps_(Seis::isPS(gt))
		    , onlyrange_(true)
		    , fornewentry_(false)
		    , multiline_(false)			//!< 2D only
		    , withoutz_(false)
		    , withstep_(true)			{}
		SelSetup( bool is_2d, bool is_ps=false )
		    : is2d_(is_2d)
		    , isps_(is_ps)
		    , onlyrange_(true)
		    , fornewentry_(false)
		    , multiline_(false)			//!< 2D only
		    , withoutz_(false)
		    , withstep_(true)			{}

    mDefSetupClssMemb(SelSetup,bool,is2d)
    mDefSetupClssMemb(SelSetup,bool,isps)
    mDefSetupClssMemb(SelSetup,bool,onlyrange)
    mDefSetupClssMemb(SelSetup,bool,fornewentry)
    mDefSetupClssMemb(SelSetup,bool,multiline)
    mDefSetupClssMemb(SelSetup,bool,withoutz)
    mDefSetupClssMemb(SelSetup,bool,withstep)
    mDefSetupClssMemb(SelSetup,BufferString,zdomkey)

    Seis::GeomType geomType() const	{ return geomTypeOf(is2d_,isps_); }

};


} // namespace
