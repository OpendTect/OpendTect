#ifndef visscene_h
#define visscene_h

/*+
________________________________________________________________________

 CopyRight:	(C) de Groot-Bril Earth Sciences B.V.
 Author:	Kris Tingdahl
 Date:		Jan 2002
 RCS:		$Id: visscene.h,v 1.5 2002-03-20 08:16:44 nanne Exp $
________________________________________________________________________


-*/


#include "sets.h"
#include "vissceneobjgroup.h"

class SoEnvironment;
class SoSelection;

namespace visBase
{
/*!\brief
    Scene manages all SceneObjects and has some managing
    functions such as the selection management and variables that should
    be common for the whole scene.
*/

class Scene : public SceneObjectGroup
{
public:
    static Scene*	create()
			mCreateDataObj0arg(Scene);

    void		setAmbientLight( float );
    float		ambientLight() const;

    virtual void	deSelectAll();
    virtual void	select( int id ) {}
    virtual void	deSelect( int id ) {}

    SoNode*		getData();

protected:
			Scene();
    virtual		~Scene();

private:

    SoEnvironment*	environment;
    SoSelection*	selector;
};

};


#endif
