#ifndef surveysectionprinfo_h
#define surveysectionprinfo_h

/*+
___________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Satyaki Maitra
 Date:		September 2016
___________________________________________________________________

-*/

#include "generalmod.h"
#include "bufstringset.h"
#include "odpresentationmgr.h"
#include "trckeyzsampling.h"
#include "enums.h"

mExpClass(General) SectionLayerPresentationInfo
{
public:
    virtual		~SectionLayerPresentationInfo()	{};
    SectionLayerPresentationInfo* clone() const;
    virtual void	fillPar(IOPar&) const;
    virtual bool	usePar(const IOPar&);
    const char*		sectionLayerType() const
    			{ return sectionlayertypekey_; }
protected:
    BufferString	sectionlayertypekey_;
};


mExpClass(General) SectionLayerPresentationInfoFactory
{
public:
    typedef SectionLayerPresentationInfo* (*CreateFunc)(const IOPar&);

    void			  	addCreateFunc(CreateFunc,const char*);
    SectionLayerPresentationInfo* 	create(const IOPar&);
protected:
    TypeSet<CreateFunc>			createfuncs_;
    BufferStringSet			keys_;
};

mGlobal(General) SectionLayerPresentationInfoFactory& SLPRIFac();

typedef int IDType;
mDefIntegerIDType(IDType,SurveySectionID);

mExpClass(General) SurveySectionPresentationInfo
				: public OD::ObjPresentationInfo
{
public:

    enum SectionType			{ InLine=0, CrossLine=1, Line2D=2,
					  ZSlice=3, RdmLine=4 };
    					mDeclareEnumUtils( SectionType );
    static const char*			sKeySectionID();
    static const char*			sKeySectionType();
    static const char*			sKeyNrLayers();
    static const char*			sKeyLayer();
	
					SurveySectionPresentationInfo();
    					~SurveySectionPresentationInfo();
    uiString				getName() const;
    void				fillPar(IOPar&) const;
    bool				usePar(const IOPar&);
    bool				isSameObj(
	    				const OD::ObjPresentationInfo&) const;
    static SurveySectionID		getNewSectionID();
    void				setSectionID( SurveySectionID sid )
					{ sectionid_ = sid; }
    SurveySectionID			sectionID() const
					{ return sectionid_; }
    SectionType				sectionType() const
					{ return sectiontype_; }
    void				setSectionType( SectionType sectype )
					{ sectiontype_ = sectype; }
    const TrcKeyZSampling&		sectionPos() const
					{ return sectionpos_; }
    void				setSectionPos(const TrcKeyZSampling& sp)
					{ sectionpos_ = sp; }
    void				addSectionLayer(
					    SectionLayerPresentationInfo* linfo)
					{ layerprinfos_ += linfo; }
    int 				nrLayers() const
    					{ return layerprinfos_.size(); }
    const SectionLayerPresentationInfo*	getLyerPRInfo(int idx) const
					{ return layerprinfos_.validIdx(idx)
					    	? layerprinfos_[idx] : 0; }

    static OD::ObjPresentationInfo*	createFrom( const IOPar&);
    static void				initClass();
    static const char*			sFactoryKey()
    					{ return "Survey Section"; }
protected:
    ObjectSet<SectionLayerPresentationInfo> layerprinfos_;
    SectionType				sectiontype_;
    TrcKeyZSampling			sectionpos_;
    SurveySectionID			sectionid_;
    //TODO Randomline not handled yet

};

#endif
