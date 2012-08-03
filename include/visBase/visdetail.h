#ifndef visdetail_h
#define visdetail_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Kristofer Tingdahl
 Date:		23-06-2003
 RCS:		$Id: visdetail.h,v 1.7 2012-08-03 13:01:24 cvskris Exp $
________________________________________________________________________


-*/

#include "visbasemod.h"
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

mClass(visBase) Detail
{
public:
			Detail( DetailType dt )
			    : detailtype( dt )	{}
    
    virtual DetailType	getDetailType();
    
protected:

    DetailType		detailtype;
};  
   

mClass(visBase) FaceDetail : public Detail
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

