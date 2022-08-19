#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
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
