#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Satyaki Maitra
 Date:		September 2016
________________________________________________________________________

-*/

#include "attributeenginemod.h"
#include "attribsel.h"
#include "probe.h"
#include "coltabsequence.h"
#include "coltabmapper.h"
#include "datapack.h"

namespace Attrib { class SelSpec; }

mExpClass(AttributeEngine) AttribProbeLayer : public ProbeLayer
{
public:

    enum DispType		{ VD, Wiggle, RGB, };
    typedef ColTab::MapperSetup	MapperSetup;

				AttribProbeLayer(
					DispType dt=AttribProbeLayer::VD);
				~AttribProbeLayer();
				mDeclMonitorableAssignment(AttribProbeLayer);

    const char*			layerType() const;
    void			fillPar(IOPar&) const;
    bool			usePar(const IOPar&);
    static ChangeType		cDataChange()		{ return 2; }
    static ChangeType		cColSeqChange()		{ return 3; }
    static ChangeType		cMapperSetupChange()	{ return 4; }

    bool			hasData() const
				{ return !attribdpid_.isInvalid(); }
    virtual void		invalidateData();
    virtual DataPack::MgrID	getDataPackManagerID() const
				{ return DataPackMgr::SeisID(); }

    mImplSimpleMonitoredGet(	selSpec,Attrib::SelSpec,attrspec_)
    mImplSimpleMonitoredGet(	dataPackID,DataPack::ID,attribdpid_)
    mImplSimpleMonitoredGet(	dispType,DispType,disptype_)
    mImplSimpleMonitoredGetSet(	inline,colSeq,setColSeq,
					ConstRefMan<ColTab::Sequence>,colseq_,
					cColSeqChange())
    void			setSelSpec(const Attrib::SelSpec&);
    void			setDataPackID(DataPack::ID);
    ConstRefMan<MapperSetup>	mapperSetup() const;
    void			setMapperSetup(const MapperSetup&);

    bool			useStoredColTabPars();
    static ProbeLayer*		createFrom(const IOPar&);
    static void			initClass();
    static const char*		sFactoryKey();
    static const char*		sAttribDataPackID();
    static const char*		sAttribColTabName();

protected:

    Attrib::SelSpec&		attrspec_;
    DataPack::ID		attribdpid_;
    ConstRefMan<ColTab::Sequence> colseq_;
    RefMan<MapperSetup>		mappersetup_;
    DispType			disptype_;
    ConstRefMan<DataPack>	attrdp_;

    void			updateDataPack();

};
