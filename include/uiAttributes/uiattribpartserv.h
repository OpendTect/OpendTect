#ifndef uiattribpartserv_h
#define uiattribpartserv_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Bril
 Date:          Feb 2002
 RCS:           $Id: uiattribpartserv.h,v 1.54 2009-05-20 14:56:06 cvshelene Exp $
________________________________________________________________________

-*/

#include "uiapplserv.h"

#include "attribdescid.h"
#include "attribsel.h"
#include "datapack.h"
#include "menuhandler.h"
#include "multiid.h"
#include "position.h"
#include "timer.h"

namespace Attrib
{
    class DataCubes;
    class Data2DHolder;
    class Desc;
    class DescSet;
    class DescSetMan;
    class EngineMan;
    class SelInfo;
    class SelSpec;
};

namespace VolProc { class Chain; }

class BinID;
class BinIDValueSet;
class BufferStringSet;
class CubeSampling;
class Executor;
class IOObj;
class IOPar;
class LineKey;
class NLAModel;
class DataPointSet;
class SeisTrcBuf;
class SeisTrcInfo;
class uiAttribDescSetEd;
class uiAttribCrossPlot;
namespace Pick { class Set; }
template <class T> class Interval;
template <class T> class Array3D;


/*! \brief Service provider for application level - Attributes */

