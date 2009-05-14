#ifndef uihorizonshiftdlg_h
#define uihorizonshiftdlg_h
/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Satyaki Maitra
 Date:          Feb 2009
 RCS:           $Id: uihorizonshiftdlg.h,v 1.3 2009-05-14 09:05:51 cvssatyaki Exp $
________________________________________________________________________

-*/

#include "uidialog.h"
#include "attribdescid.h"
#include "bufstringset.h"
#include "datapack.h"
#include "emposid.h"
#include "position.h"

class BufferStringSet;
class DataPointSet;
class uiAttrSel;
class uiCheckBox;
class uiGenInput;
class uiGenInput;
class uiSliderExtra;
class uiPushButton;
template <class T> class StepInterval;
namespace EM { class Horizon3D; }
namespace Attrib { class DescSet; }

class uiHorizonShiftDialog : public uiDialog
{
public:
			uiHorizonShiftDialog(uiParent*,const EM::ObjectID& id,
					     const Attrib::DescSet& );
			~uiHorizonShiftDialog();
    const EM::Horizon3D&	horizon3D()		{ return *emhor3d_; }
    const StepInterval<float>&  shiftIntv()		{ return shiftrg_; }
    void			setShiftIntv( const StepInterval<float>& rg )
							{ shiftrg_ = rg; }
    TypeSet<int>&		attribIds()		{ return attribids_; }
    const TypeSet<int>&		attribIds() const	{ return attribids_; }
    Attrib::DescID		attribID() const	{ return attrid_; }
    float			curShift() const	{ return curshift_; }
    int				curShiftIdx() const	{ return curshiftidx_; }
    void			setShiftIdx( int idx )
							{ curshiftidx_ = idx; }
    void			setAttribIds( const TypeSet<int>& ids )
							{ attribids_ = ids; }
    bool			doStore() const;
    const EM::ObjectID&		emID() const		{ return emid_; }
    const char*			getAttribName() const
    				{ return attribnm_.buf(); }
    
    Notifier<uiHorizonShiftDialog>	calcAttribPushed;
    Notifier<uiHorizonShiftDialog>	horShifted;

protected:

    void			setNameFldSensitive(CallBacker*);
    void			setSliderRange(CallBacker*);
    void			calcAttrib(CallBacker*);
    void			shiftCB(CallBacker*);
    bool                	acceptOK(CallBacker*);

    uiAttrSel*			attrinpfld_;
    uiGenInput*			rangeinpfld_;
    uiSliderExtra*		slider_;
    uiPushButton*		calbut_;
    uiCheckBox*			storefld_;
    uiGenInput*			namefld_;

    Attrib::DescID		attrid_;
    float			curshift_;
    int				curshiftidx_;
    BufferString		attribnm_;
    StepInterval<float>		shiftrg_;
    StepInterval<float>		calcshiftrg_;
    TypeSet<int>		attribids_;
    EM::Horizon3D*		emhor3d_;
    EM::ObjectID		emid_;
};

#endif
