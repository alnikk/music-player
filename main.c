#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <errno.h>
#include <string.h>
#include <stdbool.h>

#include <gst/gst.h>

#define CHECK_INPUT_MS 25

#define MAX_FILENAME 255

void usage(char prog_name[]) {
    printf("Usage:\n");
    printf("\t%s <dir path>\n", prog_name);
}

static DIR *current_dir;
static struct dirent *current_dir_entry;

static int open_dir(char top_dir[]) {
    current_dir = opendir(top_dir);
    if (!current_dir) {
        return -1;
    }
    return 0;
}

static void close_current_dir(void) {
    closedir(current_dir);
}

static int read_next(char* filename) {
    struct dirent* dir = readdir(current_dir);
    if (dir == NULL) {
        return -EAGAIN;
    }

    if (strcmp(dir->d_name, ".") == 0 && strcmp(dir->d_name, "..") == 0) {
        return -EDOTDOT;
    }

    strncpy(filename, dir->d_name, MAX_FILENAME);

    if (dir->d_type == DT_DIR) {
        return -EISDIR;
    }

    return 0;
}

static int read_next_file(char* filename) {
    int ret = 0;
    do {
        ret = read_next(filename);
        switch (ret) {
            case -EAGAIN:
                //printf("ended\n");
                return -1;
                break;
            case -EDOTDOT:
            case -EISDIR:
                //printf("is dir %s\n", filename);
                break;
        }
    } while(ret < 0);

    return 0;
}

static GstParseContext* gst_parse_context;
static GMainLoop* loop;
static int init_gst(void) {
    gst_init(0, NULL);
    gst_parse_context = gst_parse_context_new();
    loop = g_main_loop_new(NULL, FALSE);
    return 0;
}

static void deinit_gst(void) {
    g_main_loop_unref(loop);
    gst_parse_context_free(gst_parse_context);
    gst_deinit();
}

static gboolean handle_message(GstBus *bus, GstMessage *msg, void* data) {
    GError *err;
    gchar *debug_info;

    switch (GST_MESSAGE_TYPE(msg)) {
      case GST_MESSAGE_EOS:
        printf("eos.\n");
      case GST_MESSAGE_ERROR:
        g_main_loop_quit (loop);
        break;
    }

    /* We want to keep receiving messages */
    return TRUE;
}

static int play_file(char* filename, GSourceFunc controlFunc) {
    char pipeline[MAX_FILENAME*2];
    GstElement* elPipeline;
    GError *err = NULL;
    GstState state;
    GstBus *bus;

    snprintf(pipeline, sizeof(pipeline), "filesrc location=%s ! fakesink", filename);
    elPipeline = gst_parse_launch(pipeline, &err);
    if (err) {
        printf("cannot parse the gstreamer pipeline: %s\n", err->message);
        goto _out_parsing;
    }

    bus = gst_element_get_bus(elPipeline);
    gst_bus_add_watch(bus, (GstBusFunc)handle_message, NULL);

    gst_element_set_state(elPipeline, GST_STATE_PLAYING);
    /* lets check it gets to PLAYING */
    if (gst_element_get_state(elPipeline, &state, NULL,
            GST_CLOCK_TIME_NONE) == GST_STATE_CHANGE_FAILURE ||
        state != GST_STATE_PLAYING) {
        printf("change to playing failed\n");
        goto _out_parsing;
    }

    g_timeout_add(CHECK_INPUT_MS, controlFunc, loop);
    g_main_loop_run(loop);

    gst_element_set_state(elPipeline, GST_STATE_NULL);

_out_bus:
    gst_object_unref(bus);
_out_parsing:
    gst_object_unref(elPipeline);
    return 0;
}

#define GPIO_STATS 1

const char gpio_export_path[] = "/sys/class/gpio/export";
const char gpio_direction_path[] = "/sys/class/gpio/%s/direction";
const char gpio_value_path[] = "/sys/class/gpio/%s/value";

enum gpio_direction {
    GPIO_IN = 0,
    GPIO_OUT,
};

enum gpio_value {
    GPIO_0 = 0,
    GPIO_1,
};

const char* gpio_direction_mapping[] = {
    [GPIO_IN] = "in",
    [GPIO_OUT] = "out",
};

const char* gpio_value_mapping[] = {
    [GPIO_0] = "0",
    [GPIO_1] = "1",
};

