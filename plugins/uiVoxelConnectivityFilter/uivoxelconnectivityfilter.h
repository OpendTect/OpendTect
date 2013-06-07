#ifndef uivoxelconnectivityfilter_h
#define uivoxelconnectivityfilter_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Kristofer Tingdahl
 Date:		May 2011
 RCS:		$Id: uivoxelconnectivityfilter.h,v 1.3 2011/08/24 13:19:43 cvskris Exp $
________________________________________________________________________

-*/

#include "uivolprocstepdlg.h"
#include "voxelconnectivityfilter.h"
#include "enums.h"
#include "factory.h"

namespace VolProc
{

mClass uiVoxelConnectivityFilter : public uiStepDialog
{
public:
    mDefaultFactoryInstanciationBase(
	    VolProc::VoxelConnectivityFilter::sFactoryKeyword(),
	    VolProc::VoxelConnectivityFilter::sFactoryDisplayName())
	    mDefaultFactoryInitClassImpl( uiStepDialog, createInstance );



protected:

    				uiVoxelConnectivityFilter(uiParent*,
					    VoxelConnectivityFilter*);
    static uiStepDialog*	createInstance(uiParent*,Step*);
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

#endif
