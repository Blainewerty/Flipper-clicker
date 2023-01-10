#include <stdio.h>
#include <furi.h>
#include <gui/gui.h>
#include <gui/elements.h>
#include <input/input.h>
#include <stdint.h>
#include <notification/notification_messages.h>

typedef enum {
    EventTypeTick,
    EventTypeInput,
} EventType;

typedef struct {
    EventType type;
    InputEvent input;
} HelloWorldEvent;

typedef struct {
    bool counter_made;
    bool show_results;
    uint16_t counter;
    uint16_t result;
    uint16_t result_pointer;
    int results[100];
} HelloWorldState;

static void draw_callback(Canvas *canvas, void *ctx) {
    furi_assert(ctx);
    HelloWorldState *app = (HelloWorldState *) ctx;

    if (app->show_results && app->result_pointer != 0) {
        int x = 5;
        int y = 28;

        for (int i = 0; i < app->result_pointer; ++i) {

            if (x > 125) {
                x = 5;
                y += 10;
            }

            char result[6];
            snprintf(result, 100, "%d", app->results[i]);

            elements_multiline_text_aligned(
                    canvas, 0, 0, AlignLeft, AlignTop, "Counter App");
            elements_multiline_text_aligned(
                    canvas, x, y, AlignCenter, AlignCenter, result);
            elements_multiline_text_aligned(
                    canvas, 125, 0, AlignRight, AlignTop, "Results");
            x += 12;
        }
    } else {
        char result[6];
        snprintf(result, 100, "%d", app->result);

        if (!app->counter_made) {
            elements_multiline_text_aligned(
                    canvas, 0, 0, AlignLeft, AlignTop, "Counter App");
            if (app->result != 0) {
                elements_multiline_text_aligned(
                        canvas, 125, 0, AlignRight, AlignTop, result);
            }
            elements_multiline_text_aligned(
                    canvas, 64, 40, AlignCenter, AlignBottom, "Press Ok or > to counter");
        } else {
            elements_multiline_text_aligned(
                    canvas, 0, 0, AlignLeft, AlignTop, "Counter App");

            char count[6];
            snprintf(count, 100, "%d", app->counter);

            elements_multiline_text_aligned(
                    canvas, 64, 28, AlignCenter, AlignCenter, count);

            elements_multiline_text_aligned(
                    canvas, 125, 0, AlignRight, AlignTop, result);

            canvas_set_font(canvas, FontSecondary);
            canvas_draw_str(canvas, 15, 60, "Long press down to drop");
        }
    }
}

static void input_callback(InputEvent *input_event, void *ctx) {
    furi_assert(ctx);
    FuriMessageQueue *event_queue = ctx;

    HelloWorldEvent event = {.type = EventTypeInput, .input = *input_event};
    furi_message_queue_put(event_queue, &event, FuriWaitForever);
}

int32_t hello_clicker_app() {
    FuriMessageQueue *event_queue = furi_message_queue_alloc(8, sizeof(HelloWorldEvent));
    HelloWorldState *state = malloc(sizeof(HelloWorldState));

    ViewPort *view_port = view_port_alloc();
    view_port_draw_callback_set(view_port, draw_callback, state);
    view_port_input_callback_set(view_port, input_callback, event_queue);

    Gui *gui = furi_record_open(RECORD_GUI);
    gui_add_view_port(gui, view_port, GuiLayerFullscreen);

    NotificationApp *notifications = furi_record_open(RECORD_NOTIFICATION);

    HelloWorldEvent event;

    while (1) {
        // Выбираем событие из очереди в переменную event (ждем бесконечно долго, если очередь пуста)
        // и проверяем, что у нас получилось это сделать
        furi_check(furi_message_queue_get(event_queue, &event, FuriWaitForever) == FuriStatusOk);

        // Наше событие — это нажатие кнопки
        if (event.type == EventTypeInput) {
            if (event.input.type == InputTypeLong && event.input.key == InputKeyDown) {
                notification_message(notifications, &sequence_blink_red_100);

                state->result = state->result + state->counter;
                state->results[state->result_pointer] = state->counter;
                state->counter = 0;
                state->result_pointer++;
                state->counter_made = false;
            }

            if (event.input.type == InputTypePress) {
                if (event.input.key == InputKeyOk || event.input.key == InputKeyRight) {
                    notification_message(notifications, &sequence_blink_blue_100);
                    state->show_results = false;
                    state->counter++;
                    state->counter_made = true;
                }
                if (event.input.key == InputKeyUp) {
                    notification_message(notifications, &sequence_blink_magenta_100);
                    state->show_results = true;
                }
                if (event.input.key == InputKeyLeft) {
                    notification_message(notifications, &sequence_blink_blue_100);
                    state->counter--;
                }
                if (event.input.key == InputKeyBack) {
                    break;
                }
            }
        }
    }

    // Специальная очистка памяти, занимаемой очередью
    furi_message_queue_free(event_queue);

    // Чистим созданные объекты, связанные с интерфейсом
    gui_remove_view_port(gui, view_port);
    view_port_free(view_port);
    furi_record_close(RECORD_GUI);

    // Очищаем нотификации
    furi_record_close(RECORD_NOTIFICATION);

    return 0;
}