#ifndef visnormals_h
#define visnormals_h

/*+
________________________________________________________________________

 CopyRight:	(C) de Groot-Bril Earth Sciences B.V.
 Author:	Kristofer Tingdahl
 Date:		4-11-2002
 RCS:		$Id: visnormals.h,v 1.1 2002-12-20 16:30:21 kristofer Exp $
________________________________________________________________________


-*/


#include "vissceneobj.h"

class CallBacker;
class SoNormal;

namespace visBase
{

class Coordinates;


/*!\brief

*/

class Normals : public SceneObject
{
public:
    static Normals*	create()
			mCreateDataObj(Normals);

    void		setCoords( Coordinates* );
    int			addNormal( int, int, int );
    void		removeNormal( int );

    SoNode*		getData();

protected:
    void		calcNormal( int nr, int, int, int );
    			~Normals();
    int			getFreeIdx();
    void		handleCoordChange( CallBacker* );

    Coordinates*		coords;
    SoNormal*			normals;
    TypeSet<int>		p0s;
    TypeSet<int>		p1s;
    TypeSet<int>		p2s;

    ObjectSet<TypeSet<int> >	normaldeps;

    TypeSet<int>		unusednormals;
};
    

};

#endif

