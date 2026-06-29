#include <catch2/catch_test_macros.hpp>

#include "state_memo.hpp"

#include <pulp/format/headless.hpp>

#include <cstdint>
#include <span>
#include <string>
#include <vector>

using namespace pulp;
using pulp::examples::classic::create_state_memo;
using pulp::examples::classic::StateMemoProcessor;
using pulp::examples::classic::kGain;

namespace {
std::span<const uint8_t> as_span(const std::vector<uint8_t>& v) {
    return std::span<const uint8_t>(v.data(), v.size());
}
}

TEST_CASE("StateMemo round-trips the memo through serialize/deserialize", "[state-memo]") {
    StateMemoProcessor a;
    a.set_memo("Take 3 - warmer, pull 2 dB at 400 Hz");
    const auto blob = a.serialize_plugin_state();

    StateMemoProcessor b;
    REQUIRE(b.deserialize_plugin_state(as_span(blob)));
    REQUIRE(b.memo() == a.memo());
}

TEST_CASE("StateMemo handles empty state safely (keeps defaults)", "[state-memo]") {
    StateMemoProcessor p;
    p.set_memo("scratch");
    std::vector<uint8_t> empty;
    REQUIRE(p.deserialize_plugin_state(as_span(empty)));  // not an error
    REQUIRE(p.memo().empty());                            // reset to default
}

TEST_CASE("StateMemo rejects corrupt / truncated state without clobbering", "[state-memo]") {
    StateMemoProcessor p;
    p.set_memo("keep me");

    // Too short to even hold the header.
    std::vector<uint8_t> short_blob{0x01, 0x02, 0x03};
    REQUIRE_FALSE(p.deserialize_plugin_state(as_span(short_blob)));
    REQUIRE(p.memo() == "keep me");                       // unchanged on failure

    // Header claims a 1000-byte memo but the buffer is far shorter.
    std::vector<uint8_t> truncated{0x01, 0, 0, 0,  0xE8, 0x03, 0, 0,  'h', 'i'};
    REQUIRE_FALSE(p.deserialize_plugin_state(as_span(truncated)));
    REQUIRE(p.memo() == "keep me");
}

TEST_CASE("StateMemo reads a forward-version blob and ignores trailing fields", "[state-memo]") {
    // version=2, memo_len=5, "hello", then extra future bytes the v1 reader skips.
    std::vector<uint8_t> blob{
        0x02, 0, 0, 0,            // version 2 (future)
        0x05, 0, 0, 0,            // memo_len 5
        'h', 'e', 'l', 'l', 'o',  // memo
        0xAA, 0xBB, 0xCC,         // trailing future fields
    };
    StateMemoProcessor p;
    REQUIRE(p.deserialize_plugin_state(as_span(blob)));
    REQUIRE(p.memo() == "hello");
}

TEST_CASE("StateMemo rejects an over-cap memo length (DoS bound)", "[state-memo]") {
    StateMemoProcessor p;
    p.set_memo("keep me");
    // Buffer is genuinely large enough to hold the claimed memo, so only the
    // cap (not the truncation guard) can reject it — isolates the cap check.
    const uint32_t len = StateMemoProcessor::kMaxMemoBytes + 904;  // 5000 > 4096
    std::vector<uint8_t> blob(8 + len, 0);
    blob[0] = 0x01;                                          // version 1
    blob[4] = uint8_t(len); blob[5] = uint8_t(len >> 8);
    REQUIRE_FALSE(p.deserialize_plugin_state(as_span(blob)));
    REQUIRE(p.memo() == "keep me");                          // unchanged on failure
}

TEST_CASE("StateMemo accepts a memo exactly at the cap", "[state-memo]") {
    const uint32_t len = StateMemoProcessor::kMaxMemoBytes;  // boundary: > vs >=
    std::vector<uint8_t> blob(8 + len, uint8_t('x'));
    blob[0] = 0x01;
    blob[4] = uint8_t(len); blob[5] = uint8_t(len >> 8);
    blob[6] = uint8_t(len >> 16); blob[7] = uint8_t(len >> 24);
    StateMemoProcessor p;
    REQUIRE(p.deserialize_plugin_state(as_span(blob)));
    REQUIRE(p.memo().size() == len);
}

TEST_CASE("StateMemo round-trips a binary memo with embedded NULs", "[state-memo]") {
    StateMemoProcessor a;
    a.set_memo(std::string{'a', '\0', 'b', '\0', 'c'});      // length-based, not NUL-terminated
    const auto blob = a.serialize_plugin_state();
    StateMemoProcessor b;
    REQUIRE(b.deserialize_plugin_state(as_span(blob)));
    REQUIRE(b.memo() == a.memo());
    REQUIRE(b.memo().size() == 5);
}

TEST_CASE("StateMemo round-trips an empty memo (8-byte blob)", "[state-memo]") {
    StateMemoProcessor a;                                     // default memo is empty
    const auto blob = a.serialize_plugin_state();
    REQUIRE(blob.size() == 8);                                // [version][len=0], no payload
    StateMemoProcessor b;
    b.set_memo("scratch");
    REQUIRE(b.deserialize_plugin_state(as_span(blob)));
    REQUIRE(b.memo().empty());
}

TEST_CASE("StateMemo clamps an over-long memo on set", "[state-memo]") {
    StateMemoProcessor p;
    p.set_memo(std::string(StateMemoProcessor::kMaxMemoBytes + 100, 'z'));
    REQUIRE(p.memo().size() == StateMemoProcessor::kMaxMemoBytes);
}

// NOTE: an end-to-end check that the memo survives host save_state()/load_state()
// needs HeadlessHost::processor_as<T>() to read it back; that accessor is landing
// separately. The parameter path through the host is covered below.
TEST_CASE("StateMemo gain parameter round-trips through host save/load", "[state-memo]") {
    format::HeadlessHost host(create_state_memo);
    host.prepare(48000.0, 64);
    host.state().set_value(kGain, 1.5f);
    const auto saved = host.save_state();

    host.state().set_value(kGain, 0.25f);     // perturb
    REQUIRE(host.load_state(std::span<const uint8_t>(saved.data(), saved.size())));
    REQUIRE(host.state().get_value(kGain) == 1.5f);
}
