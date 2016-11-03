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
				AttribProbeLayer(
					DispType dt=AttribProbeLayer::VD);
				~AttribProbeLayer();
    mDeclAbstractMonitorableAssignment( AttribProbeLayer );

    const char*			layerType() const;
    void			fillPar(IOPar&) const;
    bool			usePar(const IOPar&);
    static ChangeType		cDataChange()		{ return 2; }
    static ChangeType		cColTabSeqChange()	{ return 3; }
    static ChangeType		cColTabMapperChange()	{ return 4; }

    bool			hasData() const
				{ return !attribdpid_.isInvalid(); }
    virtual void		invalidateData();
    virtual DataPack::MgrID	getDataPackManagerID() const
				{ return DataPackMgr::SeisID(); }
    void			setSelSpec(const Attrib::SelSpec&);
    mImplSimpleMonitoredGet(getSelSpec,Attrib::SelSpec,attrspec_)
    mImplSimpleMonitoredGet(getAttribDataPack,DataPack::ID,attribdpid_)
    void			setAttribDataPack(DataPack::ID);
    mImplSimpleMonitoredGetSet(inline,getColTab,setColTab,
			       ColTab::Sequence,attrcoltab_,cColTabSeqChange())
    mImplSimpleMonitoredGetSet(inline,getColTabMapper,setColTabMapper,
			       ColTab::MapperSetup,attrmapper_,
			       cColTabMapperChange())
    mImplSimpleMonitoredGet(getDispType,DispType,disptype_)
    bool			useStoredColTabPars();


    static ProbeLayer*		createFrom(const IOPar&);
    static void			initClass();
    static const char*		sFactoryKey();
    static const char*		sAttribDataPackID();
protected:
    Attrib::SelSpec&		attrspec_;
    DataPack::ID		attribdpid_;
    ColTab::Sequence		attrcoltab_;
    ColTab::MapperSetup		attrmapper_;
    DispType			disptype_;
    ConstRefMan<DataPack>	attrdp_;

    void			updateDataPack();
};
