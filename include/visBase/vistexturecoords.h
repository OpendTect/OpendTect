#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Kristofer Tingdahl
 Date:		4-11-2002
________________________________________________________________________


-*/

#include "visbasemod.h"
#include "visdata.h"
#include "positionlist.h"
#include "threadlock.h"
#include "viscoord.h"



namespace visBase
{


mExpClass(visBase) TextureCoords : public DataObject
{
public:
    static TextureCoords*	create()
				mCreateDataObj(TextureCoords);
    void			copyFrom(const TextureCoords&);
    int				size(bool includedeleted=false) const;
    void			setCoord(int idx,const Coord3&);
    void			setCoord(int idx,const Coord&);
    int				addCoord(const Coord3&);
    int				addCoord(const Coord&);
    Coord3			getCoord(int) const;
    void			setPositions(const Coord*,int sz,int start);
    void			clear();
    int				nextID(int previd) const;
    void			removeCoord(int);
    bool			isEmpty() const { return size()==0; }

    osg::Array*			osgArray()		{ return osgcoords_; }
    const osg::Array*		osgArray() const	{ return osgcoords_; }

protected:
				~TextureCoords();

    int				searchFreeIdx();
    void			setPosWithoutLock(int,const Coord&);
				/*!< Object should be locked when calling */

    int				lastsearchedidx_;
    int				nrfreecoords_;

    osg::Array*			osgcoords_;
    mutable Threads::Lock	lock_;
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
    int			size() const	{ return texturecoords_.size(); }
    void		addValue(int,const Coord3&);
    void		remove(const TypeSet<int>&);
    TextureCoords*	getTextureCoords() { return &texturecoords_; }

protected:
			~TextureCoordListAdapter();

    TextureCoords&	texturecoords_;
};

}; //namespace

