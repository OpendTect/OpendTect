#ifndef uiwelldlgs_h
#define uiwelldlgs_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:          October 2003
 RCS:           $Id$
________________________________________________________________________

-*/

#include "uiwellmod.h"
#include "uiselsimple.h"
#include "multiid.h"
#include "ranges.h"
#include "position.h"

class uiButtonGroup;
class uiCheckBox;
class uiD2TModelGroup;
class uiFileInput;
class uiGenInput;
class uiLabel;
class uiPushButton;
class uiTable;
class uiTableImpDataSel;
class uiUnitSel;
class uiWellSel;
class BufferStringSet;

namespace Table { class FormatDesc; }
namespace Well { class Data; class Track; class D2TModel; class Log; }


/*! \brief Dialog for D2T Model editing. */

mExpClass(uiWell) uiWellTrackDlg : public uiDialog
{ mODTextTranslationClass(uiWellTrackDlg);
public:
			uiWellTrackDlg(uiParent*,Well::Data&);
			~uiWellTrackDlg();

protected:

    Well::Data&		wd_;
    Well::Track&	track_;
    Well::Track*	orgtrack_;

    Table::FormatDesc&	fd_;
    uiTable*		tbl_;
    uiCheckBox*		zinftfld_;
    uiGenInput*		wellheadxfld_;
    uiGenInput*		wellheadyfld_;
    uiGenInput*		kbelevfld_;

    Coord3		origpos_;
    float		origgl_;

    bool		fillTable(CallBacker* cb=0);
    void		fillSetFields(CallBacker* cb=0);
    bool		updNow(CallBacker*);
    void		readNew(CallBacker*);
    bool		rejectOK(CallBacker*);
    bool		acceptOK(CallBacker*);
    void		exportCB(CallBacker*);
    void		updateXpos(CallBacker*);
    void		updateYpos(CallBacker*);
    void		updateKbElev(CallBacker*);

    double		getX(int row) const;
    double		getY(int row) const;
    double		getZ(int row) const;
    float		getMD(int row) const;
    void		setX(int row,double);
    void		setY(int row,double);
    void		setZ(int row,double);
    void		setMD(int row,float);

    void		updatePos(bool isx);
    bool		rowIsIncomplete(int) const;
};


mExpClass(uiWell) uiD2TModelDlg : public uiDialog
{
public:
			uiD2TModelDlg(uiParent*,Well::Data&,bool chksh);
			~uiD2TModelDlg();

protected:

    Well::Data&		wd_;
    bool		cksh_;
    Well::D2TModel*	orgd2t_; // Must be declared *below* others
    float		origreplvel_;

    uiTable*		tbl_;
    uiCheckBox*		unitfld_;
    uiCheckBox*		timefld_;
    uiGenInput*		replvelfld_;

    void		fillTable(CallBacker*);
    void		fillReplVel(CallBacker*);
    bool		getFromScreen();
    void		updNow(CallBacker*);
    void		updReplVelNow(CallBacker*);
    void		dtpointChangedCB(CallBacker*);
    void		dtpointRemovedCB(CallBacker*);
    bool		updateDtpointDepth(int row);
    bool		updateDtpointTime(int row);
    bool		updateDtpoint(int row,float oldval);
    void		readNew(CallBacker*);
    bool		rejectOK(CallBacker*);
    bool		acceptOK(CallBacker*);
    void		expData(CallBacker*);
    void		getModel(Well::D2TModel&);

    void		getColLabels(BufferStringSet&) const;
    int			getTVDGLCol() const;
    int			getTVDSDCol() const;
    int			getTVDSSCol() const;
    int			getTimeCol() const;
    int			getVintCol() const;
    bool		rowIsIncomplete(int row) const;
    int			getPreviousCompleteRowIdx(int row) const;
    int			getNextCompleteRowIdx(int row) const;
};


/*! \brief Dialog for user made wells */

class uiColorInput;

mExpClass(uiWell) uiNewWellDlg : public uiGetObjectName
{
public:
			uiNewWellDlg(uiParent*);
			~uiNewWellDlg();

    const Color&	getWellColor();
    const char*		getName() const	{ return name_; }

protected:

    uiColorInput*	colsel_;
    BufferString	name_;
    BufferStringSet*	nms_;

    virtual bool	acceptOK(CallBacker*);
    const BufferStringSet& mkWellNms();
};


/* brief edit the logs's unit of measure */

mExpClass(uiWell) uiWellLogUOMDlg : public uiDialog
{
public:
			uiWellLogUOMDlg(uiParent*,Well::Log&);

protected:

    uiUnitSel*		unfld_;
    Well::Log&		log_;

    bool		acceptOK(CallBacker*);
};


#endif
