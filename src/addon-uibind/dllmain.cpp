// SPDX-FileCopyrightText: 2018 seri14
// SPDX-License-Identifier: BSD-3-Clause

#include "dllmain.hpp"
#include "std_string_ext.hpp"

#include <list>
#include <string>

struct __declspec(uuid("90209d18-7b48-40be-b2ff-9d7f894a9428")) uibind_context
{
    std::list<std::tuple<std::string, std::string, std::string>> replaces;
};

static void on_init(reshade::api::effect_runtime *runtime)
{
    runtime->create_private_data<uibind_context>();
}
static void on_destroy(reshade::api::effect_runtime *runtime)
{
    runtime->destroy_private_data<uibind_context>();
}

static void on_reshade_present(reshade::api::effect_runtime *runtime)
{
    if (!runtime->has_effects_loaded())
        return;

    uibind_context &ctx = runtime->get_private_data<uibind_context>();

    std::list<std::tuple<std::string, std::string, std::string>> replaces = ctx.replaces;
    ctx.replaces.clear();

    for (const auto &replace : replaces)
        runtime->set_preprocessor_definition(std::get<0>(replace).c_str(), std::get<1>(replace).c_str(), std::get<2>(replace).c_str());
}

static bool on_reshade_set_uniform_value(reshade::api::effect_runtime *runtime, reshade::api::effect_uniform_variable variable, const void *value, size_t size)
{
    char ui_bind[256] = "";
    size_t length = ARRAYSIZE(ui_bind) - 1;

    if (!runtime->get_annotation_string_from_uniform_variable(variable, "ui_bind", ui_bind, &length))
        return false;

    ui_bind[length] = '\0';
    reshade::api::format base_type = reshade::api::format::unknown;
    uint32_t rows = 0, cols = 0, array_length = 0;
    runtime->get_uniform_variable_type(variable, &base_type, &rows, &cols, &array_length);

    if (array_length != 0)
        return false;

    union
    {
        float as_float[16];
        int32_t as_int[16];
        uint32_t as_uint[16];
    } uniform_value{};

    std::memcpy(&uniform_value, value, std::min(sizeof(uniform_value), size));

    std::string str; str.reserve(40);
    for (size_t row = 0; row < rows; ++row)
    {
        for (size_t col = 0; col < cols; ++col)
        {
            const size_t array_index = cols * row + col;
            switch (base_type)
            {
                case reshade::api::format::r32_typeless: // reshadefx::type::t_bool
                    str += uniform_value.as_uint[array_index] ? "1" : "0";
                    break;
                case reshade::api::format::r16_sint: // reshadefx::type::t_min16int:
                case reshade::api::format::r32_sint: // reshadefx::type::t_int:
                    str += std::format("%d", uniform_value.as_int[array_index]);
                    break;
                case reshade::api::format::r16_uint: // reshadefx::type::t_min16uint:
                case reshade::api::format::r32_uint: // reshadefx::type::t_uint:
                    str += std::format("%u", uniform_value.as_uint[array_index]);
                    break;
                case reshade::api::format::r16_float: // reshadefx::type::t_min16float:
                case reshade::api::format::r32_float: // reshadefx::type::t_float:
                    str += std::format("%.8e", uniform_value.as_float[array_index]);
                    break;
                default: // reshade::api::format::unknown
                    str += '0';
                    break;
            }
            str += ',';
        }
    }
    if (!str.empty())
        str.resize(str.size() - 1);
#if 0
    switch (format)
    {
        case reshade::api::format::r32_typeless: // reshadefx::type::t_bool
            str = std::format("matrix<bool,%u,%u>(%*s)", out_rows, out_columns, str.size(), str.c_str());
            break;
        case reshade::api::format::r16_sint: // reshadefx::type::t_min16int:
            str = std::format("matrix<min16int,%u,%u>(%*s)", out_rows, out_columns, str.size(), str.c_str());
            break;
        case reshade::api::format::r32_sint: // reshadefx::type::t_int:
            str = std::format("matrix<int,%u,%u>(%*s)", out_rows, out_columns, str.size(), str.c_str());
            break;
        case reshade::api::format::r16_uint: // reshadefx::type::t_min16uint:
            str = std::format("matrix<min16uint,%u,%u>(%*s)", out_rows, out_columns, str.size(), str.c_str());
            break;
        case reshade::api::format::r32_uint: // reshadefx::type::t_uint:
            str = std::format("matrix<uint,%u,%u>(%*s)", out_rows, out_columns, str.size(), str.c_str());
            break;
        case reshade::api::format::r16_float: // reshadefx::type::t_min16float:
            str = std::format("matrix<min16float,%u,%u>(%*s)", out_rows, out_columns, str.size(), str.c_str());
            break;
        case reshade::api::format::r32_float: // reshadefx::type::t_float:
            str = std::format("matrix<float,%u,%u>(%*s)", out_rows, out_columns, str.size(), str.c_str());
            break;
        default: // reshade::api::format::unknown
            break;
    }
#endif

    char effect_name[256] = "";
    runtime->get_uniform_variable_effect_name(variable, effect_name);

    uibind_context &ctx = runtime->create_private_data<uibind_context>();
    ctx.replaces.emplace_back(effect_name, ui_bind, str);

    return false;
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD fdwReason, LPVOID)
{
    switch (fdwReason)
    {
        case DLL_PROCESS_ATTACH:
            if (!reshade::register_addon(hModule))
                return FALSE;
            reshade::register_event<reshade::addon_event::init_effect_runtime>(on_init);
            reshade::register_event<reshade::addon_event::destroy_effect_runtime>(on_destroy);
            reshade::register_event<reshade::addon_event::reshade_present>(on_reshade_present);
            reshade::register_event<reshade::addon_event::reshade_set_uniform_value>(on_reshade_set_uniform_value);
            break;
        case DLL_PROCESS_DETACH:
            reshade::unregister_addon(hModule);
            break;
    }

    return TRUE;
}
