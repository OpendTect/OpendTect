#ifndef vispseventdisplay_h
#define vispseventdisplay_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	K. Tingdahl
 Date:		April 2005
 RCS:		$Id$
________________________________________________________________________

-*/

#include "uiprestackviewermod.h"
#include "vissurvobj.h"

#include "draw.h"
#include "visobject.h"
#include "coltabmapper.h"
#include "coltabsequence.h"

namespace visBase 
{ 
    class IndexedPolyLine; 
    class DrawStyle;
    class DataObjectGroup;
};

namespace PreStack { class EventManager; class EventSet; }
namespace visSurvey
{

    
mClass(uiPreStackViewer) PSEventDisplay : public visBase::VisualObjectImpl,
		       public SurveyObject
{
public:
    static PSEventDisplay*	create()
				mCreateDataObj( PSEventDisplay );
    bool			isInlCrl() const { return true; }

    void			setEventManager(PreStack::EventManager*);
    void			setHorizonID(int);

    enum MarkerColor 		{ Single, Quality, Velocity, VelocityFit };
    				DeclareEnumUtils(MarkerColor);

    void			setMarkerColor(MarkerColor,bool update=true);
    MarkerColor			getMarkerColor() const;
    void			setColTabMapper(const ColTab::MapperSetup&,
	    					bool update=true);
    const ColTab::MapperSetup&	getColTabMapper() const;
    void			setColTabSequence(const ColTab::Sequence&,
	    					  bool update=true);
    virtual const ColTab::Sequence* getColTabSequence(int ch=0) const;
    virtual void		setColTabSequence(int,const ColTab::Sequence&,
	    					  TaskRunner*);
    virtual bool		canSetColTabSequence() const { return true; }
    virtual int			nrAttribs() const { return 1; }
    virtual const ColTab::MapperSetup* getColTabMapperSetup(int,int) const;
    
    enum DisplayMode		{ None, ZeroOffset, FullOnSections, 
				  ZeroOffsetOnSections, FullOnGathers };
    				DeclareEnumUtils(DisplayMode);
    void			setDisplayMode(DisplayMode);
    DisplayMode			getDisplayMode() const;

    void			setLineStyle(const LineStyle&);
    LineStyle			getLineStyle() const;

    void			setMarkerStyle(const MarkerStyle3D&,bool updat);
    virtual bool		hasColor() const { return true; }
    virtual Color		getColor() const;
    const char**		markerColorNames()const;
    const char**		displayModeNames()const;
    bool			hasParents() const;

protected:
    void			clearAll();

    				~PSEventDisplay();
    void			otherObjectsMoved( const ObjectSet<
	    				const SurveyObject>&, int whichobj );

    void			setDisplayTransformation(const mVisTrans*);
    const mVisTrans*		getDisplayTransformation() const;

    //bool			filterBinID(const BinID&) const;
    				/*!<\returns true if the binid should not be
				     viewed. */


    //visBase::PickStyle*		pickstyle_;
    void			eventChangeCB(CallBacker*);
    void			eventForceReloadCB(CallBacker*);


    //TypeSet<HorSampling>	sectionranges_;
    struct ParentAttachedObject
    {
					ParentAttachedObject(int);
					~ParentAttachedObject();
	visBase::DataObjectGroup*	separator_;
	visBase::IndexedPolyLine*	lines_;

	ObjectSet<visBase::Marker>	markers_;
	ObjectSet<PreStack::EventSet>	eventsets_;
	HorSampling			hrg_;

	const int			parentid_;
    };

    void				updateDisplay();
    void				updateDisplay(ParentAttachedObject*);
    void				clearDisplay(ParentAttachedObject*);
    void				retriveParents();
    float				getMoveoutComp(const TypeSet<float>&,
					const TypeSet<float>&) const;

    visBase::DrawStyle*			linestyle_;
    const mVisTrans*			displaytransform_;
    ObjectSet<ParentAttachedObject>	parentattached_;
    
    DisplayMode				displaymode_;

    PreStack::EventManager*		eventman_;
    int					horid_;
    Interval<float>			qualityrange_;
    float				offsetscale_;

    MarkerColor				markercolor_;
    ColTab::Mapper			ctabmapper_;
    ColTab::Sequence			ctabsequence_;
    MarkerStyle3D			markerstyle_;
    visBase::DataObjectGroup*		eventseeds_;
};


}; //namespace

#endif

