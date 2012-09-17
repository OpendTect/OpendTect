#ifndef vistexturecoords_h
#define vistexturecoords_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Kristofer Tingdahl
 Date:		4-11-2002
 RCS:		$Id: vistexturecoords.h,v 1.11 2011/04/28 07:00:12 cvsbert Exp $
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

protected:
    				~TextureCoords();
    int				getFreeIdx();
    				/*!< Object should be locked before calling */

    SoTextureCoordinate3*	coords_;
    TypeSet<int>		unusedcoords_;
    Threads::Mutex&		mutex_;

    virtual SoNode*		gtInvntrNode();

};


mClass TextureCoordListAdapter : public Coord3List
{
public:
    			TextureCoordListAdapter(TextureCoords&);

    int			nextID(int previd) const;
    Coord3		get(int id) const;
    bool		isDefined(int id) const;
    void		set(int id,const Coord3&);
    int			add(const Coord3&);
    void		remove(int id);
    int			getSize() const	{ return texturecoords_.size(); }
    void		addValue(int,const Coord3&);

protected:
    			~TextureCoordListAdapter();

    TextureCoords&	texturecoords_;
};

}; //namespace

#endif
