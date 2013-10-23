#ifndef vishordatahandler_h
#define vishordatahandler_h
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Kristofer Tingdahl
 Date:		March 2009
 RCS:		$Id$
________________________________________________________________________

-*/

// this header file only be used in the classes related to Horzonsection . 
// don't include it in somewhere else !!!

#include "refcount.h"

class ZAxisTransform;
class DataPointSet;

namespace visBase
{
    class HorizonSectionTile;
    class Coordinates;
    class HorizonSection;

class HorizonSectionDataHandler
{ mRefCountImpl(HorizonSectionDataHandler)
public:

    HorizonSectionDataHandler( const HorizonSection* );

    void updateZAxisVOI();
    void setZAxisTransform( ZAxisTransform* );
    ZAxisTransform* getZAxistransform() { return zaxistransform_; }
    void generatePositionData( DataPointSet& dtpntset, double zshift,
			       int sectionid ) const;


private:
      void removeZTransform();

      ZAxisTransform*   zaxistransform_;
      int		zaxistransformvoi_; 
      //-1 not needed by zaxistransform, -2 not set
      const HorizonSection*	horsection_;

      friend class HorizonSection;

};
}
#endif

