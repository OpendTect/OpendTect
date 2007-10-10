#ifndef visdetail_h
#define visdetail_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Kristofer Tingdahl
 Date:		23-06-2003
 RCS:		$Id: visdetail.h,v 1.4 2007-10-10 03:59:24 cvsnanne Exp $
________________________________________________________________________


-*/

#include "bufstring.h"
#include "position.h"

class SoDetail;
class SoFaceDetail;


namespace visBase
{

/*!\brief


*/

class Coordinates;

enum DetailType { Face };

class Detail
{
public:
			Detail( DetailType dt )
			    : detailtype( dt )	{}
    
    virtual DetailType	getDetailType();
    
protected:

    DetailType		detailtype;
};  
   

class FaceDetail : public Detail
{
public:
			FaceDetail( SoFaceDetail* d )
			    : Detail( Face )
			    , facedetail( d )	{}

    int			getClosestIdx(const Coordinates*,const Coord3&) const;

protected:
    SoFaceDetail*	facedetail;
};

} // namespace visBase


#endif
