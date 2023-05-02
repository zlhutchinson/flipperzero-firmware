#include "avr_isp_view_tpi_reader.h"
#include <gui/elements.h>

//#include "../helpers/avr_isp_worker_rw.h"
#include "../helpers/avr_tpi.h"

struct AvrIspReaderTpiView {
    View* view;
    //AvrIspWorkerRW* avr_isp_worker_rw;
    AvrTpi* avr_tpi;
    const char* file_path;
    const char* file_name;
    AvrIspReaderTpiViewCallback callback;
    void* context;
};

typedef struct {
    AvrIspReaderTpiViewStatus status;
    float progress_flash;
    float progress_eeprom;
} AvrIspReaderTpiViewModel;

void avr_isp_tpi_reader_update_progress(AvrIspReaderTpiView* instance) {
    UNUSED(instance);
    // with_view_model(
    //     instance->view,
    //     AvrIspReaderTpiViewModel * model,
    //     {
    //         model->progress_flash =
    //             avr_isp_worker_rw_get_progress_flash(instance->avr_isp_worker_rw);
    //         model->progress_eeprom =
    //             avr_isp_worker_rw_get_progress_eeprom(instance->avr_isp_worker_rw);
    //     },
    //     true);
}

void avr_isp_tpi_reader_view_set_callback(
    AvrIspReaderTpiView* instance,
    AvrIspReaderTpiViewCallback callback,
    void* context) {
    furi_assert(instance);
    furi_assert(callback);

    instance->callback = callback;
    instance->context = context;
}

void avr_isp_tpi_reader_set_file_path(
    AvrIspReaderTpiView* instance,
    const char* file_path,
    const char* file_name) {
    furi_assert(instance);

    instance->file_path = file_path;
    instance->file_name = file_name;
}

void avr_isp_tpi_reader_view_draw(Canvas* canvas, AvrIspReaderTpiViewModel* model) {
    canvas_clear(canvas);
    char str_buf[64] = {0};

    canvas_set_font(canvas, FontPrimary);
    switch(model->status) {
    case AvrIspReaderTpiViewStatusIDLE:
        canvas_draw_str_aligned(canvas, 64, 5, AlignCenter, AlignCenter, "Press start to dump");
        canvas_set_font(canvas, FontSecondary);
        elements_button_center(canvas, "Start");
        break;
    case AvrIspReaderTpiViewStatusReading:
        canvas_draw_str_aligned(canvas, 64, 5, AlignCenter, AlignCenter, "Reading dump");
        break;
    case AvrIspReaderTpiViewStatusVerification:
        canvas_draw_str_aligned(canvas, 64, 5, AlignCenter, AlignCenter, "Verifyng dump");
        break;

    default:
        break;
    }

    canvas_set_font(canvas, FontSecondary);
    canvas_draw_str(canvas, 0, 27, "Flash");
    snprintf(str_buf, sizeof(str_buf), "%d%%", (uint8_t)(model->progress_flash * 100));
    elements_progress_bar_with_text(canvas, 44, 17, 84, model->progress_flash, str_buf);
    canvas_draw_str(canvas, 0, 43, "EEPROM");
    snprintf(str_buf, sizeof(str_buf), "%d%%", (uint8_t)(model->progress_eeprom * 100));
    elements_progress_bar_with_text(canvas, 44, 34, 84, model->progress_eeprom, str_buf);
}

bool avr_isp_tpi_reader_view_input(InputEvent* event, void* context) {
    furi_assert(context);
    AvrIspReaderTpiView* instance = context;

    bool ret = true;
    if(event->key == InputKeyBack && event->type == InputTypeShort) {
        with_view_model(
            instance->view,
            AvrIspReaderTpiViewModel * model,
            {
                if(model->status == AvrIspReaderTpiViewStatusIDLE) {
                    if(instance->callback)
                        instance->callback(AvrIspCustomEventSceneExit, instance->context);
                    ret = false;
                }
            },
            false);
    } else if(event->key == InputKeyOk && event->type == InputTypeShort) {
        with_view_model(
            instance->view,
            AvrIspReaderTpiViewModel * model,
            {
                if(model->status == AvrIspReaderTpiViewStatusIDLE) {
                    model->status = AvrIspReaderTpiViewStatusReading;
                    // avr_isp_worker_rw_read_dump_start(
                    //     instance->avr_isp_worker_rw, instance->file_path, instance->file_name);
                }
            },
            false);
    }
    return ret;
}

// static void avr_isp_tpi_reader_callback_status(void* context, AvrIspWorkerRWStatus status) {
//     furi_assert(context);
//     AvrIspReaderTpiView* instance = context;

