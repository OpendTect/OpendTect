#ifndef visdetail_h
#define visdetail_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Kristofer Tingdahl
 Date:		23-06-2003
 RCS:		$Id: visdetail.h,v 1.3 2004-08-05 08:52:08 kristofer Exp $
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
    
    virtual DetailType	getDetailType();
    
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

    int			getClosestIdx( const Coordinates*, const Coord3& );

protected:
    SoFaceDetail*	facedetail;
};

}; // Namespace


#endif
