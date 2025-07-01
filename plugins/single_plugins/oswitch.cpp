#include "wayfire/plugin.hpp"
#include "wayfire/toplevel-view.hpp"
#include "wayfire/util.hpp"
#include "wayfire/view-helpers.hpp"
#include <wayfire/output.hpp>
#include <wayfire/core.hpp>
#include <wayfire/view.hpp>
#include <wayfire/output-layout.hpp>
#include <wayfire/bindings-repository.hpp>
#include <wayfire/seat.hpp>

class wayfire_oswitch : public wf::plugin_interface_t
{
    wf::wl_idle_call idle_switch_output;

    wf::output_t *get_left_output()
    {
        return get_output_in_direction(-1, 0);
    }

    wf::output_t *get_right_output()
    {
        return get_output_in_direction(1, 0);
    }

    wf::output_t *get_up_output()
    {
        return get_output_in_direction(0, -1);
    }

    wf::output_t *get_down_output()
    {
        return get_output_in_direction(0, 1);
    }

    wf::output_t *get_output_relative(int step)
    {
        /* get the target output n steps after current output
         * if current output's index is i, and if there're n monitors
         * then return the (i + step) mod n th monitor */
        auto current_output = wf::get_core().seat->get_active_output();
        auto os = wf::get_core().output_layout->get_outputs();
        auto it = std::find(os.begin(), os.end(), current_output);
        if (it == os.end())
        {
            LOGI("Current output not found in output list");
            return current_output;
        }

        int size = os.size();
        int current_index = it - os.begin();
        int target_index  = ((current_index + step) % size + size) % size;
        return os[target_index];
    }

    wf::output_t *get_output_in_direction(int dir_x, int dir_y)
    {
        auto current_output = wf::get_core().seat->get_active_output();
        if (!current_output)
        {
            return nullptr;
        }

        auto current_geo = current_output->get_layout_geometry();

        wf::output_t *best_output = nullptr;
        double best_score = -INFINITY;

        for (auto& output : wf::get_core().output_layout->get_outputs())
        {
            if (output == current_output)
            {
                continue;
            }

            auto geo  = output->get_layout_geometry();
            double dx = (geo.x + geo.width / 2) - (current_geo.x + current_geo.width / 2);
            double dy = (geo.y + geo.height / 2) - (current_geo.y + current_geo.height / 2);

            double score = dx * dir_x + dy * dir_y;

            if ((score > 0) && ((best_output == nullptr) || (score < best_score)))
            {
                best_output = output;
                best_score  = score;
            }
        }

        return best_output ? best_output : current_output;
    }

    void switch_to_output(wf::output_t *target_output)
    {
        if (!target_output)
        {
            LOGI("No output found in requested direction. Cannot switch.");
            return;
        }

        /* when we switch the output, the oswitch keybinding
         * may be activated for the next output, which we don't want,
         * so we postpone the switch */
        idle_switch_output.run_once([=] ()
        {
            wf::get_core().seat->focus_output(target_output);
            target_output->ensure_pointer(true);
        });
    }

    void switch_to_output_with_window(wf::output_t *target_output)
    {
        auto current_output = wf::get_core().seat->get_active_output();
        auto view =
            wf::find_topmost_parent(wf::toplevel_cast(wf::get_active_view_for_output(current_output)));
        if (view)
        {
            move_view_to_output(view, target_output, true);
        }

        switch_to_output(target_output);
    }

    wf::activator_callback next_output = [=] (auto)
    {
        auto target_output = get_output_relative(1);
        switch_to_output(target_output);
        return true;
    };

    wf::activator_callback next_output_with_window = [=] (auto)
    {
        auto target_output = get_output_relative(1);
        switch_to_output_with_window(target_output);
        return true;
    };

    wf::activator_callback prev_output = [=] (auto)
    {
        auto target_output = get_output_relative(-1);
        switch_to_output(target_output);
        return true;
    };

    wf::activator_callback prev_output_with_window = [=] (auto)
    {
        auto target_output = get_output_relative(-1);
        switch_to_output_with_window(target_output);
        return true;
    };

    wf::activator_callback switch_left = [=] (auto)
    {
        auto target_output = get_left_output();
        switch_to_output(target_output);
        return true;
    };

    wf::activator_callback switch_right = [=] (auto)
    {
        auto target_output = get_right_output();
        switch_to_output(target_output);
        return true;
    };

    wf::activator_callback switch_up = [=] (auto)
    {
        auto target_output = get_up_output();
        switch_to_output(target_output);
        return true;
    };

    wf::activator_callback switch_down = [=] (auto)
    {
        auto target_output = get_down_output();
        switch_to_output(target_output);
        return true;
    };

  public:
    void init()
    {
        auto& bindings = wf::get_core().bindings;
        bindings->add_activator(wf::option_wrapper_t<wf::activatorbinding_t>{"oswitch/next_output"},
            &next_output);
        bindings->add_activator(wf::option_wrapper_t<wf::activatorbinding_t>{"oswitch/next_output_with_win"},
            &next_output_with_window);
        bindings->add_activator(wf::option_wrapper_t<wf::activatorbinding_t>{"oswitch/prev_output"},
            &prev_output);
        bindings->add_activator(wf::option_wrapper_t<wf::activatorbinding_t>{"oswitch/prev_output_with_win"},
            &prev_output_with_window);
        bindings->add_activator(wf::option_wrapper_t<wf::activatorbinding_t>{"oswitch/left_output"},
            &switch_left);
        bindings->add_activator(wf::option_wrapper_t<wf::activatorbinding_t>{"oswitch/right_output"},
            &switch_right);
        bindings->add_activator(wf::option_wrapper_t<wf::activatorbinding_t>{"oswitch/up_output"},
            &switch_up);
        bindings->add_activator(wf::option_wrapper_t<wf::activatorbinding_t>{"oswitch/down_output"},
            &switch_down);
    }

    void fini()
    {
        auto& bindings = wf::get_core().bindings;
        bindings->rem_binding(&next_output);
        bindings->rem_binding(&next_output_with_window);
        bindings->rem_binding(&prev_output);
        bindings->rem_binding(&prev_output_with_window);
        bindings->rem_binding(&switch_left);
        bindings->rem_binding(&switch_right);
        bindings->rem_binding(&switch_up);
        bindings->rem_binding(&switch_down);
        idle_switch_output.disconnect();
    }
};

DECLARE_WAYFIRE_PLUGIN(wayfire_oswitch);
