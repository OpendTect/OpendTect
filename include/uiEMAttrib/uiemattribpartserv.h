#ifndef uiemattribpartserv_h
#define uiemattribpartserv_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:          September 2006
 RCS:           $Id$
________________________________________________________________________

-*/

#include "uiemattribmod.h"
#include "uiapplserv.h"
#include "attribdescid.h"
#include "datacoldef.h"
#include "emposid.h"
#include "multiid.h"
#include "ranges.h"

namespace Attrib { class DescSet; }
class DataPointSet;
class NLAModel;
class TaskRunner;

class uiHorizonShiftDialog;
class uiImportFaultStickSet2D;
class uiImportHorizon2D;
class uiSeisEventSnapper;

/*!
\ingroup uiEMAttrib
\brief Part Server for Attribute handling on EarthModel objects
*/

mExpClass(uiEMAttrib) uiEMAttribPartServer : public uiApplPartServer
{
public:
				uiEMAttribPartServer(uiApplService&);
				~uiEMAttribPartServer()	{}

    const char*			name() const		{ return "EMAttribs"; }

    static int			evCalcShiftAttribute()		{ return 0; }
    static int			evHorizonShift()		{ return 1; }
    static int			evStoreShiftHorizons()		{ return 2; }
    static int			evShiftDlgOpened()		{ return 3; }
    static int			evShiftDlgClosedCancel()	{ return 4; }
    static int			evShiftDlgClosedOK()		{ return 5; }
    static int			evDisplayEMObject()		{ return 6; }

    enum HorOutType		{ OnHor, AroundHor, BetweenHors };
    void			createHorizonOutput(HorOutType);

    void			snapHorizon(const EM::ObjectID&,bool is2d);

    void			setNLA( const NLAModel* mdl, const MultiID& id )
				{ nlamodel_ = mdl; nlaid_ = id; }
    void			setDescSet( const Attrib::DescSet* ads )
				{ descset_ = ads; }

    void			showHorShiftDlg(const EM::ObjectID&,
	    			    const BoolTypeSet& attrenabled,
				    float initialshift,bool canaddattrib);
    void			fillHorShiftDPS(ObjectSet<DataPointSet>&,
	    				TaskRunner*);

    const DataColDef&		sidDef() const;
    const BoolTypeSet&		initialAttribStatus() const
				    { return initialattribstatus_; }
    float			initialShift() const { return initialshift_; }

    float			getShift() const;
    void			setAttribID( Attrib::DescID id )
    				{ attribid_ = id; }
    void			setAttribIdx(int);
    Attrib::DescID		attribID() const	{ return attribid_; }
    int				attribIdx() const	{ return attribidx_; }
					//Works only in case of Shift Dlg
    int				textureIdx() const;
    					//Works only in case of Shift Dlg
    StepInterval<float>		shiftRange() const;
    const char*			getAttribBaseNm() const;
    void			import2DHorizon();
    void			import2DFaultStickset(const char* type);

    const TypeSet<EM::ObjectID>& getEMObjIDs() const	{ return emobjids_; }

protected:

    const NLAModel*		nlamodel_;
    const Attrib::DescSet*	descset_;
    MultiID			nlaid_;
    uiHorizonShiftDialog*	horshiftdlg_;
    uiSeisEventSnapper*		uiseisevsnapdlg_;
    uiImportHorizon2D*		uiimphor2ddlg_;
    uiImportFaultStickSet2D*	uiimpfss2ddlg_;
    TypeSet<EM::ObjectID>	emobjids_;

    float			initialshift_;
    BoolTypeSet			initialattribstatus_;

    int				shiftidx_;
    Attrib::DescID		attribid_;
    int				attribidx_;

    void			calcDPS(CallBacker*);
    void			horShifted(CallBacker*);
    void			shiftDlgClosed(CallBacker*);

    void			readyForDisplayCB(CallBacker*);
};

#endif

