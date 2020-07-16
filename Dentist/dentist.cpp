//
//  dentist.cpp
//  Dentist
//
//  Copyright © 2020 unixb0y. All rights reserved.
//

#include <Headers/kern_api.hpp>
#include <Headers/plugin_start.hpp>

#include "dentist.hpp"

Connector *Connector::instance = nullptr;

static const char *kextIOBluetoothFamily[] { "/System/Library/Extensions/IOBluetoothFamily.kext/Contents/PlugIns/IOBluetoothHostControllerUSBTransport.kext/Contents/MacOS/IOBluetoothHostControllerUSBTransport" };

static KernelPatcher::KextInfo kextIOBluetooth { "com.apple.iokit.IOBluetoothHostControllerUSBTransport", kextIOBluetoothFamily, 1, {true}, {}, KernelPatcher::KextInfo::Unloaded };

void Connector::init() {
    instance = this;
    
    IOLog("Initializing");
    DBGLOG("dentist", "Initializing");

    auto error = lilu.onKextLoad(&kextIOBluetooth, 1,
    [](void *user, KernelPatcher &patcher, size_t index, mach_vm_address_t address, size_t size) {
        Connector *dentist = static_cast<Connector*>(user);
        dentist->processKext(patcher, index, address, size);
    }, this);

    if (error != LiluAPI::Error::NoError) {
        SYSLOG(kThisKextID, "Failed to register onKextLoad: %d", error);
        IOLog("Failed to register onKextLoad: %d", error);
    } else {
        SYSLOG(kThisKextID, "Successfully loaded");
        IOLog("Successfully loaded");
    }
}

int64_t Connector::IOBluetoothHostControllerUSBTransport_SendHCIRequest(void *that, uint8_t *a2, uint64_t a3)
{
    Connector::instance->ioBTInstance = that;
    DBGLOG("dentist", "IOBluetoothHostControllerUSBTransport::SendHCIRequest is called. First Byte is: ");
//    IOLog("IOBluetoothHostControllerUSBTransport::SendHCIRequest is called.");

    uint8_t data_bkup[a3+1];
    lilu_os_memcpy(data_bkup, a2, a3);
    data_bkup[a3] = 0x02;
    
    int64_t result = FunctionCast(IOBluetoothHostControllerUSBTransport_SendHCIRequest,
        instance->orgIOBluetoothHostControllerUSBTransport_SendHCIRequest)(that, a2, a3);
    
    // Send data sent to BT chip to userspace
    ctl_enqueuedata(Connector::instance->mCtlref, Connector::instance->mClient.sc_unit, (uint8_t*)&data_bkup, sizeof(data_bkup), 0);

    return result;
}

int64_t Connector::IOBluetoothHostControllerUSBTransport_ReceiveInterruptData(void *that, uint8_t *data, uint64_t len, bool unknown) {
    Connector::instance->ioBTInstance = that;
    DBGLOG("dentist", "IOBluetoothHostControllerUSBTransport::ReceiveInterruptData is called.");
//    IOLog("IOBluetoothHostControllerUSBTransport::ReceiveInterruptData is called.");
    
    uint8_t data_bkup[len+1];
    lilu_os_memcpy(data_bkup, data, len);
    data_bkup[len] = 0x03;

    int64_t result = FunctionCast(IOBluetoothHostControllerUSBTransport_ReceiveInterruptData,
        instance->orgIOBluetoothHostControllerUSBTransport_ReceiveInterruptData)(that, data, len, unknown);
    
    // Send data received from BT chip to userspace
    ctl_enqueuedata(Connector::instance->mCtlref, Connector::instance->mClient.sc_unit, (uint8_t*)&data_bkup, sizeof(data_bkup), 0);
    
    return result;
}

