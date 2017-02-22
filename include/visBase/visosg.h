#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Kristofer Tingdahl
 Date:		Sep 2012
________________________________________________________________________


-*/

/*! Definition of macros used to make osg-life easier */

#include "visbasecommon.h"
#include "refcount.h"


namespace osg { class Vec3f; class Array; class Referenced; }


#define mGetOsgArrPtr(tp,ptr) ((tp) ptr->getDataPointer() )
#define mGetOsgVec2Arr(ptr) ((osg::Vec2Array*) ptr )
#define mGetOsgVec3Arr(ptr) ((osg::Vec3Array*) ptr )
#define mGetOsgVec4Arr(ptr) ((osg::Vec4Array*) ptr )

#if defined(visBase_EXPORTS) || defined(VISBASE_EXPORTS) || \
defined(uiOSG_EXPORTS) || defined(UIOSG_EXPORTS)
//Only available in visBase
#include <osg/Vec3>
#include <osg/Vec3d>
#include <position.h>
#include <osg/Vec4>
#include <osg/Vec4d>
#include <osg/Node>
#include <color.h>
#include <convert.h>

namespace visBase
{
   void unRefOsgPtr(osg::Referenced*);
   void refOsgPtr(const osg::Referenced*);
   void unRefAndZeroOsgPtr(osg::Referenced*);
}



namespace Conv
{
    template <>
    inline void set( Coord3d& _to, const osg::Vec3f& v )
    { _to.x_ = v[0]; _to.y_=v[1]; _to.z_=v[2]; }

    template <>
    inline void set( osg::Vec3f& _to, const Coord3d& v )
    { _to.set( (float) v.x_, (float) v.y_, (float) v.z_ ); }

    template <>
    inline void set( Coord3f& _to, const osg::Vec3f& v )
    { _to.x_ = v[0]; _to.y_=v[1]; _to.z_=v[2]; }

    template <>
    inline void set( osg::Vec3f& _to, const Coord3f& v )
    { _to.set( v.x_, v.y_, v.z_ ); }

    template <>
    inline void set( Coord2d& _to, const osg::Vec2f& v )
    { _to.x_ = v[0]; _to.y_=v[1]; }

    template <>
    inline void set( osg::Vec2f& _to, const Coord2d& v )
    { _to.set( (float) v.x_, (float) v.y_ ); }

    template <>
    inline void set( Coord2f& _to, const osg::Vec2f& v )
    { _to.x_ = v[0]; _to.y_=v[1]; }

    template <>
    inline void set( osg::Vec2f& _to, const Coord2f& v )
    { _to.set( v.x_, v.y_ ); }

    template <>
    inline void set( Coord3& _to, const osg::Vec3d& v )
    { _to.x_ = v[0]; _to.y_=v[1]; _to.z_=v[2]; }

    template <>
    inline void set( osg::Vec3d& _to, const Coord3& v )
    { _to.set(	v.x_, v.y_, v.z_ ); }

#define mIsOsgVec3Def( pos ) \
( pos[0]<mUdf(float) && pos[1]<mUdf(float) && pos[2]<mUdf(float) )

#define mODColVal(val)   ( val<=0.0 ? 0  : val>=1.0 ? 255  : mNINT32(255*val) )
#define mOsgColValF(val) ( val<=0 ? 0.0f : val>=255 ? 1.0f : float(val)/255   )
#define mOsgColValD(val) ( val<=0 ? 0.0  : val>=255 ? 1.0  : double(val)/255  )

    template <>
    inline void set( Color& _to, const osg::Vec4f& col )
    { _to.set( mODColVal(col[0]), mODColVal(col[1]),
	       mODColVal(col[2]), 255-mODColVal(col[3]) ); }

    template <>
    inline void set( osg::Vec4f& _to, const Color& col )
    { _to.set( mOsgColValF(col.r()), mOsgColValF(col.g()),
	       mOsgColValF(col.b()), 1.0f-mOsgColValF(col.t()) ); }

    template <>
    inline void set( Color& _to, const osg::Vec4d& col )
    { _to.set( mODColVal(col[0]), mODColVal(col[1]),
	       mODColVal(col[2]), 255-mODColVal(col[3]) ); }

    template <>
    inline void set( osg::Vec4d& _to, const Color& col )
    { _to.set( mOsgColValD(col.r()), mOsgColValD(col.g()),
	       mOsgColValD(col.b()), 1.0-mOsgColValD(col.t()) ); }

} //Namespace conv

namespace Values
{
    template<>
    class Undef<osg::Vec3f>
    {
    public:
	static void		setUdf( osg::Vec3f& i )	{}
    };

    template<>
    class Undef<osg::Vec3d>
    {
    public:
	static void		setUdf( osg::Vec3d& i )	{}
    };

    template<>
    class Undef<osg::Vec2f>
    {
    public:
	static void		setUdf( osg::Vec2f& i )	{}
    };

    template<>
    class Undef<osg::Vec2d>
    {
    public:
	static void		setUdf( osg::Vec2d& i )	{}
    };


    template<>
    class Undef<osg::Vec4f>
    {
    public:
	static void		setUdf( osg::Vec4f& i )	{}
    };

    template<>
    class Undef<osg::Vec4d>
    {
    public:
	static void		setUdf( osg::Vec4d& i )	{}
    };

} //Namespace Values


/*! The OneFrameCullDisabler may be used to break an OpenSceneGraph
    chicken-and-egg problem that occurs when computing the initial
    bounding box of a screen-sized object (see e.g. code comments at
    osgText::TextBase::computeBound()). The initial underestimation
    of its scale can make it become a victim of small feature culling
    for as long as the user does not extremely zoom in on it. */

mExpClass(visBase) OneFrameCullDisabler : public osg::NodeCallback
{
public:
				OneFrameCullDisabler(osg::Node*);
    virtual void		operator()(osg::Node*,osg::NodeVisitor*);
};

#define mAttachOneFrameCullDisabler( osgnode ) \
    osg::ref_ptr<OneFrameCullDisabler> oneframeculldisablerof##osgnode = \
				       new OneFrameCullDisabler( osgnode );


#endif
