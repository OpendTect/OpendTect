#ifndef visnormals_h
#define visnormals_h

/*+
________________________________________________________________________

 CopyRight:	(C) de Groot-Bril Earth Sciences B.V.
 Author:	Kristofer Tingdahl
 Date:		4-11-2002
 RCS:		$Id: visnormals.h,v 1.2 2003-01-07 10:26:33 kristofer Exp $
________________________________________________________________________


-*/


#include "vissceneobj.h"

class CallBacker;
class SoNormal;
class Vector3;

namespace Threads { class Mutex; };

namespace visBase
{

/*!\brief

*/

class Normals : public SceneObject
{
public:
    static Normals*	create()
			mCreateDataObj(Normals);

    void		setNormal( int, const Vector3& );
    int			addNormal( const Vector3& );
    void		removeNormal( int );

    SoNode*		getData();

protected:
    			~Normals();
    int			getFreeIdx();
    			/*!< Object should be locked when calling */

    SoNormal*		normals;

    TypeSet<int>	unusednormals;
    Threads::Mutex&	mutex;
    			
};

};

#endif

