#ifndef vistexturecoords_h
#define vistexturecoords_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Kristofer Tingdahl
 Date:		4-11-2002
 RCS:		$Id: vistexturecoords.h,v 1.4 2004-11-16 14:24:20 kristofer Exp $
________________________________________________________________________


-*/

#include "visdata.h"

class SoTextureCoordinate3;
class Coord3;
class Coord;
namespace Threads { class Mutex; };

namespace visBase
{

/*!\brief

*/

class TextureCoords : public DataObject
{
public:
    static TextureCoords*	create()
				mCreateDataObj(TextureCoords);

    int				size(bool includedelete=false) const;
    void			setCoord( int,  const Coord3& );
    void			setCoord( int,  const Coord& );
    int				addCoord( const Coord3& );
    int				addCoord( const Coord& );
    void			removeCoord( int );

    SoNode*			getInventorNode();

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
