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
#include "uigeom.h"
#include "uiodapplmgr.h"

class uiFlatViewer;
class uiODViewer2D;
class uiTreeFactorySet;
class Line2DInterSectionSet;
class MouseEventHandler;
class TrcKeyZSampling;
namespace Attrib { class SelSpec; }


mExpClass(uiODMain) uiODViewer2DMgr : public CallBacker
{ mODTextTranslationClass(uiODViewer2DMgr);
public:

    uiODViewer2D*		find2DViewer(int id,bool byvisid);
    uiODViewer2D*		find2DViewer(const MouseEventHandler&);

    int				displayIn2DViewer(DataPack::ID,
						  const Attrib::SelSpec&,
						  bool wva, Pos::GeomID);
    void			displayIn2DViewer(int visid,int attribid,
						  bool wva);
    void			remove2DViewer(int id,bool byvisid);

    uiTreeFactorySet*		treeItemFactorySet2D()	{ return tifs2d_; }
    uiTreeFactorySet*		treeItemFactorySet3D()	{ return tifs3d_; }

    float			defTrcsPerCM() const	{ return deftrcspercm_;}
    float			defZPerCM() const	{ return defzpercm_;}
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
    float			deftrcspercm_;
    float			defzpercm_;

    uiTreeFactorySet*		tifs2d_;
    uiTreeFactorySet*		tifs3d_;

    uiODMain&			appl_;

    bool			isVWR2DDisplayed(const Pos::GeomID&) const;
    inline uiODApplMgr&         applMgr()     { return appl_.applMgr(); }
    inline uiVisPartServer&     visServ()     { return *applMgr().visServer(); }

    void			viewWinClosedCB(CallBacker*);
    void			vw2DPosChangedCB(CallBacker*);
    void			homeZoomChangedCB(CallBacker*);
    void			mouseClickCB(CallBacker*);

    void			create2DViewer(const uiODViewer2D& curvwr2d,
					       const TrcKeyZSampling& newtkzs);
				/*!< \param newtkzs is the new TrcKeyZSampling
				for which a new uiODViewer2D will be created.
				\param curvwr2d is the current 2D Viewer of
				which the newly created 2D Viewer will inherit
				Attrib::SelSpec and other display properties.*/
    void			attachNotifiers(uiODViewer2D*);
    int				intersection2DIdx(Pos::GeomID) const;
    bool			intersection2DReCalNeeded(Pos::GeomID) const;
    void			reCalc2DIntersetionIfNeeded(Pos::GeomID);
    void			setAllIntersectionPositions();
    void			setVWR2DIntersectionPositions(uiODViewer2D*);

    void			fillPar(IOPar&) const;
    void			usePar(const IOPar&);

    friend class                uiODMain;
};

#endif