//     with_view_model(
//         instance->view,
//         AvrIspReaderTpiViewModel * model,
//         {
//             switch(status) {
//             case AvrIspWorkerRWStatusEndReading:
//                 model->status = AvrIspReaderTpiViewStatusVerification;
//                 // avr_isp_worker_rw_verification_start(
//                 //     instance->avr_isp_worker_rw, instance->file_path, instance->file_name);
//                 model->status = AvrIspReaderTpiViewStatusVerification;
//                 break;
//             case AvrIspWorkerRWStatusEndVerification:
//                 if(instance->callback)
//                     instance->callback(AvrIspCustomEventSceneReadingOk, instance->context);
//                 break;
//             case AvrIspWorkerRWStatusErrorVerification:
//                 if(instance->callback)
//                     instance->callback(AvrIspCustomEventSceneErrorVerification, instance->context);
//                 break;

//             default:
//                 //AvrIspWorkerRWStatusErrorReading;
//                 if(instance->callback)
//                     instance->callback(AvrIspCustomEventSceneErrorReading, instance->context);
//                 break;
//             }
//         },
//         true);
// }

void avr_isp_tpi_reader_view_enter(void* context) {
    furi_assert(context);
    AvrIspReaderTpiView* instance = context;

    with_view_model(
        instance->view,
        AvrIspReaderTpiViewModel * model,
        {
            model->status = AvrIspReaderTpiViewStatusIDLE;
            model->progress_flash = 0.0f;
            model->progress_eeprom = 0.0f;
        },
        true);

    instance->avr_tpi = avr_tpi_alloc();
    if(avr_tpi_start_pmode(instance->avr_tpi)) {
        FURI_LOG_E("TAG", "read signature");
        avr_tpi_get_signature(instance->avr_tpi);

        FURI_LOG_E("TAG", "nwm_lock_bit 0x%02X", avr_tpi_get_nwm_lock_bit(instance->avr_tpi));
        FURI_LOG_E(
            "TAG", "configuration_bit 0x%02X", avr_tpi_get_configuration_bit(instance->avr_tpi));
        FURI_LOG_E(
            "TAG", "calibration_bit 0x%02X", avr_tpi_get_calibration_bit(instance->avr_tpi));
        FURI_LOG_E("TAG", "end pmode");

        uint8_t data[32];
        avr_tpi_read_data_memory(instance->avr_tpi, 0x0040, data, 32);
        for(int i = 0; i < 32; i++) {
            FURI_LOG_E("TAG", "data[%d] 0x%02X", i, data[i]);
        }
        avr_tpi_end_pmode(instance->avr_tpi);
    }
    // //Start avr_isp_worker_rw
    // instance->avr_isp_worker_rw = avr_isp_worker_rw_alloc(instance->context);

    // avr_isp_worker_rw_set_callback_status(
    //     instance->avr_isp_worker_rw, avr_isp_tpi_reader_callback_status, instance);

    // avr_isp_worker_rw_start(instance->avr_isp_worker_rw);
}

void avr_isp_tpi_reader_view_exit(void* context) {
    furi_assert(context);

    AvrIspReaderTpiView* instance = context;
    UNUSED(instance);
    // //Stop avr_isp_worker_rw
    // if(avr_isp_worker_rw_is_running(instance->avr_isp_worker_rw)) {
    //     avr_isp_worker_rw_stop(instance->avr_isp_worker_rw);
    // }

    // avr_isp_worker_rw_free(instance->avr_isp_worker_rw);
}

AvrIspReaderTpiView* avr_isp_tpi_reader_view_alloc() {
    AvrIspReaderTpiView* instance = malloc(sizeof(AvrIspReaderTpiView));

    // View allocation and configuration
    instance->view = view_alloc();

    view_allocate_model(instance->view, ViewModelTypeLocking, sizeof(AvrIspReaderTpiViewModel));
    view_set_context(instance->view, instance);
    view_set_draw_callback(instance->view, (ViewDrawCallback)avr_isp_tpi_reader_view_draw);
    view_set_input_callback(instance->view, avr_isp_tpi_reader_view_input);
    view_set_enter_callback(instance->view, avr_isp_tpi_reader_view_enter);
    view_set_exit_callback(instance->view, avr_isp_tpi_reader_view_exit);

    return instance;
}

void avr_isp_tpi_reader_view_free(AvrIspReaderTpiView* instance) {
    furi_assert(instance);

    view_free(instance->view);
    free(instance);
}

View* avr_isp_tpi_reader_view_get_view(AvrIspReaderTpiView* instance) {
    furi_assert(instance);

    return instance->view;
}
