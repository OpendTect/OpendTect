#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiemattribmod.h"
#include "uiapplserv.h"
#include "uistring.h"
#include "attribdescid.h"
#include "datacoldef.h"
#include "emposid.h"
#include "multiid.h"
#include "ranges.h"

namespace Attrib { class DescSet; }
namespace Geometry { class RandomLine; }
class DataPointSet;
class NLAModel;
class TaskRunner;

class uiAttrSurfaceOut;
class uiAttrTrcSelOut;
class uiCreate2DGrid;
class uiFlatUnflatCube;
class uiHorizonShiftDialog;
class uiImportHorizon2D;
class uiSeisEventSnapper;
class uiStratAmpCalc;

/*!
\ingroup uiEMAttrib
\brief Part Server for Attribute handling on EarthModel objects
*/

mExpClass(uiEMAttrib) uiEMAttribPartServer : public uiApplPartServer
{mODTextTranslationClass(uiEMAttribPartServer)
public:
				uiEMAttribPartServer(uiApplService&);
				~uiEMAttribPartServer();

    const char*			name() const override	{ return "EMAttribs"; }

    static int			evCalcShiftAttribute()		{ return 0; }
    static int			evHorizonShift()		{ return 1; }
    static int			evStoreShiftHorizons()		{ return 2; }
    static int			evShiftDlgOpened()		{ return 3; }
    static int			evShiftDlgClosedCancel()	{ return 4; }
    static int			evShiftDlgClosedOK()		{ return 5; }
    static int			evDisplayEMObject()		{ return 6; }

    enum HorOutType		{ OnHor, AroundHor, BetweenHors,
				  FlattenSingle };
    void			createHorizonOutput(HorOutType);
    void			createHorizonOutput(HorOutType,const MultiID&);

    void			snapHorizon(const EM::ObjectID&,bool is2d);
    void			computeStratAmp();

    void			setNLAModel(const NLAModel*,const MultiID&);
    void			setDescSet(const Attrib::DescSet*);

    void			showHorShiftDlg(const EM::ObjectID&,
						const VisID& visid,
						const BoolTypeSet& attrenabled,
						float initialshift,
						bool canaddattrib);
    void			fillHorShiftDPS(ObjectSet<DataPointSet>&,
					TaskRunner*);

    const DataColDef&		sidDef() const;
    const BoolTypeSet&		initialAttribStatus() const
				    { return initialattribstatus_; }
    float			initialShift() const { return initialshift_; }

    float			getShift() const;
    void			setAttribID( const Attrib::DescID& id )
				{ attribid_ = id; }
    VisID			getShiftedObjectVisID() const;
    void			setAttribIdx(int);
    Attrib::DescID		attribID() const	{ return attribid_; }
    int				attribIdx() const	{ return attribidx_; }
					//Works only in case of Shift Dlg
    int				textureIdx() const;
					//Works only in case of Shift Dlg
    StepInterval<float>		shiftRange() const;
    const char*			getAttribBaseNm() const;
    void			import2DHorizon();
    void			create2DGrid(const Geometry::RandomLine*);

    const TypeSet<EM::ObjectID>& getEMObjIDs() const	{ return emobjids_; }

    mDeprecated("Use setNLAModel")
    void			setNLA( const NLAModel* mdl, const MultiID& id )
				{ setNLAModel( mdl, id ); }

protected:

    NLAModel*			nlamodel_		= nullptr;
    Attrib::DescSet*		descset_		= nullptr;
    MultiID			nlaid_;
    uiHorizonShiftDialog*	horshiftdlg_		= nullptr;
    uiSeisEventSnapper*		uiseisevsnapdlg_	= nullptr;
    uiImportHorizon2D*		uiimphor2ddlg_		= nullptr;
    uiFlatUnflatCube*		flattendlg_		= nullptr;
    TypeSet<EM::ObjectID>	emobjids_;

    float			initialshift_;
    BoolTypeSet			initialattribstatus_;

    int				shiftidx_ = 10;
    Attrib::DescID		attribid_;
    int				attribidx_ = 0;

    uiAttrTrcSelOut*		aroundhor2ddlg_		= nullptr;
    uiAttrTrcSelOut*		aroundhor3ddlg_		= nullptr;
    uiAttrTrcSelOut*		betweenhor2ddlg_	= nullptr;
    uiAttrTrcSelOut*		betweenhor3ddlg_	= nullptr;
    uiAttrSurfaceOut*		surfattr2ddlg_		= nullptr;
    uiAttrSurfaceOut*		surfattr3ddlg_		= nullptr;
    uiCreate2DGrid*		crgriddlg_		= nullptr;
    uiStratAmpCalc*		stratampdlg_		= nullptr;

    void			calcDPS(CallBacker*);
    void			horShifted(CallBacker*);
    void			shiftDlgClosed(CallBacker*);

    void			readyForDisplayCB(CallBacker*);
};