struct gpio_binding {
    int gpio_num; // act as identifier
    bool inited;
    enum gpio_direction direction;
#if GPIO_STATS == 1
    // last_wrote_value
    // last read_value
    // open at time
    // time read/write value
#endif
};

#define MAX_GPIO 40
static int gpio_inited = 0;
static struct gpio_binding gpio_array[MAX_GPIO];

static int gpio_open_and_write(const char *path, const char *val) {
    // open
    // write
    return 0;
}

static int gpio_open_and_read(const char *path, const char *val) {
    // open
    // read
    return 0;
}

static int gpio_format_path(int gpio, char *path, char *result) {
    // snprintf
    return 0;
}

static int gpio_get(int gpio, struct gpio_binding* full_gpio_object) {
    for (int i = 0; i < gpio_inited; ++i) {
        if (gpio_array[i].inited == TRUE && gpio_array[i].gpio_num == gpio) {
            full_gpio_object = &gpio_array[i];
            return 0;
        }
    }
    full_gpio_object = NULL;
    return -EINVAL;
}

static int init_gpio(int gpio, struct gpio_binding* full_gpio_object) {
    int ret;

    if (gpio < 0)
        return -EINVAL;
    if (gpio_inited > MAX_GPIO)
        return -EOVERFLOW;

    ret = gpio_get(gpio, full_gpio_object);
    if (ret == 0) {
        return -EEXIST;
    }

    // open(path)
    // write(gpio)
    // check folder exists
    // if not return -ENXIO
    gpio_inited++;
    // insert it
    // return it in full_gpio_object

#if GPIO_STATS == 1
    // insert time open
#endif
    return 0;
}

static int close_gpio(int gpio) {
    // find gpio i in array
    // unexport it
    // and move all remaining object to override this one
    gpio_inited--;
    return 0;
}

static int gpio_direction(int gpio, enum gpio_direction dir) {
    // get gpio
    // write gpio_direction[dir] in gpio_direction_path
    return 0;
}

static int gpio_value(int gpio, enum gpio_value val) {
    // get gpio
    // if direction is GPIO_IN return -EBUSY
    // write gpio_value_mapping[val] in gpio_value_path
#if GPIO_STATS == 1
    // insert val write and time
#endif

    return 0;
}

static int gpio_read_value(int gpio, enum gpio_value* val) {
    // get gpio
    // read gpio_value_mapping[val] in gpio_value_path
#if GPIO_STATS == 1
    // insert val read and time
#endif
    return 0;
}

// TODO better version with select/poll

static gboolean checkInput(gpointer data) {
//  check button pushed
//     g_main_loop_quit( (GMainLoop*)data );
//     return FALSE;
//  otherwise
    return TRUE;
}

#define GPIO_NEXT 1
#define GPIO_PLAY_PAUSE 2
static int init_player_gpio(void) {
    // open GPIO_NEXT
    // open GPIO_PLAY_PAUSE
    return 0;
}

static void close_player_gpio(void) {
    // close GPIO_NEXT
    // close GPIO_PLAY_PAUSE
}

int main(int argc, char* argv[]) {
    int ret = 0;

    if (argc != 2) {
        usage(argv[0]);
        goto _out;
    }

    ret = open_dir(argv[1]);
    if (ret < 0) {
        printf("cannot open %s\n", argv[1]);
        goto _out;
    }

    ret = init_gst();
    if (ret < 0) {
        printf("cannot init gst\n");
        goto _closedir;
    }

    ret = init_player_gpio();
    if (ret < 0) {
        printf("cannot init gpio");
        goto _gpioclose;
    }

    do {
        char filename[MAX_FILENAME];
        char full_path[MAX_FILENAME*2];
        ret = read_next_file(filename);
        if (ret < 0) {
            printf("cannot read next file\n");
            goto _closedir;
        }

        snprintf(full_path, MAX_FILENAME*2, "%s/%s", argv[1], filename);
        ret = play_file(full_path, checkInput);
        if (ret < 0) {
            printf("cannot play file %s\n", filename);
            goto _gpioclose;
        }
    } while (ret >= 0);

_gpioclose:
    close_player_gpio();
_gstclose:
    deinit_gst();
_closedir:
    close_current_dir();
_out:
    return ret;
}
