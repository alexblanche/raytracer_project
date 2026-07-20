#pragma once

struct program_parameters {
    enum class mode {
        Interactive, Offline
    };
    mode p_mode;
    unsigned int target_number_of_rays;
};

struct sampling_parameters {
    enum class mode {
        UniSample, MultiSample
    };
    mode s_mode;
    unsigned int multisample_number_of_samples;
};

struct tone_mapping_parameters {
    enum class mode {
        Disabled, Gamma, Reinhardt
    };
    mode tm_mode;
    float gamma_value;
};

enum class time_mode {
    Disabled, Simple, Full
};

enum class russian_roulette_mode {
    Disabled, Enabled
};

struct runtime_debugger {
    enum class option {
        Disabled, Enabled
    };

    const option option_;
    int x, y;
};

struct runtime_parameters {
    unsigned int             number_of_bounces      = 2;
    program_parameters       program                = { program_parameters::mode::Interactive,   0    };
    sampling_parameters      sampling               = { sampling_parameters::mode::UniSample,    1    };
    tone_mapping_parameters  tone_mapping           = { tone_mapping_parameters::mode::Disabled, 1.0f };
    time_mode                time                   = time_mode::Disabled;
    russian_roulette_mode    russian_roulette       = russian_roulette_mode::Disabled;
    runtime_debugger::option debug                  = runtime_debugger::option::Disabled;
};