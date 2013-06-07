#ifndef visnormals_h
#define visnormals_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Kristofer Tingdahl
 Date:		4-11-2002
 RCS:		$Id$
________________________________________________________________________


-*/


#include "visdata.h"
#include "positionlist.h"

class CallBacker;
class SoNormal;
class Coord3;

namespace Threads { class Mutex; };

namespace visBase
{

/*!\brief

*/

mClass Normals : public DataObject
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

    void		setAll(const float* vals,int nmsz);
    			//!<vals are transformed, ordered in x,y,z.

    void		setDisplayTransformation( const mVisTrans* nt );
    const mVisTrans*	getDisplayTransformation() const { return transformation_; }

protected:
    void		transformNormal(const Transformation*,Coord3&,
	    				bool todisplay) const;

    			~Normals();
    int			getFreeIdx();
    			/*!< Object should be locked when calling */

    SoNormal*			normals_;

    TypeSet<int>		unusednormals_;
    Threads::Mutex&		mutex_;

    const mVisTrans*		transformation_;

    virtual SoNode*		gtInvntrNode();
    			
};

mClass NormalListAdapter : public Coord3List
{
public:
    		NormalListAdapter(Normals& n )
		    : normals_( n )
		{ normals_.ref(); }

    int		nextID(int previd) const{ return normals_.nextID(previd); }

    int		add(const Coord3& n )	{ return normals_.addNormal(n); }
    void	set(int idx,const Coord3& n)	{ normals_.setNormal(idx,n); }
    void	remove(int idx)		{ normals_.removeNormal(idx); }
    Coord3	get(int idx) const	{ return normals_.getNormal(idx); }
    void	addValue(int idx, const Coord3& n) 
    		{ normals_.addNormalValue(idx,n ); }
    bool	isDefined(int idx) const
    		{ return normals_.getNormal(idx).isDefined(); }

protected:
		~NormalListAdapter()	{ normals_.unRef(); }

    Normals&	normals_;
};

};

#endif