mClass uiAttribPartServer : public uiApplPartServer
{
public:
			uiAttribPartServer(uiApplService&);
			~uiAttribPartServer();

    const char*		name() const			{ return "Attributes"; }

    static const int	evDirectShowAttr();
    			//!< User requested direct redisplay of curAttrDesc()
    static const int	evNewAttrSet();
    			//!< FYI
    static const int	evAttrSetDlgClosed();
    			//!< AttributeSet window closes
    static const int	evEvalAttrInit();
    			//!< Initialization of evaluation dialog
    static const int	evEvalCalcAttr();
    			//!< User wants to evaluate current attribute
    static const int	evEvalShowSlice();
    			//!< Display slice
    static const int	evEvalStoreSlices();
    			//!< Store slices
    static const int	evShowSelPts();
    			//!< Show Selected Points from CrossPlot in Workspace
    static const int	evRemoveSelPts();
    			//!< Remove Selected Points from CrossPlot in Workspace
    static const int	evEvalUpdateName();
    			//!< Update name in tree after evaluation dlg closed
    static const int	objNLAModel2D();
    			//!< Request current 2D NLAModel* via getObject()
    static const int	objNLAModel3D();
    			//!< Request current 3D NLAModel* via getObject()

    void		handleAutoSet();
    void		manageAttribSets();
    const Attrib::DescSet* curDescSet(bool) const;
    void		getDirectShowAttrSpec(Attrib::SelSpec&) const;
    bool		setSaved(bool) const;
    void		saveSet(bool);
    bool		editSet(bool);
    			//!< returns whether new AttribDescSet has been created
    bool		attrSetEditorActive() const	{ return attrsetdlg_; }
    void		updateSelSpec(Attrib::SelSpec&) const;

    bool		selectAttrib(Attrib::SelSpec&,const char*,bool);
    bool		setPickSetDirs(Pick::Set&,const NLAModel*);
    void		outputVol(MultiID&,bool);
    bool		replaceSet(const IOPar&,bool);
    bool		addToDescSet(const char*,bool);
    int			getSliceIdx() const		{ return sliceidx_; }
    void		getPossibleOutputs(bool,BufferStringSet&) const;

    void		setTargetSelSpec(const Attrib::SelSpec&);
    const TypeSet<Attrib::SelSpec>& getTargetSelSpecs() const
			    { return targetspecs_; }

    DataPack::ID	createOutput(const CubeSampling&,DataPack::ID);
    const Attrib::DataCubes* createOutput(const CubeSampling&,
				          const Attrib::DataCubes* prevslcs=0);
    bool		createOutput(DataPointSet&,int firstcol =0);
    bool		createOutput(ObjectSet<DataPointSet>&,
	    			     int firstcol =0);
    bool		createOutput(const BinIDValueSet&,SeisTrcBuf&,
	    			     TypeSet<BinID>*,TypeSet<BinID>*);
    DataPack::ID	createRdmTrcsOutput(const Interval<float>& zrg,
	    				    TypeSet<BinID>* path,
					    TypeSet<BinID>* trueknotspos);
    DataPack::ID	create2DOutput(const CubeSampling&,const LineKey&);

    bool		isDataAngles(bool) const;
			/*!<\returns true if the target data is an
				     angle, i.e. -PI==PI. */
    bool		isDataClassified(const Array3D<float>&) const;

    Attrib::DescID	getStoredID(const LineKey&,bool is2d,int selout=-1);
    IOObj*		getIOObj(const Attrib::SelSpec&) const;

    bool		extractData(ObjectSet<DataPointSet>&);
    bool		createAttributeSet(const BufferStringSet&,
	    				   Attrib::DescSet*);

    const NLAModel*	getNLAModel(bool) const;
    void		setNLAName( const char* nm )	{ nlaname_ = nm; }

    void		resetMenuItems();
    MenuItem*         	storedAttribMenuItem(const Attrib::SelSpec&,bool,bool);
    MenuItem*         	calcAttribMenuItem(const Attrib::SelSpec&,bool,bool);
    MenuItem*         	nlaAttribMenuItem(const Attrib::SelSpec&,bool,bool);
    MenuItem*         	zDomainAttribMenuItem(const Attrib::SelSpec&,
	    				      const char* zkey,bool,bool);
    void         	fillInStoredAttribMenuItem(MenuItem*,bool,bool,
	    					   const Attrib::SelSpec&,bool,
						   bool needext=false);

    bool		handleAttribSubMenu(int mnuid,Attrib::SelSpec&) const;
    bool		handleMultiCompSubMenu(int,bool,const char*,
	    				       TypeSet<Attrib::SelSpec>&) const;

    void		setEvaluateInfo(bool ae,bool as)
			{ alloweval_=ae; allowevalstor_=as; }

    void		fillPar(IOPar&,bool) const;
    void		usePar(const IOPar&,bool);

    void                set2DEvent( bool is2d )		{ is2devsent_ = is2d; }
    bool                is2DEvent()                     { return is2devsent_; }
    int			use3DMode() const;
    			//!< If you have services that can work on 2D or 3D
    			//!< 0 = 2D, 1 = 3D, -1 = user cancel
    Attrib::DescSet*	getUserPrefDescSet() const;
    			//!< For services that can work on 2D or 3D
    void		showXPlot(CallBacker*);

    void		doVolProc();
    void		createVolProcOutput( const IOObj* setupsel = 0);

protected:

    MenuItem            stored2dmnuitem_;
    MenuItem            stored3dmnuitem_;
    MenuItem            calc2dmnuitem_;
    MenuItem            calc3dmnuitem_;
    MenuItem            nla2dmnuitem_;
    MenuItem            nla3dmnuitem_;
    MenuItem            zdomain2dmnuitem_;
    MenuItem            zdomain3dmnuitem_;
    MenuItem            steering2dmnuitem_;
    MenuItem            steering3dmnuitem_;
    MenuItem            multcomp2d_;
    MenuItem            multcomp3d_;
    ObjectSet<MenuItem> linesets2dmnuitem_;

    Attrib::DescSetMan*	adsman2d_;
    Attrib::DescSetMan*	adsman3d_;
    uiAttribCrossPlot*	uiattrxplot_;
    const Attrib::Desc*	dirshwattrdesc_;
    uiAttribDescSetEd*	attrsetdlg_;
    Timer		attrsetclosetim_;
    bool                is2devsent_;

    Attrib::EngineMan*	createEngMan(const CubeSampling* cs=0,
	    			     const char* lk=0);

    void		directShowAttr(CallBacker*);

    void		sendPickEvent(CallBacker*);
    void		sendRemoveEvent(CallBacker*);
    void		showEvalDlg(CallBacker*);
    void		calcEvalAttrs(CallBacker*);
    void		showSliceCB(CallBacker*);
    void		evalDlgClosed(CallBacker*);

    void		attrsetDlgClosed(CallBacker*);
    void		attrsetDlgCloseTimTick(CallBacker*);

    Attrib::DescSetMan* getAdsMan(bool) const;
    Attrib::DescSetMan* getAdsMan(bool);

    Attrib::DescID	targetID(bool is2d,int nr=0) const;

    BufferStringSet	get2DStoredLSets(const Attrib::SelInfo&) const;
    BufferStringSet	get2DStoredItems(const MultiID&,bool,bool) const;
    void		insert2DStoredItems(const BufferStringSet&,int,int,
	    				    bool,MenuItem*,
					    const Attrib::SelSpec&,bool,bool,
					    bool onlymultcomp=false);
    void		insertNumerousItems(const BufferStringSet&,
	    				    const Attrib::SelSpec&,
					    bool,bool,bool);

    static const char*	attridstr();
    BufferString	nlaname_;

    bool		alloweval_;
    bool		allowevalstor_;
    int			sliceidx_;
    Attrib::DescSet*	evalset;
    TypeSet<Attrib::SelSpec> targetspecs_;

    VolProc::Chain*	volprocchain_;
};


/*!\mainpage Attribute User Interface

 The main thing here is handling the attribute set editor. Other
 services of the uiAttribPartServer are selection and volume output.

 The Attribute set editor is a pretty complex piece of user interface. The left
 and top part of the window are fixed. They handle the 'common' things in
 attribute set editing. The right part is defined via the uiAttribFactory .

 The problem that was facing us was that we needed a user interface that could
 be dynamically extended. Further more,much of the needed functionality is
 common to all attributes. Thus, we defined:

 - A base class for all attribute editors (uiAttrDescEd)
 - A factory of uiAttrDescEdCreater, creating uiAttrDescEd instances from
   the name of the attribute

 The uiAttrDescEd itself already has a lot of implemented functionality,
 leaving only things specific for that particular attribute to be defined.
 Once such a subclass is fully defined, a uiAttrDescEdCreater instance must be
 added to the factory to make it active.

 To see how such a new attribute can be created aswell as a user interface for
 it, take a look at the uiTut seismic 'attribute style' example.

*/


#endif
