#ifndef vistexturecoords_h
#define vistexturecoords_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Kristofer Tingdahl
 Date:		4-11-2002
 RCS:		$Id: vistexturecoords.h,v 1.7 2009-07-22 16:01:25 cvsbert Exp $
________________________________________________________________________


-*/

#include "visdata.h"
#include "positionlist.h"

class SoTextureCoordinate3;
class Coord3;
class Coord;
namespace Threads { class Mutex; };

namespace visBase
{

/*!\brief

*/

mClass TextureCoords : public DataObject
{
public:
    static TextureCoords*	create()
				mCreateDataObj(TextureCoords);

    int				size(bool includedelete=false) const;
    void			setCoord( int,  const Coord3& );
    void			setCoord( int,  const Coord& );
    int				addCoord( const Coord3& );
    int				addCoord( const Coord& );
    Coord3			getCoord( int ) const;
    void			removeCoord( int );

    int				nextID(int previd) const;

    SoNode*			getInventorNode();

protected:
    				~TextureCoords();
    int				getFreeIdx();
    				/*!< Object should be locked before calling */

    SoTextureCoordinate3*	coords_;
    TypeSet<int>		unusedcoords_;
    Threads::Mutex&		mutex_;
};


mClass TextureCoordListAdapter : public Coord3List
{
public:
    			TextureCoordListAdapter(TextureCoords&);

    int			nextID(int previd) const;
    Coord3		get(int id) const;
    void		set(int id,const Coord3&);
    int			add(const Coord3&);
    void		remove(int id);

protected:
    			~TextureCoordListAdapter();

    TextureCoords&	texturecoords_;
};

}; //namespace

#endif
