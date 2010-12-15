#ifndef visscene_h
#define visscene_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Kris Tingdahl
 Date:		Jan 2002
 RCS:		$Id: visscene.h,v 1.22 2010-12-15 22:54:14 cvskris Exp $
________________________________________________________________________


-*/


#include "sets.h"
#include "visdatagroup.h"

class SoEnvironment;

namespace visBase
{
    class SelectionManager;
    class EventCatcher;
    class PolygonOffset;
    class DirectionalLight;

/*!\brief
    Scene manages all DataObjects and has some managing
    functions such as the selection management and variables that should
    be common for the whole scene.
*/

mClass Scene : public DataObjectGroup
{
public:
    static Scene*	create()
			mCreateDataObj(Scene);

    void		addObject(DataObject*);
    void		insertObject(int idx,DataObject*);

    void		setAmbientLight(float);
    float		ambientLight() const;

    void		setDirectionalLight(const DirectionalLight&);
    DirectionalLight*	getDirectionalLight() const;

    PolygonOffset*	getPolygonOffset()	{ return polygonoffset_; }
    bool		saveCurrentOffsetAsDefault() const;

    bool		blockMouseSelection(bool yn);
    			/*!<\returns previous status. */

    SoNode*		getInventorNode();
    EventCatcher&	eventCatcher();

    void		setName(const char*);

    Notifier<Scene>	nameChanged;

    static const char*	sKeyOffset()	{ return "Polygon offset"; }
    static const char*	sKeyFactor()	{ return "Factor"; }
    static const char*	sKeyUnits()	{ return "Units"; }


protected:
    virtual		~Scene();
    EventCatcher&	events_;

    void		fillPar(IOPar&,TypeSet<int>&) const;
    int			usePar(const IOPar&);

    void		fillOffsetPar( IOPar& ) const;

private:
    int			mousedownid_;

    void		mousePickCB(CallBacker*);

    SoEnvironment*	environment_;
    PolygonOffset*	polygonoffset_;
    DirectionalLight*	directionallight_;
    SoGroup*		selroot_;
    bool		blockmousesel_;
};

};


#endif
