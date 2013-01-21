#ifndef vistexturecoords_h
#define vistexturecoords_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Kristofer Tingdahl
 Date:		4-11-2002
 RCS:		$Id$
________________________________________________________________________


-*/

#include "visbasemod.h"
#include "visdata.h"
#include "positionlist.h"
#include "viscoord.h"

class SoTextureCoordinate2;
class SoTextureCoordinate3;
class Coord3;
class Coord;
namespace Threads { class Mutex; };

namespace visBase
{

/*!\brief

*/

mExpClass(visBase) TextureCoords : public DataObject
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

    osg::Array*			osgArray() { return osgcoords_; }
    const osg::Array*		osgArray() const { return osgcoords_; }

protected:
    				~TextureCoords();
    int				getFreeIdx();
    				/*!< Object should be locked before calling */

    SoTextureCoordinate3*	coords_;
    osg::Array*			osgcoords_;
    TypeSet<int>		unusedcoords_;
    Threads::Mutex&		mutex_;

    virtual SoNode*		gtInvntrNode();

};


mExpClass(visBase) TextureCoords2 : public DataObject
{
public:
    static TextureCoords2*	create()
				mCreateDataObj(TextureCoords2);

    void			setCoord( int,  const Coord& );

protected:
    				~TextureCoords2();
    SoTextureCoordinate2*	coords_;
    virtual SoNode*		gtInvntrNode();
    Threads::Mutex&		mutex_;
};


mExpClass(visBase) TextureCoordListAdapter : public Coord3List
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

