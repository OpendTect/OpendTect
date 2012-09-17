#ifndef visdetail_h
#define visdetail_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Kristofer Tingdahl
 Date:		23-06-2003
 RCS:		$Id: visdetail.h,v 1.6 2009/07/22 16:01:24 cvsbert Exp $
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
