#include <stddef.h>
#include <stdint.h>
#include "src/iso14229.h"

// LPM / protobuf
#include <libprotobuf_mutator/src/libfuzzer/libfuzzer_macro.h>
#include "fuzz/uds_input.pb.h"

extern "C" {
uint32_t UDSMillis();
}

static uint64_t g_time_now_us = 0;
uint32_t UDSMillis() { return g_time_now_us / 1000; }

static void poll_server(UDSServer_t *srv, size_t wait_ms, size_t poll_rate_us) {
    for (size_t time_us = 0; time_us < wait_ms * 1000; time_us += poll_rate_us) {
        g_time_now_us += poll_rate_us;
        UDSServerPoll(srv);
    }
}

static UDSErr_t fn(UDSServer_t *srv, UDSEvent_t ev, void *arg) {
    udsfuzz::Testcase *tc = (udsfuzz::Testcase *)srv->fn_data;

    // For structure-aware fuzzing, we want deterministic behavior
    // Most handlers will accept requests with positive responses
    switch (ev) {
    case UDS_EVT_DiagSessCtrl: {
        const auto &dsc = tc->msgs().diag_sess_ctrl();
        UDSDiagSessCtrlArgs_t *r = (UDSDiagSessCtrlArgs_t *)arg;
        if (!dsc.has_response()) {
            r->p2_ms = 50;
            r->p2_star_ms = 5000;
        } else {
            r->p2_ms = (uint16_t)(dsc.response().p2_ms() & 0xFFFF);
            r->p2_star_ms = (uint16_t)(dsc.response().p2_star_ms() & 0xFFFF);
        }
        return UDS_PositiveResponse;
    }
    default:
        return UDS_PositiveResponse;
    }
}

// Constrain 0x10 request fields
static protobuf_mutator::libfuzzer::PostProcessorRegistration<udsfuzz::Request0x10DiagSessCtrl>
    reg_req10 = {[](udsfuzz::Request0x10DiagSessCtrl *m, unsigned int seed) {}};

// Keep UdsMessage meta sane (addresses, enums in range, etc.)
static protobuf_mutator::libfuzzer::PostProcessorRegistration<udsfuzz::Messages> reg_msg = {
    [](udsfuzz::Messages *m, unsigned int seed) {}};

bool is_valid_input(const udsfuzz::Testcase &tc) {
    if (!tc.has_msgs()) {
        return false;
    }
    if (tc.msgs().payload_case() == udsfuzz::Messages::PAYLOAD_NOT_SET) {
        return false;
    }

    switch (tc.msgs().payload_case()) {
    case udsfuzz::Messages::kDiagSessCtrl: {
        const auto &dsc = tc.msgs().diag_sess_ctrl();
        if (!dsc.has_request()) {
            return false;
        }
        if (!dsc.has_response()) {
            return false;
        }
        break;
    }
    default:
        break;
    }

    return true;
}

DEFINE_PROTO_FUZZER(const udsfuzz::Testcase &tc) {
    // Reject test cases without request and response set
    if (!is_valid_input(tc)) {
        return;
    }
    UDSServer_t srv;
    UDSTp_t *mock_client = nullptr;

    // Use addressing from testcase, with fallback defaults
    uint16_t server_sa = tc.ta() ? tc.ta() : 0x7E8;
    uint16_t server_ta = tc.sa() ? tc.sa() : 0x7E0;
    uint16_t client_sa = tc.sa() ? tc.sa() : 0x7E0;
    uint16_t client_ta = tc.ta() ? tc.ta() : 0x7E8;

    ISOTPMockArgs_t server_args = {
        .sa_phys = server_sa, .ta_phys = server_ta, .sa_func = 0x7DF, .ta_func = UDS_TP_NOOP_ADDR};
    ISOTPMockArgs_t client_args = {
        .sa_phys = client_sa, .ta_phys = client_ta, .sa_func = UDS_TP_NOOP_ADDR, .ta_func = 0x7DF};

    UDSServerInit(&srv);
    srv.fn = fn;
    srv.fn_data = (void *)&tc;
    srv.tp = ISOTPMockNew("server", &server_args);
    mock_client = ISOTPMockNew("client", &client_args);

    g_time_now_us = tc.start_time_us();

    uint8_t recv_buf[UDS_TP_MTU];

    // pre-poll
    poll_server(&srv, tc.pre_wait_ms(), 1000);

    // build SDU meta
    UDSSDU_t sdu{};
    sdu.A_Mtype = (UDS_A_Mtype_t)tc.mtype();
    sdu.A_SA = tc.sa();
    sdu.A_TA = tc.ta();
    sdu.A_TA_Type = (UDS_A_TA_Type_t)tc.ta_type();
    sdu.A_AE = 0;

    // construct frame based on payload type
    std::vector<uint8_t> frame;

    switch (tc.msgs().payload_case()) {
    case udsfuzz::Messages::kDiagSessCtrl: {
        // 0x10 Diagnostic Session Control
        const auto &dsc = tc.msgs().diag_sess_ctrl();
        const auto &req = dsc.request();
        frame.push_back((uint8_t)(req.srv() & 0xFF));      // SID
        frame.push_back((uint8_t)(req.sub_func() & 0xFF)); // subfunction
        break;
    }
    case udsfuzz::Messages::PAYLOAD_NOT_SET:
    default:
        // No payload, skip sending
        break;
    }

    if (!frame.empty()) {
        UDSTpSend(mock_client, frame.data(), frame.size(), &sdu);
    }

    // post-poll + drain
    poll_server(&srv, tc.post_wait_ms(), 1000);
    (void)UDSTpRecv(mock_client, recv_buf, sizeof(recv_buf), nullptr);

    ISOTPMockFree(mock_client);
    ISOTPMockFree(srv.tp);
}
