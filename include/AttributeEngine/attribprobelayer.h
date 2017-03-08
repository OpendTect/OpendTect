#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Satyaki Maitra
 Date:		September 2016
________________________________________________________________________

-*/

#include "attributeenginemod.h"
#include "probe.h"
#include "datapack.h"
#include "attribsel.h"
#include "coltabmapper.h"
#include "coltabsequence.h"
#include "datadistribution.h"


mExpClass(AttributeEngine) AttribProbeLayer : public ProbeLayer
{
public:

    typedef ColTab::Sequence		Sequence;
    typedef ColTab::MapperSetup		MapperSetup;
    typedef DataDistribution<float>	DistribType;

    enum DispType		{ VD, Wiggle, RGB };

				AttribProbeLayer(DispType dt=VD);
				~AttribProbeLayer();
				mDeclMonitorableAssignment(AttribProbeLayer);

    const char*			layerType() const;
    void			fillPar(IOPar&) const;
    bool			usePar(const IOPar&);

    bool			hasData() const
				{ return !attribdpid_.isInvalid(); }
    virtual void		invalidateData();
    virtual DataPack::MgrID	getDataPackManagerID() const
				{ return DataPackMgr::SeisID(); }

    mImplSimpleMonitoredGet(	selSpec,Attrib::SelSpec,attrspec_)
    mImplSimpleMonitoredGet(	dataPackID,DataPack::ID,attribdpid_)
    mImplSimpleMonitoredGet(	dispType,DispType,disptype_)
    mImplSimpleMonitoredGet(	selectedComponent,int,selcomp_)
    mImplSimpleMonitoredGet(	distribution,RefMan<DistribType>,distrib_)
    mImplSimpleMonitoredGetSet(	inline,colSeq,setColSeq,ConstRefMan<Sequence>,
	    				colseq_,cColSeqChange())
    mImplSimpleMonitoredGetSet(	inline,mapperSetup,setMapperSetup,
					RefMan<MapperSetup>,mappersetup_,
					cMapperSetupChange())

    void			setSelSpec(const Attrib::SelSpec&);
    int				nrAvialableComponents() const;
    void			setSelectedComponent(int);
    void			setDataPackID(DataPack::ID);
    bool			useStoredColTabPars();

    static ProbeLayer*		createFrom(const IOPar&);
    static void			initClass();

    static ChangeType		cDataChange()		{ return 2; }
    static ChangeType		cColSeqChange()		{ return 3; }
    static ChangeType		cMapperSetupChange()	{ return 4; }
    static ChangeType		cSelCompChange()	{ return 5; }

    static const char*		sFactoryKey();
    static const char*		sAttribDataPackID();
    static const char*		sAttribColTabName();

protected:

    Attrib::SelSpec&		attrspec_;
    DispType			disptype_;
    DataPack::ID		attribdpid_;
    ConstRefMan<DataPack>	attrdp_;
    int				selcomp_;
    ConstRefMan<Sequence>	colseq_;
    RefMan<MapperSetup>		mappersetup_;
    RefMan<DistribType>		distrib_;

    void			handleDataPackChange();

};
