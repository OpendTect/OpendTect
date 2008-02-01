#ifndef visprestackviewer_h
#define visprestackviewer_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Yuancheng Liu
 Date:		May 2007
 RCS:		$Id: visprestackviewer.h,v 1.6 2008-02-01 23:19:09 cvsyuancheng Exp $
________________________________________________________________________

-*/

#include "vissurvobj.h"
#include "visobject.h"

namespace visBase 
{
    class DepthTabPlaneDragger;
    class FaceSet;
    class FlatViewer;
    class PickStyle;
};

namespace visSurvey 
{ 
    class PlaneDataDisplay; 
    class Seis2DDisplay;
};


namespace PreStackView
{

class PreStackViewer :	public visBase::VisualObjectImpl,
			public visSurvey::SurveyObject
{
public:
    static PreStackViewer*	create()
				mCreateDataObj( PreStackViewer );
    void			allowShading(bool yn);
    void			setColor(Color);			
    void			setMultiID(const MultiID& mid);
    bool			isInlCrl() const { return true; }
    
    bool			is3DSeis() const;
    DataPack::ID		getDataPackID() const;

    				//3D case
    bool			setPosition(const BinID&);
    const BinID&		getPosition() const;
    void			setSectionDisplay(visSurvey::PlaneDataDisplay*);
    const visSurvey::PlaneDataDisplay* getSectionDisplay() const;	
   
   				//2D case 
    const visSurvey::Seis2DDisplay*    getSeis2DDisplay() const;    
    void			setSeis2DDisplay(visSurvey::Seis2DDisplay*,
	    					 int trcnr);
    void			setSeis2DData(DataPack::ID);
    int				traceNr() const 	  { return trcnr_; }
    const char*			lineName();

    bool                        displayAutoWidth() const { return autowidth_; }
    void                        displaysAutoWidth(bool yn);
    bool                        displayOnPositiveSide() const {return posside_;}
    void                        displaysOnPositiveSide(bool yn);
    float                       getFactor() { return factor_; }
    void			setFactor(float scale);
    float                       getWidth() { return width_; }
    void			setWidth(float width);
    BinID			getBinID() { return bid_; }
    MultiID			getMultiID() { return mid_; }
    void			getMousePosInfo(const visBase::EventInfo&,
	    					const Coord3&,
				  		BufferString& val,
						BufferString& info) const;
    void			 otherObjectsMoved( 
	    				const ObjectSet<const SurveyObject>&, 
					int whichobj );

    void			fillPar(IOPar&, TypeSet<int>&) const;
    int				usePar(const IOPar&);

    static const char*		sKeyBinID()	{ return "PreStack BinID"; }
    static const char*		sKeyMultiID()	{ return "PreStack MultiID"; }
    static const char*		sKeySectionID() { return "Section ID"; }
    static const char*		sKeyFactor()	{ return "PSViewer Factor"; }
    static const char*		sKeyWidth() 	{ return "PSViewer Width"; }
    static const char*		sKeyiAutoWidth(){ return "PSViewer AutoWidth"; }
    static const char*		sKeySide() 	{ return "PSViewer ShowSide"; }

protected:
    					~PreStackViewer();
    void				setDisplayTransformation(mVisTrans*);
    void				dataChangedCB(CallBacker*);
    void				sectionMovedCB(CallBacker*);

    void				draggerMotion(CallBacker*);
    void				finishedCB(CallBacker*);
    void				mouseMoveCB(CallBacker*);

    BinID				bid_;
    visBase::DepthTabPlaneDragger* 	planedragger_;
    visBase::FaceSet*                   draggerrect_;
    visBase::FlatViewer*		flatviewer_;
    visBase::Material*			draggermaterial_;
    visBase::PickStyle*			pickstyle_;
    
    MultiID				mid_;
    visSurvey::PlaneDataDisplay*	section_;
    visSurvey::Seis2DDisplay*		seis2d_;
    int 				trcnr_;
    Coord				basedirection_;
    Coord				seis2dpos_;

    bool				posside_;
    bool				autowidth_;
    float				factor_;
    float				width_;
    Interval<float>			offsetrange_;
    Interval<float>			zrg_;
};

}; //namespace

#endif
