#ifndef vislevelofdetail_h
#define vislevelofdetail_h

/*+
________________________________________________________________________

 CopyRight:	(C) de Groot-Bril Earth Sciences B.V.
 Author:	Kristofer Tingdahl
 Date:		4-11-2002
 RCS:		$Id: vislevelofdetail.h,v 1.2 2002-11-15 08:14:32 kristofer Exp $
________________________________________________________________________


-*/

#include "vissceneobj.h"

class SoLevelOfDetail;

namespace visBase
{

/*!\brief
Level Of details has a number of different versions of an object with different
resolution. Depending on how large parts of the screen the objects will fill,
one of the versions will be used.
*/

class LevelOfDetail : public SceneObject
{
public:
    static LevelOfDetail*	create()
				mCreateDataObj(LevelOfDetail);

    void			addChild( SceneObject*, float maxscreensize );
    				//!< maxscreensize is in pixels. The versions
    				//!< are added with the full versions first.
    SoNode*			getData();

protected:
    				~LevelOfDetail();

    SoLevelOfDetail*		lod;
    ObjectSet<SceneObject>	children;
};

}; // Namespace


#endif
