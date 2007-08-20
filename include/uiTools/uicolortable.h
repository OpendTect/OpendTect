#ifndef uicolortable_h
#define uicolortable_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Bert/Nanne
 Date:          Aug 2007
 RCS:           $Id: uicolortable.h,v 1.2 2007-08-20 16:00:55 cvsbert Exp $
________________________________________________________________________

-*/

#include "uigroup.h"
class uiLineEdit;
class uiComboBox;
class ColorTable;
class ColTabScaling;
class uiColorTableCanvas;


class uiColorTable : public uiGroup
{
public:

			uiColorTable(uiParent*,bool vertical);
			   //!< Editable
			uiColorTable(uiParent*,const ColorTable&,
				     Interval<float>,bool vertical);
			   //!< Display only
			~uiColorTable();

    const ColorTable&	colorTable() const	{ return coltab_; }
    const ColTabScaling& tableScaling() const	{ return scale_; }

    void		setTable(const char*,bool emitnotif=true);
    void		setTable(const ColorTable&,bool emitnotif=true);
    void		setTableScaling(const ColTabScaling&,bool emit=true);

    Notifier<uiColorTable>	tableSelected;
    Notifier<uiColorTable>	scaleChanged;

protected:

    ColorTable&		coltab_;
    ColTabScaling&	scale_;
    const bool		vertical_;

    uiColorTableCanvas*	dispfld_;
    uiLineEdit*		minfld_;
    uiLineEdit*		maxfld_;
    uiComboBox*		selfld_;

    void		tabSel(CallBacker*);
    void		rangeEntered(CallBacker*);
    void		doEdit(CallBacker*);
    void		doFlip(CallBacker*);
    void		doApply4Man(CallBacker*);
    void		makeSymmetrical(CallBacker*);
    void		mkCanvas(const Interval<float>&);

    bool		isEditable() const	{ return maxfld_; }
    void		fillTabList();
    void		setRangeFields();
};


#endif
