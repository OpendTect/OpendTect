#ifndef visdetail_h
#define visdetail_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Kristofer Tingdahl
 Date:		23-06-2003
 RCS:		$Id: visdetail.h,v 1.2 2003-11-07 12:21:54 bert Exp $
________________________________________________________________________


-*/

#include "position.h"
#include <bufstring.h>

class SoDetail;
class SoFaceDetail;
class SoPointDetail;


namespace visBase
{

/*!\brief


*/

class Coordinates;

enum DetailType { Face };

class Detail
{
public:
    Detail( DetailType dt)
    	: detailtype( dt )
    {};
    
    DetailType		getDetailType();
    
protected:

    DetailType		detailtype;
};  
   

class FaceDetail : public Detail
{
public:
    FaceDetail( SoFaceDetail* d )
	: Detail( Face )
	, facedetail( d )
    {};

    int			getClosestIdx( Coordinates*, Coord3 );

protected:
    SoFaceDetail*	facedetail;
};

}; // Namespace


#endif