void Connector::processKext(KernelPatcher &patcher, size_t index, mach_vm_address_t address, size_t size) {
    IOLog("Process Kext. %s -- index: %zu, loadIndex: %zu", kextIOBluetooth.id, index, kextIOBluetooth.loadIndex);
    
    if (index != kextIOBluetooth.loadIndex || mDone) {
        return;
    }
    
    DBGLOG("dentist", "%s", kextIOBluetooth.id);
    
    errno_t err;
    bzero(&mCtl, sizeof(mCtl));
    mCtl.ctl_id = 0;
    mCtl.ctl_unit = 0;
    strcpy(mCtl.ctl_name, kThisKextID, strlen(kThisKextID));
    mCtl.ctl_flags = CTL_FLAG_PRIVILEGED;
    mCtl.ctl_send = ctlHandleWrite;
    mCtl.ctl_getopt = ctlHandleGet;
    mCtl.ctl_setopt = ctlHandleSet;
    mCtl.ctl_connect = ctlHandleConnect;
    mCtl.ctl_disconnect = ctlHandleDisconnect;
    err = ctl_register(&mCtl, &mCtlref);
    
    u_int32_t ctl_id = mCtl.ctl_id;
    SYSLOG(kThisKextID, "ctl_register status: %d, id: %d", err, ctl_id);
    IOLog("ctl_register status: %d, id: %d", err, ctl_id);
    
    KernelPatcher::RouteRequest request {
        "__ZN37IOBluetoothHostControllerUSBTransport14SendHCIRequestEPhy",
        IOBluetoothHostControllerUSBTransport_SendHCIRequest,
        orgIOBluetoothHostControllerUSBTransport_SendHCIRequest
    };
    if (!patcher.routeMultiple(index, &request, 1, address, size)) {
        SYSLOG("dentist", "failed to resolve %s %d", request.symbol, patcher.getError());
        IOLog("failed to resolve %s %d", request.symbol, patcher.getError());
        patcher.clearError();
    }
    
    KernelPatcher::RouteRequest request2 {
        "__ZN37IOBluetoothHostControllerUSBTransport20ReceiveInterruptDataEPvjb",
        IOBluetoothHostControllerUSBTransport_ReceiveInterruptData,
        orgIOBluetoothHostControllerUSBTransport_ReceiveInterruptData
    };
    if (!patcher.routeMultiple(index, &request2, 1, address, size)) {
        SYSLOG("dentist", "failed to resolve %s %d", request.symbol, patcher.getError());
        IOLog("failed to resolve %s %d", request.symbol, patcher.getError());
        patcher.clearError();
    }
}

errno_t Connector::ctlHandleSet(kern_ctl_ref ctlref, unsigned int unit, void *userdata, int opt, void *data, size_t len) {
    errno_t error = KERN_SUCCESS;
    
    SYSLOG(kThisKextID, "ctlHandleSet, opt: %d", opt);
    IOLog("ctlHandleSet, opt: %d", opt);
    
    return error;
}

errno_t Connector::ctlHandleGet(kern_ctl_ref ctlref, unsigned int unit, void *userdata, int opt, void *data, size_t *len) {
    errno_t error = KERN_SUCCESS;
    SYSLOG(kThisKextID, "ctlHandleGet, opt: %d", opt);
    IOLog("ctlHandleGet, opt: %d", opt);
    return error;
}

errno_t Connector::ctlHandleConnect(kern_ctl_ref ctlref, struct sockaddr_ctl *sac, void **unitinfo) {
    errno_t error = KERN_SUCCESS;
    
    SYSLOG(kThisKextID, "ctlHandleConnect");
    IOLog("ctlHandleConnect. Unit: %d", sac->sc_unit);
    
    Connector::instance->mClient.sc_unit = sac->sc_unit;
    Connector::instance->mClient.connected = true;
    Connector::instance->mEnabled = false;

    return error;
}
errno_t Connector::ctlHandleDisconnect(kern_ctl_ref ctlref, unsigned int unit, void *unitinfo) {
    errno_t error = KERN_SUCCESS;
    
    SYSLOG(kThisKextID, "ctlHandleDisconnect");
    IOLog("ctlHandleDisconnect");
    
    Connector::instance->mClient.connected = false;

    return error;
}

errno_t Connector::ctlHandleWrite(kern_ctl_ref ctlref, unsigned int unit, void *userdata, mbuf_t m, int flags) {
    errno_t error = KERN_SUCCESS;
    
    SYSLOG(kThisKextID, "ctlHandleHandleWrite");
    IOLog("ctlHandleHandleWrite");
    
    uint8_t *data = (uint8_t*)mbuf_data(m);
    uint8_t data_bkup[mbuf_len(m)+1];
    lilu_os_memcpy(data_bkup, data, mbuf_len(m));
    data_bkup[mbuf_len(m)] = 0x01;
    
    Connector::instance->IOBluetoothHostControllerUSBTransport_SendHCIRequest(Connector::instance->ioBTInstance, data, mbuf_len(m));
    
    error = ctl_enqueuedata(Connector::instance->mCtlref, Connector::instance->mClient.sc_unit, (uint8_t*)&data_bkup, sizeof(data_bkup), 0);
    if (error != KERN_SUCCESS) {
        SYSLOG(kThisKextID, "ctl_enquedata failed: %d", error);
    }
    
    return error;
}
