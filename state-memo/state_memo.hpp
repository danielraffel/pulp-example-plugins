#pragma once

// State Memo — a plugin that owns custom state separate from its parameters.
//
// It has one automatable parameter (Gain) handled by the usual StateStore path,
// plus a free-text "memo" the user can attach to a session. The memo is NOT a
// parameter (it isn't automatable); it is plugin-owned state serialized via
// serialize_plugin_state() / deserialize_plugin_state(). This demonstrates the
// SDK contract for state that lives outside the parameter system, including
// fail-safe handling of empty / corrupt / forward-version blobs.

#include <pulp/format/processor.hpp>
#include <pulp/view/view.hpp>

#include <algorithm>
#include <cstdint>
#include <memory>
#include <span>
#include <string>
#include <vector>

namespace pulp::examples::classic {

enum StateMemoParams : state::ParamID {
    kGain = 1,  // 0..2 linear
};

// Defined out-of-line in state_memo_editor.hpp (included at the bottom of this file).
// Forward-declared so the editor the screenshot tests render is the same
// tree the host receives from create_view().
std::unique_ptr<view::View> build_state_memo_editor(state::StateStore& store);

class StateMemoProcessor : public format::Processor {
public:
    // Hand the host our dark Ink & Signal editor; the framework owns the
    // returned tree and may call this once per attached editor window.
    std::unique_ptr<view::View> create_view() override { return build_state_memo_editor(state()); }

    static constexpr uint32_t kSchemaVersion = 1;
    static constexpr uint32_t kMaxMemoBytes = 4096;  // bound untrusted input

    format::PluginDescriptor descriptor() const override {
        return {.name = "StateMemo", .manufacturer = "Pulp Examples",
                .bundle_id = "com.pulp.examples.state-memo", .version = "0.1.0",
                .category = format::PluginCategory::Effect,
                .input_buses = {{"Audio In", 2}}, .output_buses = {{"Audio Out", 2}}};
    }

    void define_parameters(state::StateStore& store) override {
        store.add_parameter({.id = kGain, .name = "Gain", .unit = "",
                             .range = {0.0f, 2.0f, 1.0f, 0.0f}});
    }

    void prepare(const format::PrepareContext&) override {}

    void process(audio::BufferView<float>& output,
                 const audio::BufferView<const float>& input,
                 midi::MidiBuffer&, midi::MidiBuffer&,
                 const format::ProcessContext&) override {
        const float gain = std::clamp(state().get_value(kGain), 0.0f, 2.0f);
        const std::size_t channels =
            std::min(output.num_channels(), input.num_channels());
        for (std::size_t ch = 0; ch < channels; ++ch) {
            auto in = input.channel(ch); auto out = output.channel(ch);
            for (std::size_t i = 0; i < out.size(); ++i) out[i] = in[i] * gain;
        }
        for (std::size_t ch = channels; ch < output.num_channels(); ++ch) {
            auto out = output.channel(ch);
            for (std::size_t i = 0; i < out.size(); ++i) out[i] = 0.0f;
        }
    }

    // ---- Custom (non-parameter) plugin state -----------------------------
    void set_memo(std::string memo) {
        if (memo.size() > kMaxMemoBytes) memo.resize(kMaxMemoBytes);
        memo_ = std::move(memo);
    }
    const std::string& memo() const { return memo_; }

    // Layout: [u32 version][u32 memo_len][memo bytes][...future fields ignored...]
    std::vector<uint8_t> serialize_plugin_state() const override {
        std::vector<uint8_t> out;
        put_u32(out, kSchemaVersion);
        put_u32(out, static_cast<uint32_t>(memo_.size()));
        out.insert(out.end(), memo_.begin(), memo_.end());
        return out;
    }

    bool deserialize_plugin_state(std::span<const uint8_t> data) override {
        // Empty blob: nothing to restore, keep defaults (not an error).
        if (data.empty()) { memo_.clear(); return true; }
        // Need at least the two header words.
        if (data.size() < 8) return false;
        std::size_t pos = 0;
        const uint32_t version = get_u32(data, pos);   // forward-compatible: unused but read
        (void)version;
        const uint32_t len = get_u32(data, pos);
        // Bound the claimed length BEFORE the pos + len addition. This ordering
        // is a safety invariant: capping len first means pos + len cannot wrap
        // size_t (which on a 32-bit target a hostile len = 0xFFFFFFFF otherwise
        // could, turning the truncation check into a 4 GB over-read).
        if (len > kMaxMemoBytes) return false;
        if (pos + len > data.size()) return false;     // truncated / corrupt
        memo_.assign(reinterpret_cast<const char*>(data.data() + pos), len);
        return true;  // trailing bytes (future fields) are intentionally ignored
    }

private:
    static void put_u32(std::vector<uint8_t>& v, uint32_t x) {
        v.push_back(uint8_t(x & 0xFF)); v.push_back(uint8_t((x >> 8) & 0xFF));
        v.push_back(uint8_t((x >> 16) & 0xFF)); v.push_back(uint8_t((x >> 24) & 0xFF));
    }
    static uint32_t get_u32(std::span<const uint8_t> d, std::size_t& pos) {
        const uint32_t x = uint32_t(d[pos]) | (uint32_t(d[pos + 1]) << 8) |
                           (uint32_t(d[pos + 2]) << 16) | (uint32_t(d[pos + 3]) << 24);
        pos += 4;
        return x;
    }
    std::string memo_;
};

inline std::unique_ptr<format::Processor> create_state_memo() {
    return std::make_unique<StateMemoProcessor>();
}

} // namespace pulp::examples::classic

// Pulls in the inline definition of build_state_memo_editor (declared above) so create_view()
// links in the plugin adapter and the headless tests alike. After the class so
// the editor header sees a complete definition; its re-include is a no-op.
#include "state_memo_editor.hpp"
