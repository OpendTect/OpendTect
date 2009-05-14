#ifndef uiemattribpartserv_h
#define uiemattribpartserv_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Nanne Hemstra
 Date:          September 2006
 RCS:           $Id: uiemattribpartserv.h,v 1.9 2009-05-14 09:05:51 cvssatyaki Exp $
________________________________________________________________________

-*/

#include "uiapplserv.h"
#include "attribdescid.h"
#include "emposid.h"
#include "multiid.h"
#include "ranges.h"

namespace Attrib { class DescSet; }
template <class T> class TypeSet;
class BufferStringSet;
class DataPointSet;
class NLAModel;
class uiHorizonShiftDialog;

/*! \brief Part Server for Attribute handling on EarthModel objects */

mClass uiEMAttribPartServer : public uiApplPartServer
{
public:
				uiEMAttribPartServer(uiApplService&);
				~uiEMAttribPartServer()	{}

    const char*			name() const		{ return "EMAttribs"; }

    static const int		evCalcShiftAttribute()	{ return 0; }
    static const int		evHorizonShift()	{ return 1; }
    static const int		evStoreShiftHorizons()	{ return 2; }
    static const int		evShiftDlgOpened()	{ return 3; }
    static const int		evShiftDlgClosed()	{ return 4; }
    static const int		evShiftDlgFinalised()	{ return 5; }

    enum HorOutType		{ OnHor, AroundHor, BetweenHors };
    void			createHorizonOutput(HorOutType);

    void			snapHorizon(const EM::ObjectID&);

    void			setNLA( const NLAModel* mdl, const MultiID& id )
				{ nlamodel_ = mdl; nlaid_ = id; }
    void			setDescSet( const Attrib::DescSet* ads )
				{ descset_ = ads; }

    void			showHorShiftDlg(uiParent*,const EM::ObjectID&,
	    					const TypeSet<int>&);
    void			getDataPointSet(const EM::ObjectID&,
					const EM::SectionID&, DataPointSet&,
					float shift); 
    void			fillHorShiftDPS(ObjectSet<DataPointSet>&);

    float			getShift() const;
    void			setAttribID( Attrib::DescID id )
    				{ attribid_ = id; }
    void			setAttribIdx(int);
    Attrib::DescID		attribID() const	{ return attribid_; }
    const TypeSet<int>&		attribIds() const;
    int				attribIdx() const	{ return attribidx_; }
					//Works only in case of Shift Dlg
    int				textureIdx() const;
    					//Works only in case of Shift Dlg
    const StepInterval<float>&	shiftRange() const	{ return shiftrg_; }
    void			setShiftRange( const StepInterval<float>& rg )
    				{ shiftrg_ = rg; }
    const char*			getAttribBaseNm() const
	                        { return shiftattrbasename_.buf(); }
    void			import2DHorizon() const;
    void			import2DFaultStickset(const char* type);

protected:

    const NLAModel*		nlamodel_;
    const Attrib::DescSet*	descset_;
    MultiID			nlaid_;
    uiHorizonShiftDialog*	horshiftdlg_;

    StepInterval<float>		shiftrg_;
    int				shiftidx_;
    Attrib::DescID		attribid_;
    int				attribidx_;
    BufferString		shiftattrbasename_;

    void			calcDPS(CallBacker*);
    void			horShifted(CallBacker*);
    void			shiftDlgClosed(CallBacker*);
};

/*!\mainpage EMAttrib User Interface

  Here you will find all attribute handling regarding EarthModel objects.
  The uiEMAttribPartServer delivers the services needed.

*/


#endif
