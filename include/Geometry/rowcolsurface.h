#pragma once
                                                                                
/*+
________________________________________________________________________
(C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
Author:        K. Tingdahl
Date:          April 2006
Contents:      Ranges
________________________________________________________________________

-*/

#include "geometrymod.h"
#include "rowcol.h"
#include "geomelement.h"


namespace Geometry
{

/*!Surface which positions are orgainzied in rows/cols. The number of
   columns in each row may vary. */

mExpClass(Geometry) RowColSurface : public Element
{
public:
    virtual void		getPosIDs(TypeSet<GeomPosID>&,bool=true) const;

    virtual bool		isEmpty() const				= 0;


    Iterator*			createIterator() const;

    virtual StepInterval<int>	colRange() const;
    virtual StepInterval<int>	colRange(int row) const			= 0;
    virtual StepInterval<int>	rowRange() const			= 0;

    virtual bool		setKnot(const RowCol&,const Coord3&)	= 0;
    virtual Coord3		getKnot(const RowCol&) const		= 0;
    virtual bool		isKnotDefined(const RowCol&) const	= 0;

    virtual Coord3		getPosition(GeomPosID pid) const;
    virtual bool		setPosition(GeomPosID pid,const Coord3&);
    virtual bool		isDefined(GeomPosID pid) const;
};

};

