#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Kristofer Tingdahl
 Date:		May 2011
 RCS:		$Id$
________________________________________________________________________

-*/

#include "uivoxelconnectivityfiltermod.h"
#include "uivolprocstepdlg.h"
#include "voxelconnectivityfilter.h"
#include "enums.h"
#include "factory.h"

namespace VolProc
{

mExpClass(uiVoxelConnectivityFilter) uiVoxelConnectivityFilter :
public uiStepDialog
{ mODTextTranslationClass(uiVoxelConnectivityFilter);
public:
    mDefaultFactoryInstanciationBase(
	    VolProc::VoxelConnectivityFilter::sFactoryKeyword(),
	    VolProc::VoxelConnectivityFilter::sFactoryDisplayName())
	    mDefaultFactoryInitClassImpl( uiStepDialog, createInstance );



protected:

				uiVoxelConnectivityFilter(uiParent*,
					    VoxelConnectivityFilter*,bool is2d);
    static uiStepDialog*	createInstance(uiParent*,Step*,bool is2d);
    bool			acceptOK(CallBacker*);

    void		updateFieldsCB(CallBacker*);

    uiGenInput*		cutofftypefld_;
    uiGenInput*		cutoffvalfld_;
    uiGenInput*		cutoffrangefld_;

    uiGenInput*		connectivityfld_;
    uiGenInput*		minbodysizefld_;
    uiGenInput*		acceptoutputfld_;
    uiGenInput*		acceptvaluefld_;
    uiGenInput*		rejectoutputudffld_;
    uiGenInput*		rejectoutputvalfld_;
};

}; //namespace

