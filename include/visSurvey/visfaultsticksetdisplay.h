#ifndef visfaultsticksetdisplay_h
#define visfaultsticksetdisplay_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	J.C. Glas
 Date:		November 2008
 RCS:		$Id: visfaultsticksetdisplay.h,v 1.5 2009-08-06 02:04:14 cvskris Exp $
________________________________________________________________________


-*/

#include "vissurvobj.h"
#include "visobject.h"

#include "emposid.h"


namespace visBase 
{ 
    class Transformation;
    class IndexedPolyLine3D;
    class PickStyle;
}

namespace EM { class FaultStickSet; }
namespace MPE { class FaultStickSetEditor; }

namespace visSurvey
{
class MPEEditor;
class Seis2DDisplay;

/*!\brief Display class for FaultStickSets
*/

mClass FaultStickSetDisplay : public visBase::VisualObjectImpl,
			     public SurveyObject
{
public:
    static FaultStickSetDisplay* create()
				mCreateDataObj(FaultStickSetDisplay);

    MultiID			getMultiID() const;
    bool			isInlCrl() const	{ return false; }

    bool			hasColor() const		{ return true; }
    Color			getColor() const;
    void			setColor(Color);
    bool			allowMaterialEdit() const	{ return true; }
    NotifierAccess*		materialChange();

    void			showManipulator(bool);
    bool			isManipulatorShown() const;

    void			setDisplayTransformation(mVisTrans*);
    mVisTrans*			getDisplayTransformation();

    void			setSceneEventCatcher(visBase::EventCatcher*);

    bool			setEMID(const EM::ObjectID&);
    EM::ObjectID		getEMID() const;

    void			updateSticks(bool nearest=false);
    void			updateEditPids();

    Notifier<FaultStickSetDisplay> colorchange;

    void			removeSelection(const Selector<Coord3>&,
	    					TaskRunner*);

    void			setDisplayOnlyAtSections(bool yn);
    bool			displayedOnlyAtSections() const;

    virtual void                fillPar(IOPar&,TypeSet<int>&) const;
    virtual int                 usePar(const IOPar&);

protected:

    virtual			~FaultStickSetDisplay();

    void   			otherObjectsMoved(
	    				const ObjectSet<const SurveyObject>&,
					int whichobj);

    static const char*		sKeyEarthModelID()	{ return "EM ID"; }

    void			mouseCB(CallBacker*);
    void			emChangeCB(CallBacker*);

    visBase::EventCatcher*	eventcatcher_;
    visBase::Transformation*	displaytransform_;

    EM::FaultStickSet*		emfss_;
    MPE::FaultStickSetEditor*	fsseditor_;
    visSurvey::MPEEditor*	viseditor_;

    Coord3			mousepos_;
    bool			showmanipulator_;

    int				neareststicknr_;

    TypeSet<EM::PosID>		editpids_;

    visBase::IndexedPolyLine3D* sticks_;
    visBase::IndexedPolyLine3D* neareststick_;

    visBase::PickStyle*		stickspickstyle_;
    visBase::PickStyle*		neareststickpickstyle_;

    bool			displayonlyatsections_;
};

} // namespace VisSurvey


#endif
