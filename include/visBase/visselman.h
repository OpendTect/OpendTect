#ifndef visselman_h
#define visselman_h

/*+
________________________________________________________________________

 CopyRight:	(C) de Groot-Bril Earth Sciences B.V.
 Author:	Kris Tingdahl
 Date:		Jan 2002
 RCS:		$Id: visselman.h,v 1.5 2002-03-11 10:46:12 kristofer Exp $
________________________________________________________________________


-*/


#include "sets.h"
#include "vissceneobj.h"

class SoSelection;
class SoNode;
class SoPath;

namespace visBase
{
class Scene;
class SceneObject;
class DataObject;
class DataManager;
class Selectable;

/*!\brief
SelectionManager handles DataObject that can be selected. If an object can be
selected, it has to register himself with regSelObject. At registration it has
to give two objects, first of all, he gives the object that outside users
will associate him with when they want to add their CB to detect his selection.

Secondly it has to give the SceneObject that actually is selected.
*/

class SelectionManager : public CallBackClass
{
public:
				SelectionManager();
    virtual			~SelectionManager();

    void			deSelectAll();

    bool			isSelectable(  const DataObject& assoc );
    				/*!< Check if the object is selectable */
   
    Notifier<SelectionManager>	selnotifer;
    Notifier<SelectionManager>	deselnotifer;

    int				nrSelected() const;
    const DataObject*		getSelDataObject( int ) const;
    const Scene*		getSelScene( int ) const;
    int				getSelNr( const DataObject*,
	    				  const Scene* =0) const ;

protected:
    void	select( Scene*, SoPath* );
    void	deSelect( Scene*, SoPath* );

    friend	visBase::DataObject;
    void	regSelObject( DataObject& );
    void	unRegSelObject( DataObject& );

    ObjectSet<const DataObject>		selected;
    ObjectSet<Scene>			selscenes;

    ObjectSet<DataObject>		selobjs;

    friend		visBase::Scene;
    static void		selectCB(void*, SoPath* );
    static void		deSelectCB(void*, SoPath* );
};

};

#endif
