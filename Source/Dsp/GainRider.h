#pragma once
#include <algorithm>
#include <cmath>
#include <limits>

namespace vocalbalance {
struct Controls {
    float target = -21.0f, lower = -6.0f, upper = 6.0f;
    float vocal = 0.0f, music = 0.0f, idle = 0.0f, output = 0.0f, rider = 0.0f;
    bool fast = false, manual = false, bypass = false;
};

// Independent, causal gain riding. No heap storage, locks, I/O or lookahead.
class GainRider {
public:
    void prepare(double sampleRate) noexcept {
        rate = std::isfinite(sampleRate) ? std::clamp(sampleRate, 8000.0, 384000.0) : 48000.0;
        vocalPower = musicPower = gain = trim = 0.0;
        wet = 1.0; active = false; hold = 0;
    }
    float gainDb() const noexcept { return static_cast<float>(gain); }
    float inputDb() const noexcept { return db(vocalPower); }
    bool isActive() const noexcept { return active; }
    bool hasMusic() const noexcept { return db(musicPower) > -65.0f; }
    float outputDb() const noexcept { return peakDb; }

    void process(float* const* audio, int channels, int frames, const Controls& input,
                 const float* const* sidechain = nullptr, int sideChannels = 0) noexcept {
        if (channels <= 0 || frames <= 0) return;
        Controls c = input;
        c.target = finite(c.target, -21); c.lower = std::clamp(finite(c.lower, -6), -12.0f, 11.0f);
        c.upper = std::clamp(finite(c.upper, 6), c.lower + 1, 12.0f);
        c.vocal = std::clamp(finite(c.vocal, 0), -12.0f, 12.0f);
        c.music = std::clamp(finite(c.music, 0), -12.0f, 12.0f);
        c.output = std::clamp(finite(c.output, 0), -12.0f, 12.0f);
        c.idle = finite(c.idle, 0); c.rider = std::clamp(finite(c.rider, 0), -12.0f, 12.0f);
        const double detect = coefficient(c.fast ? 10 : 40);
        const double release = coefficient(c.fast ? 80 : 200);
        const double movement = coefficient(c.manual ? 10 : (c.fast ? 100 : 500));
        const double musicFilter = coefficient(100), smooth = coefficient(15);
        const double on = -55.0 - c.vocal;
        const double musicAmount = std::pow(10.0, c.music / 20.0);
        double peak = 0;
        for (int i = 0; i < frames; ++i) {
            double power = 0;
            for (int ch = 0; ch < channels; ++ch) {
                float& sample = audio[ch][i];
                if (!std::isfinite(sample)) sample = 0;
                const double x = sample; power += x * x;
            }
            power /= channels;
            const double follow = power > vocalPower ? detect : release;
            vocalPower = follow * vocalPower + (1 - follow) * power;
            double backing = 0;
            for (int ch = 0; ch < sideChannels; ++ch) {
                double x = sidechain[ch][i]; if (!std::isfinite(x)) x = 0;
                backing += x * x;
            }
            backing /= std::max(1, sideChannels);
            musicPower = musicFilter * musicPower + (1 - musicFilter) * backing;
            const double level = db(vocalPower);
            if (level > on) { active = true; hold = static_cast<int>(rate * 0.12); }
            else if (level < on - 4 && --hold <= 0) { active = false; hold = 0; }
            const double musicLevel = db(musicPower);
            const double offset = sideChannels > 0 && musicLevel > -65
                ? std::clamp((musicLevel + 18) * musicAmount, -12.0, 12.0) : 0;
            double wanted = active ? std::clamp(c.target + offset - level, static_cast<double>(c.lower), static_cast<double>(c.upper))
                                   : std::clamp(static_cast<double>(c.idle), static_cast<double>(c.lower), static_cast<double>(c.upper));
            if (c.manual) wanted = c.rider;
            gain = movement * gain + (1 - movement) * wanted;
            trim = smooth * trim + (1 - smooth) * c.output;
            wet = smooth * wet + (1 - smooth) * (c.bypass ? 0.0 : 1.0);
            const double multiplier = 1 + wet * (std::pow(10.0, (gain + trim) / 20.0) - 1);
            for (int ch = 0; ch < channels; ++ch) {
                const double value = static_cast<double>(audio[ch][i]) * multiplier;
                const double limit = std::numeric_limits<float>::max();
                audio[ch][i] = static_cast<float>(std::clamp(value, -limit, limit));
                peak = std::max(peak, std::abs(value));
            }
        }
        peakDb = static_cast<float>(20 * std::log10(std::max(1.0e-6, peak)));
    }
private:
    static float finite(float value, float fallback) noexcept { return std::isfinite(value) ? value : fallback; }
    static float db(double power) noexcept { return static_cast<float>(10 * std::log10(std::max(1.0e-12, power))); }
    double coefficient(double milliseconds) const noexcept { return std::exp(-1.0 / (rate * milliseconds * .001)); }
    double rate = 48000, vocalPower = 0, musicPower = 0, gain = 0, trim = 0, wet = 1;
    bool active = false;
    int hold = 0;
    float peakDb = -120;
};
}
