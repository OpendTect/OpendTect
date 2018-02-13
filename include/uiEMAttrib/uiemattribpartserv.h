#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:          September 2006
________________________________________________________________________

-*/

#include "uiemattribmod.h"
#include "uiapplserv.h"
#include "uistring.h"
#include "attribdescid.h"
#include "datacoldef.h"
#include "dbkey.h"
#include "ranges.h"

namespace Attrib { class DescSet; }
class DataPointSet;
class NLAModel;
class TaskRunner;

class uiAttrSurfaceOut;
class uiAttrTrcSelOut;
class uiHorizonShiftDialog;
class uiImportHorizon2D;
class uiSeisEventSnapper;

/*!
\ingroup uiEMAttrib
\brief Part Server for Attribute handling on EarthModel objects
*/

mExpClass(uiEMAttrib) uiEMAttribPartServer : public uiApplPartServer
{mODTextTranslationClass(uiEMAttribPartServer);
public:
				uiEMAttribPartServer(uiApplService&);
				~uiEMAttribPartServer();

    const char*			name() const		{ return "EMAttribs"; }

    static int			evCalcShiftAttribute()		{ return 0; }
    static int			evHorizonShift()		{ return 1; }
    static int			evStoreShiftHorizons()		{ return 2; }
    static int			evShiftDlgOpened()		{ return 3; }
    static int			evShiftDlgClosedCancel()	{ return 4; }
    static int			evShiftDlgClosedOK()		{ return 5; }
    static int			evDisplayEMObject()		{ return 6; }

    enum HorOutType		{ OnHor, AroundHor, BetweenHors };
    void			createHorizonOutput(HorOutType,bool is2d);

    void			snapHorizon(const DBKey&,bool is2d);

    void			setNLA( const NLAModel* mdl, const DBKey& id )
				{ nlamodel_ = mdl; nlaid_ = id; }

    void			showHorShiftDlg(const DBKey&,
						int visid,
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
    void			setAttribID( Attrib::DescID id )
				{ attribid_ = id; }
    int				getShiftedObjectVisID() const;
    void			setAttribIdx(int);
    Attrib::DescID		attribID() const	{ return attribid_; }
    int				attribIdx() const	{ return attribidx_; }
					//Works only in case of Shift Dlg
    int				textureIdx() const;
					//Works only in case of Shift Dlg
    StepInterval<float>		shiftRange() const;
    const char*			getAttribBaseNm() const;
    void			import2DHorizon();

    const DBKeySet& getEMObjIDs() const	{ return emobjids_; }

protected:

    const NLAModel*		nlamodel_;
    DBKey			nlaid_;
    uiHorizonShiftDialog*	horshiftdlg_;
    uiSeisEventSnapper*		uiseisevsnapdlg_;
    uiImportHorizon2D*		uiimphor2ddlg_;
    DBKeySet			emobjids_;

    float			initialshift_;
    BoolTypeSet			initialattribstatus_;

    int				shiftidx_;
    Attrib::DescID		attribid_;
    int				attribidx_;

    uiAttrTrcSelOut*		aroundhor2ddlg_;
    uiAttrTrcSelOut*		aroundhor3ddlg_;
    uiAttrTrcSelOut*		betweenhor2ddlg_;
    uiAttrTrcSelOut*		betweenhor3ddlg_;
    uiAttrSurfaceOut*		surfattr2ddlg_;
    uiAttrSurfaceOut*		surfattr3ddlg_;

    void			calcDPS(CallBacker*);
    void			horShifted(CallBacker*);
    void			shiftDlgClosed(CallBacker*);

    void			readyForDisplayCB(CallBacker*);
};
