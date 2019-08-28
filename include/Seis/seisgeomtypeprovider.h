#pragma once

/*
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		Mar 2019
________________________________________________________________________

*/

#include "seistype.h"
#include "notify.h"


namespace Seis
{

mClass(Seis) GeomTypeProvider : public CallBacker
{
public:

				GeomTypeProvider()
				    : geomTypeChanged(this)	{}
    virtual			~GeomTypeProvider()
				{ detachAllNotifiers(); }

    virtual bool		hasFixedGeomType() const	{ return false;}
    virtual GeomType		geomType() const		= 0;
    virtual TypeSet<GeomType>	availableTypes() const		= 0;

    Notifier<GeomTypeProvider>	geomTypeChanged;

};


mClass(Seis) SingleGeomTypeProvider : public GeomTypeProvider
{
public:

			SingleGeomTypeProvider( GeomType gt )
			    : gt_(gt)				{}

    bool		hasFixedGeomType() const override	{ return true; }
    GeomType		geomType() const override
			{ return gt_; }
    TypeSet<GeomType>	availableTypes() const override
			{ return TypeSet<GeomType>(gt_); }

    GeomType	gt_;

};


} // namespace Seis
