#define NX_SERVICE_ASSUME_NON_DOMAIN
#include <string.h>
#include "service_guard.h"
#include "runtime/hosversion.h"
#include "services/btmu.h"
#include "services/applet.h"

static Service g_btdrvIBtmUserCore;

static Result _btmuGetSession(Service* srv, Service* srv_out, u32 cmd_id);

NX_GENERATE_SERVICE_GUARD(btmu);

Result _btmuInitialize(void) {
    Result rc=0;
    Service srv={0};

    rc = btmuGetServiceSession(&srv);
    if (R_SUCCEEDED(rc)) rc = _btmuGetSession(&srv, &g_btdrvIBtmUserCore, 0); // GetCore
    serviceClose(&srv);
    return rc;
}

void _btmuCleanup(void) {
    serviceClose(&g_btdrvIBtmUserCore);
}

Result btmuGetServiceSession(Service* srv_out) {
    if (hosversionBefore(5,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return smGetService(srv_out, "btm:u");
}

Service* btmuGetServiceSession_IBtmUserCore(void) {
    return &g_btdrvIBtmUserCore;
}

static Result _btmuGetSession(Service* srv, Service* srv_out, u32 cmd_id) {
    return serviceDispatch(srv, cmd_id,
        .out_num_objects = 1,
        .out_objects = srv_out,
    );
}

static Result _btmuCmdNoIO(u32 cmd_id) {
    return serviceDispatch(&g_btdrvIBtmUserCore, cmd_id);
}

static Result _btmuCmdGetEventOutFlag(Event* out_event, bool autoclear, u32 cmd_id) {
    Handle tmp_handle = INVALID_HANDLE;
    Result rc = 0;
    u8 out=0;

    rc = serviceDispatchOut(&g_btdrvIBtmUserCore, cmd_id, out,
        .out_handle_attrs = { SfOutHandleAttr_HipcCopy },
        .out_handles = &tmp_handle,
    );
    if (R_SUCCEEDED(rc) && !out) rc = MAKERESULT(Module_Libnx, LibnxError_ShouldNotHappen); // sdknso would Abort here.
    if (R_SUCCEEDED(rc)) eventLoadRemote(out_event, tmp_handle, autoclear);
    return rc;
}

static Result _btmuCmdInU32NoOut(u32 inval, u32 cmd_id) {
    return serviceDispatchIn(&g_btdrvIBtmUserCore, cmd_id, inval);
}

static Result _btmuCmdInBleAdvertisePacketParameterAruidNoOutput(BtdrvBleAdvertisePacketParameter param, u32 cmd_id) {
    const struct {
        BtdrvBleAdvertisePacketParameter param;
        u64 AppletResourceUserId;
    } in = { param, appletGetAppletResourceUserId() };

    return serviceDispatchIn(&g_btdrvIBtmUserCore, cmd_id, in,
        .in_send_pid = true,
    );
}

static Result _btmuGetBleScanResults(BtdrvBleScanResult *results, u8 count, u8 *total_out, u32 cmd_id) {
    u64 AppletResourceUserId = appletGetAppletResourceUserId();
    return serviceDispatchInOut(&g_btdrvIBtmUserCore, cmd_id, AppletResourceUserId, *total_out,
        .buffer_attrs = { SfBufferAttr_HipcMapAlias | SfBufferAttr_Out },
        .buffers = { { results, sizeof(BtdrvBleScanResult)*count } },
        .in_send_pid = true,
    );
}

static Result _btmuBlePairDevice(BtdrvBleAdvertisePacketParameter param, u32 unk, u32 cmd_id) {
    const struct {
        BtdrvBleAdvertisePacketParameter param;
        u32 unk;
    } in = { param, unk };

    return serviceDispatchIn(&g_btdrvIBtmUserCore, cmd_id, in);
}

static Result _btmuGetGattServiceData(u32 unk0, u16 unk1, void* buffer, size_t entrysize, u8 count, u8 *out, u32 cmd_id) {
    const struct {
        u16 unk1;
        u16 pad;
        u32 unk0;
        u64 AppletResourceUserId;
    } in = { unk1, 0, unk0, appletGetAppletResourceUserId() };

    return serviceDispatchInOut(&g_btdrvIBtmUserCore, cmd_id, in, *out,
        .buffer_attrs = { SfBufferAttr_HipcMapAlias | SfBufferAttr_Out },
        .buffers = { { buffer, entrysize*count } },
        .in_send_pid = true,
    );
}

static Result _btmuRegisterBleGattDataPath(const BtmuBleDataPath *path, u32 cmd_id) {
    const struct {
        BtmuBleDataPath path;
        u64 AppletResourceUserId;
    } in = { *path, appletGetAppletResourceUserId() };

    return serviceDispatchIn(&g_btdrvIBtmUserCore, cmd_id, in,
        .in_send_pid = true,
    );
}

Result btmuAcquireBleScanEvent(Event* out_event) {
    return _btmuCmdGetEventOutFlag(out_event, true, 0);
}

Result btmuGetBleScanFilterParameter(u16 unk, BtdrvBleAdvertisePacketParameter *out) {
    return serviceDispatchInOut(&g_btdrvIBtmUserCore, 1, unk, *out);
}

Result btmuGetBleScanFilterParameter2(u16 unk, BtdrvGattAttributeUuid *out) {
    return serviceDispatchInOut(&g_btdrvIBtmUserCore, 2, unk, *out);
}

Result btmuStartBleScanForGeneral(BtdrvBleAdvertisePacketParameter param) {
    return _btmuCmdInBleAdvertisePacketParameterAruidNoOutput(param, 3);
}

Result btmuStopBleScanForGeneral(void) {
    return _btmuCmdNoIO(4);
}

Result btmuGetBleScanResultsForGeneral(BtdrvBleScanResult *results, u8 count, u8 *total_out) {
    return _btmuGetBleScanResults(results, count, total_out, 5);
}

Result btmuStartBleScanForPaired(BtdrvBleAdvertisePacketParameter param) {
    return _btmuCmdInBleAdvertisePacketParameterAruidNoOutput(param, 6);
}

Result btmuStopBleScanForPaired(void) {
    return _btmuCmdNoIO(7);
}

Result btmuStartBleScanForSmartDevice(const BtdrvGattAttributeUuid *uuid) {
    const struct {
        BtdrvGattAttributeUuid uuid;
        u32 pad;
        u64 AppletResourceUserId;
    } in = { *uuid, 0, appletGetAppletResourceUserId() };

    return serviceDispatchIn(&g_btdrvIBtmUserCore, 8, in,
        .in_send_pid = true,
    );
}

Result btmuStopBleScanForSmartDevice(void) {
    return _btmuCmdNoIO(9);
}

Result btmuGetBleScanResultsForSmartDevice(BtdrvBleScanResult *results, u8 count, u8 *total_out) {
    return _btmuGetBleScanResults(results, count, total_out, 10);
}

Result btmuAcquireBleConnectionEvent(Event* out_event) {
    return _btmuCmdGetEventOutFlag(out_event, true, 17);
}

Result btmuBleConnect(BtdrvAddress addr) {
    const struct {
        BtdrvAddress addr;
        u8 pad[2];
        u64 AppletResourceUserId;
    } in = { addr, {0}, appletGetAppletResourceUserId() };

    return serviceDispatchIn(&g_btdrvIBtmUserCore, 18, in,
        .in_send_pid = true,
    );
}

Result btmuBleDisconnect(u32 unk) {
    return _btmuCmdInU32NoOut(unk, 19);
}

Result btmuBleGetConnectionState(BtdrvBleConnectionInfo *info, u8 count, u8 *total_out) {
    u64 AppletResourceUserId = appletGetAppletResourceUserId();
    return serviceDispatchInOut(&g_btdrvIBtmUserCore, 20, AppletResourceUserId, *total_out,
        .buffer_attrs = { SfBufferAttr_HipcPointer | SfBufferAttr_Out },
        .buffers = { { info, sizeof(BtdrvBleConnectionInfo)*count } },
        .in_send_pid = true,
    );
}

Result btmuAcquireBlePairingEvent(Event* out_event) {
    return _btmuCmdGetEventOutFlag(out_event, true, 21);
}

Result btmuBlePairDevice(BtdrvBleAdvertisePacketParameter param, u32 unk) {
    return _btmuBlePairDevice(param, unk, 22);
}

Result btmuBleUnPairDevice(BtdrvBleAdvertisePacketParameter param, u32 unk) {
    return _btmuBlePairDevice(param, unk, 23);
}

Result btmuBleUnPairDevice2(BtdrvAddress addr, BtdrvBleAdvertisePacketParameter param) {
    const struct {
        BtdrvAddress addr;
        BtdrvBleAdvertisePacketParameter param;
    } in = { addr, param };

    return serviceDispatchIn(&g_btdrvIBtmUserCore, 24, in);
}

Result btmuBleGetPairedDevices(BtdrvBleAdvertisePacketParameter param, BtdrvAddress *addrs, u8 count, u8 *total_out) {
    return serviceDispatchInOut(&g_btdrvIBtmUserCore, 25, param, *total_out,
        .buffer_attrs = { SfBufferAttr_HipcPointer | SfBufferAttr_Out },
        .buffers = { { addrs, sizeof(BtdrvAddress)*count } },
    );
}

Result btmuAcquireBleServiceDiscoveryEvent(Event* out_event) {
    return _btmuCmdGetEventOutFlag(out_event, true, 26);
}

Result btmuGetGattServices(u32 unk, BtmuGattService *services, u8 count, u8 *total_out) {
    const struct {
        u32 unk;
        u32 pad;
        u64 AppletResourceUserId;
    } in = { unk, 0, appletGetAppletResourceUserId() };

    return serviceDispatchInOut(&g_btdrvIBtmUserCore, 27, in, *total_out,
        .buffer_attrs = { SfBufferAttr_HipcMapAlias | SfBufferAttr_Out },
        .buffers = { { services, sizeof(BtmuGattService)*count } },
        .in_send_pid = true,
    );
}

Result btmuGetGattService(u32 unk, const BtdrvGattAttributeUuid *uuid, BtmuGattService *service, u8 *total_out) {
    const struct {
        u32 unk;
        BtdrvGattAttributeUuid uuid;
        u64 AppletResourceUserId;
    } in = { unk, *uuid, appletGetAppletResourceUserId() };

    return serviceDispatchInOut(&g_btdrvIBtmUserCore, 28, in, *total_out,
        .buffer_attrs = { SfBufferAttr_HipcPointer | SfBufferAttr_Out | SfBufferAttr_FixedSize },
        .buffers = { { service, sizeof(*service) } },
        .in_send_pid = true,
    );
}

Result btmuGetGattIncludedServices(u32 unk0, u16 unk1, BtmuGattService *services, u8 count, u8 *out) {
    return _btmuGetGattServiceData(unk0, unk1, services, sizeof(BtmuGattService), count, out, 29);
}

Result btmuGetBelongingGattService(u32 unk0, u16 unk1, BtmuGattService *service, u8 *total_out) {
    const struct {
        u16 unk1;
        u16 pad;
        u32 unk0;
        u64 AppletResourceUserId;
    } in = { unk1, 0, unk0, appletGetAppletResourceUserId() };

    return serviceDispatchInOut(&g_btdrvIBtmUserCore, 30, in, *total_out,
        .buffer_attrs = { SfBufferAttr_HipcPointer | SfBufferAttr_Out | SfBufferAttr_FixedSize },
        .buffers = { { service, sizeof(*service) } },
        .in_send_pid = true,
    );
}

Result btmuGetGattCharacteristics(u32 unk0, u16 unk1, BtmuGattCharacteristic *characteristics, u8 count, u8 *total_out) {
    return _btmuGetGattServiceData(unk0, unk1, characteristics, sizeof(BtmuGattCharacteristic), count, total_out, 31);
}

Result btmuGetGattDescriptors(u32 unk0, u16 unk1, BtmuGattDescriptor *descriptors, u8 count, u8 *total_out) {
    return _btmuGetGattServiceData(unk0, unk1, descriptors, sizeof(BtmuGattDescriptor), count, total_out, 32);
}

Result btmuAcquireBleMtuConfigEvent(Event* out_event) {
    return _btmuCmdGetEventOutFlag(out_event, true, 33);
}

Result btmuConfigureBleMtu(u32 unk, u16 mtu) {
    const struct {
        u16 mtu;
        u16 pad;
        u32 unk;
        u64 AppletResourceUserId;
    } in = { mtu, 0, unk, appletGetAppletResourceUserId() };

    return serviceDispatchIn(&g_btdrvIBtmUserCore, 34, in,
        .in_send_pid = true,
    );
}

Result btmuGetBleMtu(u32 unk, u16 *out) {
    const struct {
        u32 unk;
        u32 pad;
        u64 AppletResourceUserId;
    } in = { unk, 0, appletGetAppletResourceUserId() };

    return serviceDispatchInOut(&g_btdrvIBtmUserCore, 35, in, *out,
        .in_send_pid = true,
    );
}

Result btmuRegisterBleGattDataPath(const BtmuBleDataPath *path) {
    return _btmuRegisterBleGattDataPath(path, 36);
}

Result btmuUnregisterBleGattDataPath(const BtmuBleDataPath *path) {
    return _btmuRegisterBleGattDataPath(path, 37);
}
