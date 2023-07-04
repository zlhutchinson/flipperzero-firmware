#pragma once

#include "nfc_protocol_support_common.h"

// Poller handler
NfcCustomEvent nfc_protocol_support_handle_poller(NfcGenericEvent event, void* context);

// Listener handler
// TODO

// Scene builders
void nfc_protocol_support_build_scene_info(NfcApp* instance);

void nfc_protocol_support_build_scene_read_menu(NfcApp* instance);

void nfc_protocol_support_build_scene_read_success(NfcApp* instance);

void nfc_protocol_support_build_scene_saved_menu(NfcApp* instance);

// Scene handlers
bool nfc_protocol_support_handle_scene_info(NfcApp* instance, uint32_t event);

bool nfc_protocol_support_handle_scene_read_menu(NfcApp* instance, uint32_t event);

bool nfc_protocol_support_handle_scene_read_success(NfcApp* instance, uint32_t event);

bool nfc_protocol_support_handle_scene_saved_menu(NfcApp* instance, uint32_t event);