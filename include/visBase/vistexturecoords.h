#ifndef vistexturecoords_h
#define vistexturecoords_h

/*+
________________________________________________________________________

 CopyRight:	(C) de Groot-Bril Earth Sciences B.V.
 Author:	Kristofer Tingdahl
 Date:		4-11-2002
 RCS:		$Id: vistexturecoords.h,v 1.1 2003-01-07 10:34:56 kristofer Exp $
________________________________________________________________________


-*/

#include "vissceneobj.h"

class SoTextureCoordinate3;
class Coord3;
class Coord;
namespace Threads { class Mutex; };

namespace visBase
{

/*!\brief

*/

class TextureCoords : public SceneObject
{
public:
    static TextureCoords*	create()
				mCreateDataObj(TextureCoords);

    void			setCoord( int,  const Coord3& );
    void			setCoord( int,  const Coord& );
    int				addCoord( const Coord3& );
    int				addCoord( const Coord& );
    void			removeCoord( int );

    SoNode*			getData();

protected:
    				~TextureCoords();
    int				getFreeIdx();
    				/*!< Object should be locked before calling */

    SoTextureCoordinate3*	coords;
    TypeSet<int>		unusedcoords;
    Threads::Mutex&		mutex;
};

};

#endif
