#ifndef wpe_tvcontrol_backend_h
#define wpe_tvcontrol_backend_h

#ifdef __cplusplus
extern "C" {
#endif

#ifndef __GBM__
#define __GBM__
#endif

#include <stdint.h>

typedef enum { Added, Removed } tuner_changed_operation;
typedef enum { Cleared, Scanned, Completed, Stopped } scanning_state;

struct wpe_tvcontrol_string {
    char*    data;
    uint64_t length;
};

struct wpe_tvcontrol_string_vector {
    struct wpe_tvcontrol_string* strings;
    uint64_t                     length;
};

typedef enum {TUNER_CHANGED, SOURCE_CHANGED, CHANNEL_CHANGED, SCANNING_CHANGED} tvcontrol_events;

struct wpe_tvcontrol_event {
    tvcontrol_events            eventID;
    tuner_changed_operation     operation;
    scanning_state              state;
    struct wpe_tvcontrol_string tuner_id;
    struct wpe_tvcontrol_string source_id;
    struct wpe_tvcontrol_string channel_id;
};

struct wpe_tvcontrol_backend_manager_event_client {
    void (*handle_tuner_event)(void*, struct wpe_tvcontrol_event*);
    void (*handle_source_changed_event)(void*, struct wpe_tvcontrol_event*);
    void (*handle_channel_changed_event)(void*, struct wpe_tvcontrol_event*);
    void (*handle_scanning_state_changed_event)(void*, struct wpe_tvcontrol_event*);
};

typedef enum { DvbT, DvbT2, DvbC, DvbC2, DvbS, DvbS2, DvbH, DvbSh, Atsc, AtscMH, IsdbT, IsdbTb, IsdbS, IsdbC, _1seg, Dtmb, Cmmb, TDmb, SDmb, Undifined } SourceType;
struct wpe_tvcontrol_src_types_vector {
    SourceType* types;
    uint64_t    length;
};

typedef enum { Tv, Radio, Data } ChannelType;
struct wpe_tvcontrol_channel {
    char*       networkId;
    char*       transportSId;
    char*       serviceId;
    char*       name;
    uint64_t    number;
    ChannelType type;
};

struct wpe_tvcontrol_channel_vector {
    struct wpe_tvcontrol_channel* channels;
    uint64_t                      length;
};

struct wpe_tvcontrol_backend_interface {
    void* (*create)(struct  wpe_tvcontrol_backend*);
    void  (*destroy)(void*);
    void  (*get_tuner_list)(void*, struct wpe_tvcontrol_string_vector*);
    void  (*get_supported_source_types_list)(void*, const char*, struct wpe_tvcontrol_src_types_vector*);
    void  (*get_source_list)(void*, const char*, struct wpe_tvcontrol_src_types_vector*);
    void  (*get_signal_strength)(void*, const char*, double*);
    void  (*start_scanning)(void*, const char*, SourceType);
    void  (*stop_scanning)(void*, const char*);
    void  (*set_current_channel)(void*, const char*, SourceType, uint64_t);
    void  (*get_channel_list)(void*, const char*, SourceType, struct wpe_tvcontrol_channel_vector*);
    void  (*set_current_source)(void*, const char*, SourceType);
};

struct wpe_tvcontrol_backend*
wpe_tvcontrol_backend_create();

void
wpe_tvcontrol_backend_destroy(struct wpe_tvcontrol_backend*);

void
wpe_tvcontrol_backend_set_manager_event_client(struct wpe_tvcontrol_backend*, struct wpe_tvcontrol_backend_manager_event_client*, void*);

void
wpe_tvcontrol_backend_dispatch_tuner_event(struct wpe_tvcontrol_backend*, struct wpe_tvcontrol_event*);

void
wpe_tvcontrol_backend_dispatch_source_event(struct wpe_tvcontrol_backend*, struct wpe_tvcontrol_event*);

void
wpe_tvcontrol_backend_dispatch_channel_event(struct wpe_tvcontrol_backend*, struct wpe_tvcontrol_event*);

void
wpe_tvcontrol_backend_dispatch_scanning_state_event(struct wpe_tvcontrol_backend*, struct wpe_tvcontrol_event*);

void
wpe_tvcontrol_backend_get_tuner_list(struct wpe_tvcontrol_backend* backend, struct wpe_tvcontrol_string_vector* out_tuner_list);

void
wpe_tvcontrol_backend_get_supported_source_types_list(struct wpe_tvcontrol_backend* backend, const char* tuner_id, struct wpe_tvcontrol_src_types_vector* out_source_types_list);

void
wpe_tvcontrol_backend_get_source_list(struct wpe_tvcontrol_backend* backend, const char* tuner_id, struct wpe_tvcontrol_src_types_vector* out_source_list);

void
wpe_tvcontrol_backend_set_current_source(struct wpe_tvcontrol_backend* backend, const char* tuner_id, SourceType sType);

void
wpe_tvcontrol_backend_get_signal_strength(struct wpe_tvcontrol_backend* backend, const char* tuner_id, double* out_signal_strength);

void
wpe_tvcontrol_backend_start_scanning(struct wpe_tvcontrol_backend*, const char* tuner_id, SourceType type);

void
wpe_tvcontrol_backend_stop_scanning(struct wpe_tvcontrol_backend*, const char* tuner_id);

void
wpe_tvcontrol_backend_set_current_channel(struct wpe_tvcontrol_backend*, const char* tuner_id, SourceType type, uint64_t channel_number);

void
wpe_tvcontrol_backend_get_channel_list(struct wpe_tvcontrol_backend*, const char* tuner_id, SourceType type, struct wpe_tvcontrol_channel_vector* out_channel_list);

#ifdef __cplusplus
}
#endif

#endif // wpe_tvcontrol_backend_h
