#ifndef uiemattribpartserv_h
#define uiemattribpartserv_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:          September 2006
 RCS:           $Id: uiemattribpartserv.h,v 1.16 2009/11/19 04:04:12 cvssatyaki Exp $
________________________________________________________________________

-*/

#include "uiapplserv.h"
#include "attribdescid.h"
#include "datacoldef.h"
#include "emposid.h"
#include "multiid.h"
#include "ranges.h"

namespace Attrib { class DescSet; }
template <class T> class TypeSet;
class BufferStringSet;
class DataPointSet;
class NLAModel;
class TaskRunner;
class uiHorizonShiftDialog;

/*! \brief Part Server for Attribute handling on EarthModel objects */

mClass uiEMAttribPartServer : public uiApplPartServer
{
public:
				uiEMAttribPartServer(uiApplService&);
				~uiEMAttribPartServer()	{}

    const char*			name() const		{ return "EMAttribs"; }

    static const int		evCalcShiftAttribute()		{ return 0; }
    static const int		evHorizonShift()		{ return 1; }
    static const int		evStoreShiftHorizons()		{ return 2; }
    static const int		evShiftDlgOpened()		{ return 3; }
    static const int		evShiftDlgClosedCancel()	{ return 4; }
    static const int		evShiftDlgClosedOK()		{ return 5; }

    enum HorOutType		{ OnHor, AroundHor, BetweenHors };
    void			createHorizonOutput(HorOutType);

    bool			snapHorizon(const EM::ObjectID&,MultiID& nwid,
	    				    bool& displaynew,bool is2d);
    				/*<return bool is overwrite old hor or not, if
				   No and displaynew, newmid will be set. */

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
    const BoolTypeSet&		initialAttribStatus() const { return initialattribstatus_; }
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
    void			import2DHorizon() const;
    void			import2DFaultStickset(const char* type);

protected:

    const NLAModel*		nlamodel_;
    const Attrib::DescSet*	descset_;
    MultiID			nlaid_;
    uiHorizonShiftDialog*	horshiftdlg_;


    float			initialshift_;
    BoolTypeSet			initialattribstatus_;

    int				shiftidx_;
    Attrib::DescID		attribid_;
    int				attribidx_;

    void			calcDPS(CallBacker*);
    void			horShifted(CallBacker*);
    void			shiftDlgClosed(CallBacker*);
};

/*!\mainpage EMAttrib User Interface

  Here you will find all attribute handling regarding EarthModel objects.
  The uiEMAttribPartServer delivers the services needed.

*/


#endif
