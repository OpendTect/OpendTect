#ifndef visscene_h
#define visscene_h

/*+
________________________________________________________________________

 CopyRight:	(C) de Groot-Bril Earth Sciences B.V.
 Author:	Kris Tingdahl
 Date:		Jan 2002
 RCS:		$Id: visscene.h,v 1.2 2002-02-27 13:04:36 kristofer Exp $
________________________________________________________________________


-*/


#include "sets.h"
#include "vissceneobjgroup.h"

class SoEnvironment;

namespace visBase
{

class SelectionManager;

/*!\brief
    Scene manages all SceneObjects and has some managing
    functions such as the selection management and variables that should
    be common for the whole scene.
*/

class Scene : public SceneObjectGroup
{
public:
			Scene();
    virtual		~Scene();

    int 		id() const { return id_; }

    void		setAmbientLight( float );
    float		ambientLight() const;

    SelectionManager&	selMan() { return *selman; }

    SoNode*		getData();

private:
    SelectionManager*	selman;
    SoEnvironment*	environment;
    int			id_;
};

};


#endif
