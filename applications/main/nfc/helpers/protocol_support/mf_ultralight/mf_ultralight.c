#include "mf_ultralight.h"
#include "mf_ultralight_render.h"

#include <nfc/protocols/mf_ultralight/mf_ultralight_poller.h>

#include "../nfc_protocol_support_gui_common.h"
#include "../../../nfc_app_i.h"

enum {
    SubmenuIndexUnlock = SubmenuIndexCommonMax,
    SubmenuIndexUnlockByReader,
    SubmenuIndexUnlockByPassword,
};

static void nfc_protocol_support_render_info_mf_ultralight(
    const MfUltralightData* data,
    NfcProtocolFormatType format_type,
    FuriString* str) {
    nfc_render_mf_ultralight_info(data, format_type, str);
}

static NfcCustomEvent nfc_protocol_support_handle_poller_mf_ultralight(
    MfUltralightPollerEvent* event,
    NfcApp* nfc_app) {
    NfcCustomEvent custom_event = NfcCustomEventReadHandlerIgnore;

    if(event->type == MfUltralightPollerEventTypeReadSuccess) {
        custom_event = NfcCustomEventReadHandlerSuccess;
    } else if(event->type == MfUltralightPollerEventTypeAuthRequest) {
        nfc_device_set_data(
            nfc_app->nfc_device, NfcProtocolMfUltralight, nfc_poller_get_data(nfc_app->poller));
        const MfUltralightData* data =
            nfc_device_get_data(nfc_app->nfc_device, NfcProtocolMfUltralight);
        if(nfc_app->mf_ul_auth->type == MfUltralightAuthTypeXiaomii) {
            if(mf_ultralight_generate_xiaomi_pass(
                   nfc_app->mf_ul_auth,
                   data->iso14443_3a_data->uid,
                   data->iso14443_3a_data->uid_len)) {
                event->data->auth_context.skip_auth = false;
            }
        } else if(nfc_app->mf_ul_auth->type == MfUltralightAuthTypeAmiibo) {
            if(mf_ultralight_generate_amiibo_pass(
                   nfc_app->mf_ul_auth,
                   data->iso14443_3a_data->uid,
                   data->iso14443_3a_data->uid_len)) {
                event->data->auth_context.skip_auth = false;
            }
        } else if(nfc_app->mf_ul_auth->type == MfUltralightAuthTypeManual) {
            event->data->auth_context.skip_auth = false;
        } else {
            event->data->auth_context.skip_auth = true;
        }
        if(!event->data->auth_context.skip_auth) {
            event->data->auth_context.password = nfc_app->mf_ul_auth->password;
        }
    } else if(event->type == MfUltralightPollerEventTypeAuthSuccess) {
        nfc_app->mf_ul_auth->pack = event->data->auth_context.pack;
    }

    return custom_event;
}

static void nfc_protocol_support_build_scene_read_menu_mf_ultralight(NfcApp* instance) {
    Submenu* submenu = instance->submenu;

    const MfUltralightData* data =
        nfc_device_get_data(instance->nfc_device, NfcProtocolMfUltralight);

    if(!mf_ultralight_is_all_data_read(data)) {
        submenu_add_item(
            submenu,
            "Unlock",
            SubmenuIndexUnlock,
            nfc_protocol_support_common_submenu_callback,
            instance);
    }
}

static void nfc_protocol_support_build_scene_saved_menu_mf_ultralight(NfcApp* instance) {
    Submenu* submenu = instance->submenu;
    const MfUltralightData* data =
        nfc_device_get_data(instance->nfc_device, NfcProtocolMfUltralight);

    if(!mf_ultralight_is_all_data_read(data)) {
        submenu_add_item(
            submenu,
            "Unlock with Reader",
            SubmenuIndexUnlockByReader,
            nfc_protocol_support_common_submenu_callback,
            instance);

        submenu_add_item(
            submenu,
            "Unlock with Password",
            SubmenuIndexUnlockByPassword,
            nfc_protocol_support_common_submenu_callback,
            instance);
    }
}

static bool
    nfc_protocol_support_handle_scene_info_mf_ultralight(NfcApp* instance, uint32_t event) {
    if(event == GuiButtonTypeRight) {
        scene_manager_next_scene(instance->scene_manager, NfcSceneNotImplemented);
        return true;
    }

    return false;
}

static bool
    nfc_protocol_support_handle_scene_read_menu_mf_ultralight(NfcApp* instance, uint32_t event) {
    switch(event) {
    case SubmenuIndexCommonEmulate:
        scene_manager_next_scene(instance->scene_manager, NfcSceneMfUltralightEmulate);
        return true;
    case SubmenuIndexUnlock:
        scene_manager_next_scene(instance->scene_manager, NfcSceneMfUltralightUnlockMenu);
        return true;
    default:
        return false;
    }
}

static bool
    nfc_protocol_support_handle_scene_saved_menu_mf_ultralight(NfcApp* instance, uint32_t event) {
    switch(event) {
    case SubmenuIndexCommonEmulate:
        scene_manager_next_scene(instance->scene_manager, NfcSceneMfUltralightEmulate);
        return true;
    case SubmenuIndexUnlockByReader:
        scene_manager_next_scene(instance->scene_manager, NfcSceneNotImplemented);
        return true;
    case SubmenuIndexUnlockByPassword:
        scene_manager_next_scene(instance->scene_manager, NfcSceneMfUltralightUnlockMenu);
        return true;
    default:
        return false;
    }
}

const NfcProtocolSupportBase nfc_protocol_support_mf_ultralight = {
    .features = NfcProtocolFeatureMoreData | NfcProtocolFeatureEmulateFull,

    .render_info = (NfcProtocolSupportRenderData)nfc_protocol_support_render_info_mf_ultralight,

    .handle_poller =
        (NfcProtocolSupportPollerHandler)nfc_protocol_support_handle_poller_mf_ultralight,

    .build_scene_read_menu =
        (NfcProtocolSupportSceneBuilder)nfc_protocol_support_build_scene_read_menu_mf_ultralight,
    .build_scene_saved_menu =
        (NfcProtocolSupportSceneBuilder)nfc_protocol_support_build_scene_saved_menu_mf_ultralight,

    .handle_scene_info =
        (NfcProtocolSupportSceneHandler)nfc_protocol_support_handle_scene_info_mf_ultralight,
    .handle_scene_read_menu =
        (NfcProtocolSupportSceneHandler)nfc_protocol_support_handle_scene_read_menu_mf_ultralight,
    .handle_scene_saved_menu =
        (NfcProtocolSupportSceneHandler)nfc_protocol_support_handle_scene_saved_menu_mf_ultralight,
};