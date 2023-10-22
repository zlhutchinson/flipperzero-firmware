#include "infrared_remote.h"

#include <m-array.h>

#include <toolbox/m_cstr_dup.h>
#include <toolbox/path.h>
#include <storage/storage.h>

#define TAG "InfraredRemote"

#define INFRARED_FILE_HEADER "IR signals file"
#define INFRARED_FILE_VERSION (1)

ARRAY_DEF(InfraredSignalNameArray, const char*, M_CSTR_DUP_OPLIST);

struct InfraredRemote {
    InfraredSignalNameArray_t signal_names;
    FuriString* name;
    FuriString* path;
};

InfraredRemote* infrared_remote_alloc() {
    InfraredRemote* remote = malloc(sizeof(InfraredRemote));
    InfraredSignalNameArray_init(remote->signal_names);
    remote->name = furi_string_alloc();
    remote->path = furi_string_alloc();
    return remote;
}

void infrared_remote_free(InfraredRemote* remote) {
    InfraredSignalNameArray_clear(remote->signal_names);
    furi_string_free(remote->path);
    furi_string_free(remote->name);
    free(remote);
}

void infrared_remote_reset(InfraredRemote* remote) {
    InfraredSignalNameArray_reset(remote->signal_names);
    furi_string_reset(remote->name);
    furi_string_reset(remote->path);
}

const char* infrared_remote_get_name(const InfraredRemote* remote) {
    return furi_string_get_cstr(remote->name);
}

static void infrared_remote_set_path(InfraredRemote* remote, const char* path) {
    furi_string_set(remote->path, path);
    path_extract_filename(remote->path, remote->name, true);
}

const char* infrared_remote_get_path(const InfraredRemote* remote) {
    return furi_string_get_cstr(remote->path);
}

size_t infrared_remote_get_signal_count(const InfraredRemote* remote) {
    return InfraredSignalNameArray_size(remote->signal_names);
}

InfraredRemoteButton* infrared_remote_get_button(InfraredRemote* remote, size_t index) {
    UNUSED(remote);
    UNUSED(index);
    furi_crash("infrared_remote_get_button() not implemented");
}

const char* infrared_remote_get_signal_name(const InfraredRemote* remote, size_t index) {
    furi_assert(index < infrared_remote_get_signal_count(remote));
    return *InfraredSignalNameArray_cget(remote->signal_names, index);
}

bool infrared_remote_load_signal(
    const InfraredRemote* remote,
    InfraredSignal* signal,
    size_t index) {
    furi_assert(index < infrared_remote_get_signal_count(remote));

    Storage* storage = furi_record_open(RECORD_STORAGE);
    FlipperFormat* ff = flipper_format_buffered_file_alloc(storage);

    bool success = false;

    do {
        const char* path = furi_string_get_cstr(remote->path);
        if(!flipper_format_buffered_file_open_existing(ff, path)) break;

        const char* name = infrared_remote_get_signal_name(remote, index);

        if(!infrared_signal_search_and_read(signal, ff, name)) {
            FURI_LOG_E(TAG, "Failed to load signal '%s' from file '%s'", name, path);
            break;
        }

        success = true;
    } while(false);

    flipper_format_free(ff);
    furi_record_close(RECORD_STORAGE);

    return success;
}

bool infrared_remote_find_signal_by_name(
    const InfraredRemote* remote,
    const char* name,
    size_t* index) {
    uint32_t i = 0;
    InfraredSignalNameArray_it_t it;

    for(InfraredSignalNameArray_it(it, remote->signal_names); !InfraredSignalNameArray_end_p(it);
        InfraredSignalNameArray_next(it), ++i) {
        if(strcmp(*InfraredSignalNameArray_cref(it), name) == 0) {
            *index = i;
            return true;
        }
    }

    return false;
}

bool infrared_remote_append_signal(
    InfraredRemote* remote,
    const InfraredSignal* signal,
    const char* name) {
    Storage* storage = furi_record_open(RECORD_STORAGE);
    FlipperFormat* ff = flipper_format_file_alloc(storage);

    bool success = false;
    const char* path = furi_string_get_cstr(remote->path);

    do {
        if(!flipper_format_file_open_append(ff, path)) break;
        if(!infrared_signal_save(signal, ff, name)) break;

        InfraredSignalNameArray_push_back(remote->signal_names, name);
        success = true;
    } while(false);

    flipper_format_free(ff);
    furi_record_close(RECORD_STORAGE);

    return success;
}

bool infrared_remote_rename_signal(InfraredRemote* remote, size_t index, const char* new_name) {
    UNUSED(remote);
    UNUSED(index);
    UNUSED(new_name);
    furi_crash("infrared_remote_rename_signal() not implemented");
    // furi_assert(index < InfraredButtonArray_size(remote->buttons));
    // InfraredRemoteButton* button = *InfraredButtonArray_get(remote->buttons, index);
    // infrared_remote_button_set_name(button, new_name);
    // return infrared_remote_store(remote);
}

