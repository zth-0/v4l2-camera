#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

#include "v4l2_camera_impl.h"
#include "v4l2_camera.h"
#include "debug.h"

#define CAMERA_DEVICE "/dev/video0"
#define SLEEP_IN_SECOND 10

vcam_impl_t *impl;
vcam_t *vcam;

pthread_mutex_t mux;
pthread_cond_t cv;
pthread_t th;

void init() {
    log_info("Creating vcam_impl");
    impl = (vcam_impl_t*)malloc(sizeof(vcam_impl_t));
    assert(impl != NULL);

    impl->path = CAMERA_DEVICE;
    impl->n_frames = DEFAULT_FRAME_COUNT;
    impl->frames = (frame_buffer*)malloc(impl->n_frames * sizeof(frame_buffer));
    assert(impl->frames != NULL);

    for (int i = 0; i < impl->n_frames; i++) {
        impl->frames[i].data = NULL;
        impl->frames[i].length = 0;
        impl->frames[i].byteused = 0;
    }

    log_info("Creating vcam");

    vcam = (vcam_t*)malloc(sizeof(vcam_t));
    assert(vcam != NULL);

    vcam->impl = impl;
    vcam->stream_status = OFF;

    vcam->_mux = mux;
    assert(pthread_mutex_init(&vcam->_mux, NULL) != -1);

    vcam->_scv = cv;
    assert(pthread_cond_init(&vcam->_scv, NULL) != -1);

    vcam->_sth = th;
    vcam->_out = NULL;
}

void cleanup() {
    log_info("Cleaning up.");
    vcam_destroy(vcam);
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        log_err("Need to provide output file for captured frames.");
        return 1;
    }

    FILE *vid_file = fopen(argv[1], "w");
    init();
    atexit(cleanup);

    log_info("calling: vcam_init_stream.");
    vcam_init_stream(vcam, NULL);
    log_info("sleeping for %d second(s)...", SLEEP_IN_SECOND);
    
    log_info("calling: vcam_start_stream.");
    vcam_start_stream(vcam, vid_file);

    log_info("sleeping for %d second(s)...", SLEEP_IN_SECOND);
    sleep(SLEEP_IN_SECOND);
    log_info("calling: vcam_stop_stream.");
    vcam_stop_stream(vcam);

    log_info("closing video file: %s", argv[1]);
    fclose(vid_file);

    return 0;
}