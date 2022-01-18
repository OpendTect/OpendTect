#pragma once
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Kristofer Tingdahl
 Date:		March 2009
________________________________________________________________________

-*/

// this header file only be used in the classes related to Horzonsection .
// don't include it in somewhere else !!!

#include "visbasemod.h"
#include "refcount.h"

class DataPointSet;
class ZAxisTransform;

namespace visBase
{
class Coordinates;
class HorizonSection;
class HorizonSectionTile;

class HorizonSectionDataHandler : public ReferencedObject
{
public:
			HorizonSectionDataHandler(const HorizonSection*);

    void		updateZAxisVOI();
    void		setZAxisTransform(ZAxisTransform*);
    ZAxisTransform*	getZAxistransform()	{ return zaxistransform_; }
    void		generatePositionData(DataPointSet&,double zshift,
					     int sectionid ) const;

protected:
    virtual		~HorizonSectionDataHandler();

private:
    void		removeZTransform();

    ZAxisTransform*	zaxistransform_;
    int			zaxistransformvoi_;
    //-1 not needed by zaxistransform, -2 not set
    const HorizonSection*	horsection_;

    friend class HorizonSection;
};

} // namespace visBase
