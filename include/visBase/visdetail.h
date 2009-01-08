#ifndef visdetail_h
#define visdetail_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Kristofer Tingdahl
 Date:		23-06-2003
 RCS:		$Id: visdetail.h,v 1.5 2009-01-08 10:15:41 cvsranojay Exp $
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

mClass Detail
{
public:
			Detail( DetailType dt )
			    : detailtype( dt )	{}
    
    virtual DetailType	getDetailType();
    
protected:

    DetailType		detailtype;
};  
   

mClass FaceDetail : public Detail
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
