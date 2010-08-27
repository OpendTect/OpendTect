#ifndef uiattribpanel_h
#define uiattribpanel_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        H. Huck
 Date:          Jun 2010
________________________________________________________________________

-*/

#include "uiattrdesced.h"
#include "cubesampling.h"
#include "linekey.h"
#include "attribdescid.h"

class FlatDataPack;
class uiFlatViewMainWin;

template <class T> class Array2D;
namespace Attrib { class EngineMan; class DescSet; class DataCubes;
    		   class Data2DHolder; class Processor; }

/*! \brief Attribute preview in a 2d viewer */

mClass uiAttribPanel
{
public:
    			uiAttribPanel(uiParent*);
    			~uiAttribPanel();
    void                compAndDispAttrib(Attrib::DescSet*,
	    				  const Attrib::DescID&,
					  const CubeSampling&,const LineKey&);
    			//<! descset becomes mine!

protected:

    FlatDataPack*	computeAttrib();
    Attrib::EngineMan*	createEngineMan();
    virtual void	createAndDisplay2DViewer(FlatDataPack*);
    FlatDataPack*	createFDPack(const Attrib::Data2DHolder&) const;
    FlatDataPack*	createFDPack(Attrib::EngineMan*,
	    			     Attrib::Processor*) const;

    virtual const char*		getProcName()	{ return "Computing attribute";}
    virtual const char*		getPackName()	{ return "Attribute pack"; }
    virtual const char*		getPanelName()	{ return "Attribute preview"; }

    uiFlatViewMainWin*		flatvwin_;
    CubeSampling		cs_;
    LineKey			lk_;
    Attrib::DescID		attribid_;
    Attrib::DescSet*    	dset_;
    uiParent*			parent_;
};

#endif
