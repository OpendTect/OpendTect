#ifndef vislevelofdetail_h
#define vislevelofdetail_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Kristofer Tingdahl
 Date:		4-11-2002
 RCS:		$Id: vislevelofdetail.h,v 1.5 2009-01-08 10:15:41 cvsranojay Exp $
________________________________________________________________________


-*/

#include "visdata.h"

class SoLevelOfDetail;

namespace visBase
{

/*!\brief
Level Of details has a number of different versions of an object with different
resolution. Depending on how large parts of the screen the objects will fill,
one of the versions will be used.
*/

mClass LevelOfDetail : public DataObject
{
public:
    static LevelOfDetail*	create()
				mCreateDataObj(LevelOfDetail);

    void			addChild( DataObject*, float maxscreensize );
    				//!< maxscreensize is in pixels. The versions
    				//!< are added with the full versions first.
    SoNode*			getInventorNode();

protected:
    				~LevelOfDetail();

    SoLevelOfDetail*		lod;
    ObjectSet<DataObject>	children;
};

}; // Namespace


#endif
