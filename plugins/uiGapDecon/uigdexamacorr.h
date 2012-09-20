#ifndef uigdexamacorr_h
#define uigdexamacorr_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        H. Huck
 Date:          Sep 2006
 RCS:           $Id$
________________________________________________________________________

-*/

#include "uiattrdesced.h"
#include "cubesampling.h"
#include "linekey.h"
#include "attribdescid.h"

template <class T> class Array2D;
namespace Attrib { class EngineMan; class DescSet; class DataCubes;
    		   class Data2DHolder; class Processor; }
class uiFlatViewMainWin;
class FlatDataPack;

/*! \brief GapDecon Attribute autocorrelation preview in a 2d viewer */

class GapDeconACorrView
{
public:
    			GapDeconACorrView(uiParent*);
    			~GapDeconACorrView();
    bool                computeAutocorr(bool);
    void                createAndDisplay2DViewer(bool);
    void		setCubeSampling( CubeSampling cs )	{ cs_ = cs; }
    void		setLineKey( LineKey lk )		{ lk_ = lk; }
    void		setAttribID( Attrib::DescID id )	{ attribid_=id;}
    void                setDescSet(Attrib::DescSet*);
    			//<! descset becomes mine!

protected:

    Attrib::EngineMan*	createEngineMan();
    void		createFD2DDataPack(bool,const Attrib::Data2DHolder&);
    void		createFD3DDataPack(bool,Attrib::EngineMan*,
	    				   Attrib::Processor*);
    void		displayWiggles(bool,bool);
    void		setUpViewWin(bool);

    uiFlatViewMainWin*		examwin_;
    uiFlatViewMainWin*		qcwin_;
    CubeSampling		cs_;
    LineKey			lk_;
    Attrib::DescID		attribid_;
    Attrib::DescSet*    	dset_;
    uiParent*			parent_;
    BufferString		examtitle_;
    BufferString		qctitle_;

    FlatDataPack*		fddatapackqc_;
    FlatDataPack*		fddatapackexam_;
};


#endif
