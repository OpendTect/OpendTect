#ifndef uiodviewer2dmgr_h
#define uiodviewer2dmgr_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Umesh Sinha
 Date:		Apr 2010
 RCS:		$Id$
________________________________________________________________________

-*/

#include "uiodmainmod.h"
#include "callback.h"
#include "datapack.h"
#include "emposid.h"
#include "geom2dintersections.h"
#include "uigeom.h"
#include "uigraphicsviewbase.h"
#include "uiodapplmgr.h"

class uiFlatViewer;
class uiODViewer2D;
class uiTreeFactorySet;
class MouseEventHandler;
class TrcKeyZSampling;
namespace Attrib	{ class SelSpec; }

mExpClass(uiODMain) uiODViewer2DMgr : public CallBacker
{ mODTextTranslationClass(uiODViewer2DMgr);
public:

    struct SelectedAuxPos
    {
				SelectedAuxPos(int auxposidx=-1,bool isx1=true,
					       bool selected=false)
				    : auxposidx_(auxposidx)
				    , isx1_(isx1)
				    , oldauxpos_(mUdf(float))
				    , isselected_(selected)	{}
	int			auxposidx_;
	bool			isx1_;
	bool			isselected_;
	float			oldauxpos_;
	bool			isValid() const		{ return auxposidx_>=0;}
    };

    uiODViewer2D*		find2DViewer(int id,bool byvisid);
    uiODViewer2D*		find2DViewer(const MouseEventHandler&);
    uiODViewer2D*		find2DViewer(const Pos::GeomID&);
    uiODViewer2D*		find2DViewer(const TrcKeyZSampling&);

    int				displayIn2DViewer(DataPack::ID,
					const Attrib::SelSpec&,bool wva,
					float initialx1pospercm=mUdf(float),
					float initialx2pospercm=mUdf(float));
    void			displayIn2DViewer(int visid,int attribid,
						  bool wva);
    void			remove2DViewer(int id,bool byvisid);

    uiTreeFactorySet*		treeItemFactorySet2D()	{ return tifs2d_; }
    uiTreeFactorySet*		treeItemFactorySet3D()	{ return tifs3d_; }
    void			removeHorizon3D(EM::ObjectID emid);
    void			addHorizon3Ds(const TypeSet<EM::ObjectID>&);
    void			addNewTrackingHorizon3D(EM::ObjectID mid);
    void			getLoadedHorizon3Ds(
					TypeSet<EM::ObjectID>&) const;
    void			removeHorizon2D(EM::ObjectID emid);
    void			getLoadedHorizon2Ds(
					TypeSet<EM::ObjectID>&) const;
    void			addHorizon2Ds(const TypeSet<EM::ObjectID>&);
    void			addNewTrackingHorizon2D(EM::ObjectID mid);

    static int			cNameColumn()		{ return 0; }
    static int			cColorColumn()		{ return 1; }

    static const char*		sKeyVisID()		{ return "VisID"; }
    static const char*		sKeyAttrID()		{ return "Attrib ID"; }
    static const char*		sKeyWVA()		{ return "WVA"; }

protected:

				uiODViewer2DMgr(uiODMain*);
				~uiODViewer2DMgr();

    uiODViewer2D&		addViewer2D(int visid);
    ObjectSet<uiODViewer2D>     viewers2d_;
    Line2DInterSectionSet*	l2dintersections_;
    SelectedAuxPos		selauxpos_;
    uiGraphicsViewBase::ODDragMode prevdragmode_;

    uiTreeFactorySet*		tifs2d_;
    uiTreeFactorySet*		tifs3d_;

    uiODMain&			appl_;

    inline uiODApplMgr&         applMgr()     { return appl_.applMgr(); }
    inline uiVisPartServer&     visServ()     { return *applMgr().visServer(); }

    void			viewWinClosedCB(CallBacker*);
    void			vw2DPosChangedCB(CallBacker*);
    void			homeZoomChangedCB(CallBacker*);
    void			mouseClickCB(CallBacker*);
    void			mouseClickedCB(CallBacker*);
    void			mouseMoveCB(CallBacker*);

    void			create2DViewer(const uiODViewer2D& curvwr2d,
					       const TrcKeyZSampling& newtkzs,
					       const uiWorldPoint& initcentr);
				/*!< \param newtkzs is the new TrcKeyZSampling
				for which a new uiODViewer2D will be created.
				\param curvwr2d is the current 2D Viewer of
				which the newly created 2D Viewer will inherit
				Attrib::SelSpec and other display properties.*/
    void			attachNotifiers(uiODViewer2D*);
    Line2DInterSection::Point	intersectingLineID(const uiODViewer2D*,
	    					   float pos) const;
    int				intersection2DIdx(Pos::GeomID) const;
    void			reCalc2DIntersetionIfNeeded(Pos::GeomID);
    void			setAllIntersectionPositions();
    void			setVWR2DIntersectionPositions(uiODViewer2D*);
    void			handleLeftClick(uiODViewer2D*);
    void			setAuxPosLineStyles(uiFlatViewer&);
    void			setupHorizon3Ds(uiODViewer2D*);
    void			setupHorizon2Ds(uiODViewer2D*);

    void			fillPar(IOPar&) const;
    void			usePar(const IOPar&);

    friend class                uiODMain;
};

#endif