bool infrared_remote_delete_signal(InfraredRemote* remote, size_t index) {
    const size_t signal_count = infrared_remote_get_signal_count(remote);
    furi_check(index < signal_count);

    Storage* storage = furi_record_open(RECORD_STORAGE);
    // Create input and output files
    FlipperFormat* ff_in = flipper_format_buffered_file_alloc(storage);
    FlipperFormat* ff_out = flipper_format_buffered_file_alloc(storage);

    const char* path_in = furi_string_get_cstr(remote->path);
    // TODO: Generate a random file name
    const char* path_out = "/any/infrared/temp.ir.swp";

    bool success = false;
    InfraredSignal* signal = infrared_signal_alloc();
    FuriString* signal_name = furi_string_alloc();

    do {
        if(!flipper_format_buffered_file_open_existing(ff_in, path_in)) break;
        if(!flipper_format_buffered_file_open_always(ff_out, path_out)) break;
        if(!flipper_format_write_header_cstr(ff_out, INFRARED_FILE_HEADER, INFRARED_FILE_VERSION))
            break;

        size_t i;
        for(i = 0; i < signal_count; ++i) {
            // Load signals one by one from the input file
            if(!infrared_signal_read(signal, ff_in, signal_name)) break;
            // Skip the signal that needs to be deleted ...
            if(i == index) continue;
            // ... But copy all others to the output file
            if(!infrared_signal_save(signal, ff_out, furi_string_get_cstr(signal_name))) break;
        }

        if(i != signal_count) break;

        if(!flipper_format_buffered_file_close(ff_out)) break;
        if(!flipper_format_buffered_file_close(ff_in)) break;

        const FS_Error status = storage_common_rename(storage, path_out, path_in);
        if(status != FSE_OK && status != FSE_EXIST) break;

        InfraredSignalNameArray_remove_v(remote->signal_names, index, index + 1);
        success = true;
    } while(false);

    furi_string_free(signal_name);
    infrared_signal_free(signal);

    flipper_format_free(ff_out);
    flipper_format_free(ff_in);

    furi_record_close(RECORD_STORAGE);

    return success;
}

void infrared_remote_move_signal(InfraredRemote* remote, size_t index, size_t new_index) {
    UNUSED(remote);
    UNUSED(index);
    UNUSED(new_index);
    furi_crash("infrared_remote_move_signal() not implemented");
    // furi_assert(index_orig < InfraredButtonArray_size(remote->buttons));
    // furi_assert(index_dest < InfraredButtonArray_size(remote->buttons));
    //
    // InfraredRemoteButton* button;
    // InfraredButtonArray_pop_at(&button, remote->buttons, index_orig);
    // InfraredButtonArray_push_at(remote->buttons, index_dest, button);
}

bool infrared_remote_load(InfraredRemote* remote, const char* path) {
    FURI_LOG_I(TAG, "Loading file: '%s'", path);

    Storage* storage = furi_record_open(RECORD_STORAGE);
    FlipperFormat* ff = flipper_format_buffered_file_alloc(storage);

    FuriString* tmp = furi_string_alloc();
    bool success = false;

    do {
        if(!flipper_format_buffered_file_open_existing(ff, path)) break;

        uint32_t version;
        if(!flipper_format_read_header(ff, tmp, &version)) break;

        if(!furi_string_equal(tmp, INFRARED_FILE_HEADER) || (version != INFRARED_FILE_VERSION))
            break;

        infrared_remote_set_path(remote, path);
        InfraredSignalNameArray_reset(remote->signal_names);

        while(infrared_signal_read_name(ff, tmp)) {
            InfraredSignalNameArray_push_back(remote->signal_names, furi_string_get_cstr(tmp));
        }

        success = true;
    } while(false);

    furi_string_free(tmp);
    flipper_format_free(ff);
    furi_record_close(RECORD_STORAGE);

    return success;
}

bool infrared_remote_rename(InfraredRemote* remote, const char* new_path) {
    const char* old_path = infrared_remote_get_path(remote);

    Storage* storage = furi_record_open(RECORD_STORAGE);
    const FS_Error status = storage_common_rename(storage, old_path, new_path);
    furi_record_close(RECORD_STORAGE);

    const bool success = (status == FSE_OK || status == FSE_EXIST);

    if(success) {
        infrared_remote_set_path(remote, new_path);
    }

    return success;
}

bool infrared_remote_remove(InfraredRemote* remote) {
    Storage* storage = furi_record_open(RECORD_STORAGE);
    const FS_Error status = storage_common_remove(storage, infrared_remote_get_path(remote));
    furi_record_close(RECORD_STORAGE);

    const bool success = (status == FSE_OK || status == FSE_NOT_EXIST);

    if(success) {
        infrared_remote_reset(remote);
    }

    return success;
}
