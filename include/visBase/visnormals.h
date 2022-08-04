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
#include "viscoord.h"

namespace Threads { class Mutex; };


namespace visBase
{

/*!\brief

*/

mExpClass(visBase) Normals : public DataObject
{
public:
    static Normals*	create()
			mCreateDataObj(Normals);

    void		inverse();
			//!<Sets all normals to -normal
    int			nrNormals() const;
			//!<Envelope only, not all may be used.
    void		setNormal(int,const Coord3&);
    int			addNormal(const Coord3&);
    int			nextID(int previd) const;
    void		removeNormal( int );
    Coord3		getNormal(int) const;
    void		addNormalValue(int,const Coord3&);
    void		clear();

    void		setAll(const float* vals,int nmsz);
			//!<vals are transformed, ordered in x,y,z.

    void		setAll(const Coord3* coords,int nmsz);
			//!<coords are transformed.

    void		setAll(const Coord3& coord,int nmsz);
			//!<set all normals as coord.

    void		setDisplayTransformation(const mVisTrans*) override;
    const mVisTrans*	getDisplayTransformation() const override
			{ return transformation_; }

    osg::Array*		osgArray() { return osgnormals_; }
    const osg::Array*	osgArray() const { return osgnormals_; }

protected:
			~Normals();
			/*!< Object should be locked when calling */

    TypeSet<int>		unusednormals_;
    Threads::Mutex&		mutex_;

    const mVisTrans*		transformation_;

    osg::Array*			osgnormals_;
    friend  class		DoTransformation;

};

mExpClass(visBase) NormalListAdapter : public Coord3List
{
public:
		NormalListAdapter(Normals& n )
		    : normals_( n )
		{ normals_.ref(); }

    int		nextID(int previd) const override
		{ return normals_.nextID(previd); }

    int		add( const Coord3& n ) override
		{ return normals_.addNormal(n); }
    void	set( int idx,const Coord3& n ) override
		{ normals_.setNormal(idx,n); }
    void	remove( int idx ) override	{ normals_.removeNormal(idx); }
    Coord3	get( int idx ) const override
		{ return normals_.getNormal(idx); }
    void	addValue( int idx, const Coord3& n ) override
		{ normals_.addNormalValue(idx,n ); }
    bool	isDefined( int idx ) const override
		{ return normals_.getNormal(idx).isDefined(); }
    void	remove(const TypeSet<int>&) override;
    int		size() const override		{ return normals_.nrNormals(); }

    Normals*	getNormals() { return &normals_; }

protected:
		~NormalListAdapter()	{ normals_.unRef(); }

    Normals&	normals_;
};

};

