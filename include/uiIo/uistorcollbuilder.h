#pragma once
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          July 2018
________________________________________________________________________

-*/

#include "uiiomod.h"
#include "uigroup.h"
class uiButton;
class uiButtonGroup;
class IOObjContext;
class uiTable;


mExpClass(uiIo) uiStorableCollectionBuilder : public uiGroup
{
public:

    mExpClass(uiIo) Setup
    {
    public:
			Setup( int nrcolumns, const uiString& objtypnm)
			    : nrcols_(nrcolumns)
			    , objtypename_(objtypnm)
			    , pixwidth_(800)	{}

	mDefSetupMemb(uiString,objtypename);
	mDefSetupMemb(int,nrcols);
	mDefSetupMemb(int,pixwidth);

	void		set(const IOObjContext&);

	CallBack	addcb_;
	CallBack	edcb_;
	CallBack	rmcb_;
	CallBack	opencb_;
	CallBack	savecb_;
	bool		canopen_ = true;
    };

                        uiStorableCollectionBuilder(uiParent*,const Setup&);
			~uiStorableCollectionBuilder();

    uiTable*		table()			{ return tbl_; }
    void		displayButtons(bool);
    void		updLooks();
    void		setCanOpen(bool yn);

    bool		canOpen() const;

protected:

    uiTable*		tbl_;
    uiButtonGroup*	bgrp_;
    uiButton*		addbut_;
    uiButton*		rmbut_;
    uiButton*		edbut_;
    uiButton*		openbut_;
    uiButton*		savebut_;

    void		selChgCB(CallBacker*);

};
