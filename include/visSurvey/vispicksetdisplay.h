#ifndef vissurvpickset_h
#define vissurvpickset_h

/*+
________________________________________________________________________

 CopyRight:	(C) de Groot-Bril Earth Sciences B.V.
 Author:	Kristofer Tingdahl
 Date:		4-11-2002
 RCS:		$Id: vispicksetdisplay.h,v 1.2 2002-02-28 07:14:47 kristofer Exp $
________________________________________________________________________


-*/

class Color;

namespace visBase { class SceneObjectGroup; };
namespace Geometry { class Pos; }

namespace visSurvey
{

class Scene;

/*!\brief


*/
class PickSet
{
public:
    		PickSet( Scene &, int system=2 );
		/*!< system = 0: x, y, z
		     system = 1: x, y, t
		     system = 2: inl, crl, t
		 */
    virtual	~PickSet();

    int		nrPicks();
    
    Geometry::Pos	getPick( int ) const;
    int			addPick( const Geometry::Pos& );

    float	getInlSz() const { return inlsz; }
    float	getCrlSz() const { return inlsz; }
    float	getTSz() const { return inlsz; }

    void	setSize( float inl, float crl, float t );
    void	setColor( const Color& );
protected:
    visBase::SceneObjectGroup*	group;
    int				groupid;

    Scene&	scene;
    Color&	color;

    float	inlsz;
    float	crlsz;
    float	tsz;
};

};


#endif
